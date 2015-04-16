/*
    libmaus
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS_RANK_BSWAPBASE_HPP)
#define LIBMAUS_RANK_BSWAPBASE_HPP

#include <libmaus/types/types.hpp>

namespace libmaus
{
	namespace rank
	{
		struct BSwapBase
		{
			virtual ~BSwapBase() {}
		
			/**
			 * invert byte order of 2 byte word
			 * @param val
			 * @return val in inverted byte order
			 **/
			static inline uint16_t bswap2(uint16_t val)
			{
				#if defined(LIBMAUS_USE_ASSEMBLY) && defined(LIBMAUS_HAVE_i386) 
				__asm__("xchg %%al,%%ah" : "+a"(val));
				return val;
				#else
				return ((val&0xFF)<<8) | ((val&0xFF00)>>8);
				#endif
			}
			/**
			 * invert byte order of 4 byte word
			 * @param val
			 * @return val in inverted byte order
			 **/
			static inline uint32_t bswap4(uint32_t val)
			{
				#if defined(LIBMAUS_USE_ASSEMBLY) && defined(LIBMAUS_HAVE_i386)
				__asm__("bswap %0" : "+r"(val));
				return val;
				#else
				return
					  ((val & 0xff000000) >> 24) 
					| ((val & 0x000000ff) << 24)
					| ((val & 0x00ff0000) >>  8) 
					| ((val & 0x0000ff00) <<  8) 
					;
				#endif
			}
			/**
			 * invert byte order of 8 byte word
			 * @param val
			 * @return val in inverted byte order
			 **/
			static inline uint64_t bswap8(uint64_t val)
			{
				#if defined(LIBMAUS_USE_ASSEMBLY) && defined(LIBMAUS_HAVE_x86_64)
				__asm__("bswap %0" : "+r"(val));
				return val;
				#else
				return    ((val & 0xff00000000000000ull) >> 56)
					| ((val & 0x00000000000000ffull) << 56)
					| ((val & 0x00ff000000000000ull) >> 40)
					| ((val & 0x000000000000ff00ull) << 40)
					| ((val & 0x0000ff0000000000ull) >> 24)
					| ((val & 0x0000000000ff0000ull) << 24)
					| ((val & 0x000000ff00000000ull) >> 8)
					| ((val & 0x00000000ff000000ull) << 8)
					;
				#endif
			}
		};
	}
}
#endif
