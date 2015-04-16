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
#if ! defined(LIBMAUS2_BITIO_CLZ_HPP)
#define LIBMAUS2_BITIO_CLZ_HPP

#include <libmaus2/LibMausConfig.hpp>
#include <libmaus2/types/types.hpp>

namespace libmaus2
{
	namespace bitio
	{
		struct Clz
		{
			/**
			 * count number of leading zero bits in 64 bit word code
			 **/
			static inline unsigned int clz(uint64_t code)
			{
				#if defined(__GNUC__) && (LIBMAUS2_SIZEOF_UNSIGNED_LONG == 8)
				return __builtin_clzl(code);
				#elif defined(__GNUC__) && (LIBMAUS2_SIZEOF_UNSIGNED_LONG == 4)
				if ( code >> 32 )
					return __builtin_clzl(code >> 32);
				else
					return 32 + __builtin_clzl(code);
				#else
				unsigned int c = 0;
				if ( code >> 32 ) code >>= 32; else c += 32;
				if ( code >> 16 ) code >>= 16; else c += 16;
				if ( code >>  8 ) code >>=  8; else c +=  8;
				if ( code >>  4 ) code >>=  4; else c +=  4;
				if ( code >>  2 ) code >>=  2; else c +=  2;
				if ( code >>  1 ) code >>=  1; else c +=  1;
				if ( !code      ) c += 1;
				return c;
				#endif
			}

		};
	}
}
#endif
