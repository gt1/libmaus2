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

#if ! defined(REVERSETABLE_HPP)
#define REVERSETABLE_HPP

#include <cassert>
#include <libmaus2/autoarray/AutoArray.hpp>

namespace libmaus2
{
	namespace bitio
	{
		struct ReverseTable
		{
			unsigned int const b;
			::libmaus2::autoarray::AutoArray < ::libmaus2::autoarray::AutoArray<uint64_t> > A;

			uint64_t operator()(unsigned int const ib, uint64_t m) const
			{
				return A[ib][m];
			}

			ReverseTable(unsigned int const rb)
			: b(rb), A(b+1)
			{
				for ( unsigned int ib = 0; ib <= b; ++ib )
				{
					A[ib] = ::libmaus2::autoarray::AutoArray<uint64_t>(1ull << ib);

					for ( uint64_t i = 0; i < (1ull<<ib); ++i )
					{
						uint64_t w = i;
						uint64_t v = 0;

						for ( unsigned int j = 0; j < ib; ++j )
						{
							v <<= 1;
							v |= (w&1);
							w >>= 1;
						}

						A[ib][i] = v;
					}

					for ( uint64_t i = 0; i < (1ull<<ib); ++i )
					{
						assert ( A[ib][A[ib][i]] == i );
					}
				}
			}
		};
	}
}
#endif
