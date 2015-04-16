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

#if ! defined(BITSPERNUM_HPP)
#define BITSPERNUM_HPP

#include <sys/types.h>
#include <libmaus2/types/types.hpp>

namespace libmaus2
{
	namespace math
	{
		inline unsigned int bitsPerNum(uint64_t k)
		{
			unsigned int c = 0;

			while ( k & (~(0xFFFFull)) ) { k >>= 16; c += 16; }
			if    ( k & (~(0x00FFull)) ) { k >>=  8; c +=  8; }
			if    ( k & (~(0x000Full)) ) { k >>=  4; c +=  4; }
			if    ( k & (~(0x0003ull)) ) { k >>=  2; c +=  2; }
			if    ( k & (~(0x0001ull)) ) { k >>=  1; c +=  1; }
			if    ( k             ) { k >>=  1; c +=  1; }

			return c;
		}
	}
}
#endif
