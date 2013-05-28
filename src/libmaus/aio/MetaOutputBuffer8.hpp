/*
    libmaus
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

#if ! defined(LIBMAUS_AIO_METAOUTPUTBUFFER8_HPP)
#define LIBMAUS_AIO_METAOUTPUTBUFFER8_HPP

#include <libmaus/types/types.hpp>
#include <libmaus/aio/AsynchronousWriter.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/util/unique_ptr.hpp>
#include <string>

namespace libmaus
{
	namespace aio
	{
                struct MetaOutputBuffer8
                {
                        ::libmaus::autoarray::AutoArray<uint64_t> B;
                        uint64_t * const pa;
                        uint64_t * pc;
                        uint64_t * const pe;

                        ::libmaus::aio::AsynchronousWriter & W;
			uint64_t const metaid;

                        MetaOutputBuffer8(
				::libmaus::aio::AsynchronousWriter & rW, 
				uint64_t const bufsize,
				uint64_t const rmetaid)
                        : B(bufsize), pa(B.get()), pc(pa), pe(pa+B.getN()), W(rW), metaid(rmetaid)
                        {

                        }

                        void flush()
                        {
                                writeBuffer();
                        }

			void writeNumber8(uint64_t const num)
			{
				W.write (
					reinterpret_cast<char const *>(&num),
					reinterpret_cast<char const *>( (&num) + 1 )
				);
			}

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
