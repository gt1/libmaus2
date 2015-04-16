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

#if ! defined(REVERSEBYTEORDER_HPP)
#define REVERSEBYTEORDER_HPP

namespace libmaus2
{
	namespace util
	{
		struct ReverseByteOrder
		{
			template<typename N>
			static N reverseByteOrder(N n)
			{
				N A[sizeof(N)];

				// A[0] least significant
				for ( unsigned int i = 0; i < sizeof(N); ++i )
				{
					A[i] = n & 0xFF;
					n >>= 8;
				}

				N v = 0;
				for ( unsigned int i = 0; i < sizeof(N); ++i )
				{
					v <<= 8;
					v |= A[i];
				}

				return v;
			}
		};
	}
}
#endif
