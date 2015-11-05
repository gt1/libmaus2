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


#if ! defined(LIBMAUS2_LCS_LCSDYNAMIC_HPP)
#define LIBMAUS2_LCS_LCSDYNAMIC_HPP

#include <string>
#include <libmaus2/types/types.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>

namespace libmaus2
{
	namespace lcs
	{
		struct LCSDynamic
		{
			static std::pair<uint64_t,uint64_t> lcs(std::string const & a, std::string const & b)
			{
				uint64_t const n = a.size(), m = b.size();
				::libmaus2::autoarray::AutoArray<uint32_t> M( 2*m , false );
				uint32_t * m0 = M.get();
				uint32_t * m1 = m0+m;
				uint32_t maxlcp = 0;
				uint32_t maxpos = 0;

				uint32_t * tm1 = m1;
				for ( uint32_t j = 0; j < m; ++ j )
					if ( a[0] == b[j] )
						maxlcp = *(tm1++) = 1;
					else
						*(tm1++) = 0;
				std::swap(m0,m1);

				for ( uint32_t i = 1; i < n; i++ )
				{
					uint32_t * tm1 = m1;

					if ( a[i] == b[0] )
					{
						*(tm1++) = 1;
						if ( ! maxlcp )
							maxlcp = 1, maxpos = i;
					}
					else
					{
						*(tm1++) = 0;
					}

					for ( uint32_t j = 1; j < m; ++j, ++tm1 )
						if ( a[i] == b[j] )
						{
							uint32_t const v = *tm1 = m0[j-1] + 1;

							if ( v > maxlcp )
								maxlcp = v, maxpos = i;
						}
						else
						{
							*tm1 = 0;
						}

					std::swap(m0,m1);
				}

				return std::pair<uint64_t,uint64_t>(maxlcp,maxpos-maxlcp+1);
			}
		};
	}
}
#endif
