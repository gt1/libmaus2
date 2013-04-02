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
#if ! defined(LIBMAUS_BITIO_CTZ_HPP)
#define LIBMAUS_BITIO_CTZ_HPP

#include <libmaus/LibMausConfig.hpp>
#include <libmaus/types/types.hpp>

namespace libmaus
{
	namespace bitio
	{
		struct Ctz
		{
			/**
			 * count number of leading zero bits in 64 bit word code
			 **/
			static inline unsigned int ctz(uint64_t code)
			{
				#if defined(__GNUC__) && (LIBMAUS_SIZEOF_UNSIGNED_LONG == 8)
				return __builtin_ctzl(code);
				#elif defined(__GNUC__) && (LIBMAUS_SIZEOF_UNSIGNED_LONG == 4)
				if ( (code & 0xFFFFFFFFull) )
					return __builtin_ctzl(code & 0xFFFFFFFFULL);
				else
					return 32 + __builtin_ctzl(code >> 32);
				#else
				unsigned int c = 0;
				if ( !(code & 0xFFFFFFFFull) ) { code >>= 32; c += 32; }
				if ( !(code & 0xFFFFull) ) { code >>= 16; c += 16; }
				if ( !(code & 0xFFull) ) { code >>=  8; c +=  8; }
				if ( !(code & 0xFull) ) { code >>=  4; c +=  4; }
				if ( !(code & 0x3ull) ) { code >>=  2; c +=  2; }
				if ( !(code & 0x1ull) ) { code >>=  1; c +=  1; }
				if ( !code      ) c += 1;
				return c;
				#endif
			}

		};
	}
}
#endif
