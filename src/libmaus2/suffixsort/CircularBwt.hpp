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
#if ! defined(LIBMAUS2_SUFFIXSORT_CIRCULARBWT_HPP)
#define LIBMAUS2_SUFFIXSORT_CIRCULARBWT_HPP

#include <libmaus2/suffixsort/divsufsort.hpp>

namespace libmaus2
{
	namespace suffixsort
	{
		struct CircularBwt
		{
			static std::pair< std::string,std::vector<bool> > circularBwt(std::string const s, uint64_t const beg, uint64_t const len, int64_t const term, uint64_t * zrank = 0)
			{
				assert ( beg + len <= s.size() );
				std::string const s2 = s+s;

				// std::cerr << "[V] Block sorting...";
				uint8_t const * utext = reinterpret_cast<uint8_t const *>(s2.c_str()) + beg;
				typedef ::libmaus2::suffixsort::DivSufSort<32,uint8_t *,uint8_t const *,int32_t *,int32_t const *> sort_type;
				typedef sort_type::saidx_t saidx_t;
				::libmaus2::autoarray::AutoArray<saidx_t> SA(s2.size()-beg);
				sort_type::divsufsort ( utext, SA.begin() , SA.size() );
				// std::cerr << "done." << std::endl;
				
				std::vector<bool> gt(len+1,false);

				std::string bwt(len,' ');
				uint64_t j = 0;
				bool gtf = false;
				for ( uint64_t i = 0; i < SA.size(); ++i )
					if ( SA[i] < static_cast<saidx_t>(len) )
					{
						gt [ SA[i] ] = gtf;
						
						#if defined(DEBUG)
						assert ( gtf == (s2.substr(beg+SA[i]) > s2.substr(beg)) );
						#endif
						
						if ( SA[i] == 0 )
						{
							if ( zrank )
								*zrank = j;
							bwt[j++] = term; // 0;
							gtf = true;
						}
						else
						{
							bwt[j++] = utext [ SA[i]-1 ];
						}
					}
					
				#if defined(DEBUG)
				std::cerr << "-----\n\n";
				for ( uint64_t i = 0; i < SA.size(); ++i )
					if ( SA[i] < len )
						std::cerr << "[" << std::setw(2) << std::setfill('0') << SA[i] << std::setw(0) << "] = " << s2.substr(beg+SA[i]) << std::endl;
				#endif
				
				std::string const gtbackleft = s2.substr(beg+len);
				std::string const gtbackright = s2.substr(beg);
				
				#if defined(DEBUG)
				std::cerr << "gt[len] = " << gtbackleft << " > " << gtbackright << " = " << (gtbackleft > gtbackright) << std::endl;
				#endif
				
				gt[len] = gtbackleft > gtbackright;

				assert ( j == len );

				return std::make_pair(bwt,gt);
			}
		};
	}
}
#endif
