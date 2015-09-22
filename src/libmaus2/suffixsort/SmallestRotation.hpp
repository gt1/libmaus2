/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#if ! defined(LIBMAUS2_SUFFIXSORT_SMALLESTROTATION_HPP)
#define LIBMAUS2_SUFFIXSORT_SMALLESTROTATION_HPP

#include <libmaus2/suffixsort/divsufsort.hpp>
#include <libmaus2/lcp/LCP.hpp>

namespace libmaus2
{
	namespace suffixsort
	{
		struct SmallestRotation
		{
			static uint64_t smallestRotation(std::string const & s)
			{
				unsigned char const * u = reinterpret_cast<unsigned char const *>(s.c_str());
				uint64_t const n = s.size();
				
				libmaus2::autoarray::AutoArray<unsigned char> T(n ? (2*n-1) : 0);
				libmaus2::autoarray::AutoArray<int> SA(n ? (2*n-1) : 0);
				
				if ( n )
				{
					std::copy(u,u+n,T.begin());
					std::copy(u,u+n-1,T.begin()+n);

					DivSufSort<32,unsigned char *, unsigned char const *, int *, int const *,256,false>::divsufsort(T.begin(),SA.begin(),2*n-1);
					
					libmaus2::autoarray::AutoArray<int> LCP = libmaus2::lcp::computeLcp(T.begin(),2*n-1,SA.begin());
					
					int prev = -1;
					
					for ( uint64_t i = 0; i < 2*n-1; ++i )
						if ( SA[i] < static_cast<int64_t>(n) )
						{
							int64_t l = LCP[i];
							for ( int64_t j = prev + 1; j < static_cast<int64_t>(i); ++j )
								l = std::min(static_cast<int64_t>(LCP[j]),l);
							LCP[i] = l;
							prev = i;						
						}
					
					uint64_t o = 0;
					for ( uint64_t i = 0; i < 2*n-1; ++i )
						if ( SA[i] < static_cast<int64_t>(n) )
						{
							LCP[o] = LCP[i];
							SA[o] = SA[i];
							o += 1;
						}

					uint64_t j = 1;
					while ( j < n && LCP[j] >= static_cast<int64_t>(n) )
						++j;
						
					return *std::min_element(SA.begin(),SA.begin()+j);
				}
				else
				{
					return 0;
				}
			}

			static std::string smallestRotationAsString(std::string const & s)
			{
				uint64_t const j = smallestRotation(s);
				std::string r = s;
				for ( uint64_t i = 0; i < s.size(); ++i )
					r[i] = s[(i+j)%s.size()];
				return r;
			}
		};
	}
}
#endif
