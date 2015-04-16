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

#if ! defined(LIBMAUS2_AIO_METAOUTPUTBUFFER8_HPP)
#define LIBMAUS2_AIO_METAOUTPUTBUFFER8_HPP

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
		 * asynchronous block wise buffered output class attaching meta information to each block
		 **/
                struct MetaOutputBuffer8
                {
                	private:
                        ::libmaus2::autoarray::AutoArray<uint64_t> B;
                        uint64_t * const pa;
                        uint64_t * pc;
                        uint64_t * const pe;

                        ::libmaus2::aio::AsynchronousWriter & W;
			uint64_t const metaid;

			public:
			/**
			 * constructor
			 *
			 * @param rW output writer
			 * @param bufsize size of buffer
			 * @param rmetaid meta information for each written block
			 **/
                        MetaOutputBuffer8(
				::libmaus2::aio::AsynchronousWriter & rW, 
				uint64_t const bufsize,
				uint64_t const rmetaid)
                        : B(bufsize), pa(B.get()), pc(pa), pe(pa+B.getN()), W(rW), metaid(rmetaid)
                        {

                        }

                        /**
                         * flush the buffer
                         **/
                        void flush()
                        {
                                writeBuffer();
                        }

                        /**
                         * write number
                         *
                         * @param num number to be written
                         **/
			void writeNumber8(uint64_t const num)
			{
				W.write (
					reinterpret_cast<char const *>(&num),
					reinterpret_cast<char const *>( (&num) + 1 )
				);
			}

			/**
			 * written buffer contents
			 **/
                        void writeBuffer()
                        {
				if ( pc != pa )
				{
					writeNumber8(metaid);
					writeNumber8(pc-pa);

	                                W.write ( 
        	                                reinterpret_cast<char const *>(pa),
                	                        reinterpret_cast<char const *>(pc));
                        	        pc = pa;
				}
                        }

                        /**
                         * put one element in the buffer and flush if necessary
                         *
                         * @param c element to be put
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
