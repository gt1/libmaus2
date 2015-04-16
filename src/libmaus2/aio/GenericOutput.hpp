/*
    libmaus2
    Copyright (C) 2009-2013 German Tischler
    Copyright (C) 2011-2013 Genome Research Limited

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#if ! defined(LIBMAUS_AIO_GENERICOUTPUT_HPP)
#define LIBMAUS_AIO_GENERICOUTPUT_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/aio/AsynchronousWriter.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/util/GetFileSize.hpp>
#include <libmaus2/util/ArgInfo.hpp>
#include <libmaus2/util/unique_ptr.hpp>
#include <string>

namespace libmaus2
{
	namespace aio
	{
		/**
		 * class for asynchronously writing a sequence of elements of type data_type
		 **/
		template<typename data_type>
                struct GenericOutput
                {
                	//! this type
                        typedef GenericOutput this_type;
                        //! unique pointer type
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
                
			private:
			//! buffer
                        ::libmaus2::autoarray::AutoArray<data_type> B;
                        //! start of buffer pointer
                        data_type * const pa;
                        //! buffer current pointer
                        data_type * pc;
                        //! buffer end pointer
                        data_type * const pe;
                        //! asynchronous byte stream writer
                        ::libmaus2::aio::AsynchronousWriter W;

                        public:
                        /**
                         * write array A to file outputfilename
                         *
                         * @param A array to be written
                         * @param outputfilename name of output file
                         **/
			static void writeArray(::libmaus2::autoarray::AutoArray<data_type> const & A, 
				std::string const & outputfilename)
			{
				::libmaus2::aio::GenericOutput<data_type> out(outputfilename,64*1024);
				
				for ( uint64_t i = 0; i < A.getN(); ++i )
					out.put(A[i]);
				
				out.flush();
			}

			/**
			 * constructor
			 *
			 * @param filename output file name
			 * @param bufsize size of buffer in elements
			 **/
                        GenericOutput(std::string const & filename, uint64_t const bufsize)
                        : B(bufsize), pa(B.get()), pc(pa), pe(pa+B.getN()), W(filename,16)
                        {

                        }

                        /**
                         * flush buffer and underlying file
                         **/
                        void flush()
                        {
                                writeBuffer();
                                W.flush();
                        }

                        /**
                         * write buffer to asynchronous output stream
                         **/
                        void writeBuffer()
                        {
                                W.write ( 
                                        reinterpret_cast<char const *>(pa),
                                        reinterpret_cast<char const *>(pc));
                                pc = pa;
                        }

                        /**
                         * put a single element c in the buffer
                         *
                         * @param c element to be put in buffer
                         **/
                        void put(uint64_t const c)
                        {
                                *(pc++) = c;
                                if ( pc == pe )
                                        writeBuffer();
                        }
                };
	}
}
#endif
