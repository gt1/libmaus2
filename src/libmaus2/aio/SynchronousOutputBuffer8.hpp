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


#if ! defined(LIBMAUS_AIO_SYNCHRONOUSOUTPUTBUFFER8_HPP)
#define LIBMAUS_AIO_SYNCHRONOUSOUTPUTBUFFER8_HPP

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
		 * synchronous block wise output class for 8 byte (uint64_t) numbers. this class will not keep the underlying
		 * output file open across its method calls
		 **/
                struct SynchronousOutputBuffer8
                {
                	//! this type
                        typedef SynchronousOutputBuffer8 this_type;
                        //! unique pointer type
                        typedef ::libmaus2::util::unique_ptr < this_type > :: type unique_ptr_type;
                
                        private:
                        //! output file name
			std::string const filename;
			//! output buffer
                        ::libmaus2::autoarray::AutoArray<uint64_t> B;
                        //! output buffer start pointer
                        uint64_t * const pa;
                        //! output buffer current pointer
                        uint64_t * pc;
                        //! output buffer end pointer
                        uint64_t * const pe;

                        /**
                         * write output buffer to disk and reset it
                         **/
                        void writeBuffer()
                        {
				std::ofstream ostr(filename.c_str(), std::ios::binary | std::ios::app);
                                ostr.write ( 
                                        reinterpret_cast<char const *>(pa),
                                        reinterpret_cast<char const *>(pc)-reinterpret_cast<char const *>(pa)
				);
				ostr.flush();
				
				if ( ! ostr )
				{
				        ::libmaus2::exception::LibMausException se;
				        se.getStream() << "Failed to flush buffer in SynchronousOutputBuffer8::writeBuffer()";
				        se.finish();
				        throw se;
				}

				ostr.close();
                                pc = pa;
                        }

                        public:
                        /**
                         * constructor
                         *
                         * @param rfilename output file name
                         * @param bufsize output buffer size
                         * @param truncate if true file will be truncated
                         **/
                        SynchronousOutputBuffer8(std::string const & rfilename, uint64_t const bufsize, bool truncate = true)
                        : filename(rfilename), B(bufsize), pa(B.get()), pc(pa), pe(pa+B.getN())
                        {
                                if ( truncate )
                                {
        				std::ofstream ostr(filename.c_str(), std::ios::binary);
	        			ostr.flush();
                                }

                        }
                        /**
                         * destructor, flushes the buffer
                         **/
			~SynchronousOutputBuffer8()
			{
				flush();
			}

			/**
			 * flushes the output buffer
			 **/
                        void flush()
                        {
                                writeBuffer();
                        }

                        /**
			 * put one element in the buffer and flushes the buffer it is full afterwards
			 *
			 * @param c element to be put in the buffer
			 **/
                        void put(uint64_t const c)
                        {
                                *(pc++) = c;
                                if ( pc == pe )
                                        writeBuffer();
                        }
                        
                        /**
                         * @return file name
                         **/
                        std::string const & getFilename() const
                        {
                        	return filename;
                        }
                };
	}
}
#endif

