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

#if ! defined(SWAPWORDBITBLOCK_HPP)
#define SWAPWORDBITBLOCK_HPP

#include <libmaus2/bitio/getBits.hpp>
#include <libmaus2/bitio/putBits.hpp>

namespace libmaus2
{
	namespace bitio
	{
		template<typename iterator>
		void swapWordBitBlock(iterator A, uint64_t const o0, uint64_t const o1, uint64_t const l)
		{
			uint64_t const v0 = getBits(A,o0,l);
			uint64_t const v1 = getBits(A,o1,l);
			putBits(A,o0,l,v1);
			putBits(A,o1,l,v0);
		}
		template<typename iterator>
		void swapBitBlock(iterator A, uint64_t o0, uint64_t o1, uint64_t const l)
		{
			uint64_t const fullwordswaps = l / 64;
			uint64_t const restbitswaps = l - fullwordswaps * 64;

			for ( uint64_t i = 0; i < fullwordswaps; ++i, o0 += 64, o1 += 64 )
				swapWordBitBlock(A,o0,o1,64);
					
			swapWordBitBlock(A,o0,o1,restbitswaps);
		}

		template<typename iterator>
		void swapBitBlocks(iterator A, uint64_t offset, uint64_t l0, uint64_t l1)
		{
			while ( l0 && l1 )
			{
				if ( l0 <= l1 )
				{
					// ABB' -> BAB'
					swapBitBlock(A,offset,offset+l0,l0);
					offset += l0;
					l1 -= l0;
				}
				else
				{
					// AA'B -> ABA'
					swapBitBlock(A,offset+(l0-l1),offset+l0,l1);
					l0 -= l1;
				}
			}
		}	
	}
}
#endif
