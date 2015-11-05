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


#if ! defined(LIBMAUS2_AIO_OUTPUTBUFFER8_HPP)
#define LIBMAUS2_AIO_OUTPUTBUFFER8_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/aio/AsynchronousWriter.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/util/unique_ptr.hpp>
#include <string>

namespace libmaus2
{
	namespace aio
	{
		/**
		 * asynchronous output buffer for 64 bit words
		 **/
                struct OutputBuffer8
                {
                	//! this type
                        typedef OutputBuffer8 this_type;
                        //! unique pointer type
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			private:
			//! buffer
                        ::libmaus2::autoarray::AutoArray<uint64_t> B;
                        //! buffer start pointer
                        uint64_t * const pa;
                        //! buffer current pointer
                        uint64_t * pc;
                        //! buffer end pointer
                        uint64_t * const pe;
                        //! async writer
                        ::libmaus2::aio::AsynchronousWriter W;

                        public:
                        /**
                         * write an array A to file outputfilename
                         *
                         * @param A array
                         * @param outputfilename output file name
                         **/
			static void writeArray(::libmaus2::autoarray::AutoArray<uint64_t> const & A,
				std::string const & outputfilename)
			{
				::libmaus2::aio::OutputBuffer8 out(outputfilename,64*1024);

				for ( uint64_t i = 0; i < A.getN(); ++i )
					out.put(A[i]);

				out.flush();
			}

			/**
			 * constructor
			 *
			 * @param filename output file name
			 * @param bufsize output buffer size
			 **/
                        OutputBuffer8(std::string const & filename, uint64_t const bufsize)
                        : B(bufsize), pa(B.get()), pc(pa), pe(pa+B.getN()), W(filename,16)
                        {

                        }

                        /**
                         * flush buffer
                         **/
                        void flush()
                        {
                                writeBuffer();
                                W.flush();
                        }

                        /**
                         * write current buffer and reset it
                         **/
                        void writeBuffer()
                        {
                                W.write (
                                        reinterpret_cast<char const *>(pa),
                                        reinterpret_cast<char const *>(pc));
                                pc = pa;
                        }

                        /**
                         * put one element c in buffer and flush if the buffer runs full
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
