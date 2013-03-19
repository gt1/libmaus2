/**
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
**/

#if ! defined(SWAPBITBLOCKSBYREVERSAL_HPP)
#define SWAPBITBLOCKSBYREVERSAL_HPP

#include <libmaus/bitio/putBit.hpp>
#include <libmaus/bitio/getBit.hpp>

namespace libmaus
{
	namespace bitio
	{
		template<typename iterator>
		inline void reverseBitBlock(iterator A, uint64_t const offset, uint64_t const len)
		{
			uint64_t o0 = offset;
			uint64_t o1 = offset+len-1;
			uint64_t o2 = o0 + len/2;
			
			for ( ; o0 != o2; o0++, o1-- )
			{
				bool const b0 = getBit(A,o0);
				bool const b1 = getBit(A,o1);
				putBit(A,o0,b1);
				putBit(A,o1,b0);
			}
		}
		
		template<typename iterator>
		inline void swapBitBlocksByReversal(iterator A, uint64_t const offset, uint64_t const l0, uint64_t const l1)
		{
			reverseBitBlock(A, offset, l0);
			reverseBitBlock(A, offset+l0, l1);
			reverseBitBlock(A, offset, l0+l1);
		}
	}
}
#endif
