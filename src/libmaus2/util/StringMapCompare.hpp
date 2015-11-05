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
#if ! defined(LIBMAUS2_UTIL_STRINGMAPCOMPARE_HPP)
#define LIBMAUS2_UTIL_STRINGMAPCOMPARE_HPP

#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <iostream>
#include <sstream>

namespace libmaus2
{
	namespace util
	{
		struct StringMapCompare
		{
			static std::string printPair(std::pair<std::string,std::string> const & P)
			{
				std::ostringstream ostr;
				ostr << "(" << P.first << "," << P.second << ")";
				return ostr.str();
			}

			static bool compareSortedStringPairVectors(
				std::vector< std::pair<std::string,std::string> > const & SA,
				std::vector< std::pair<std::string,std::string> > const & SB
			)
			{
				uint64_t ia = 0, ib = 0;

				for ( ; ia != SA.size() && ib != SB.size() ; ++ia, ++ib )
					if ( SA[ia] != SB[ib] )
					{
						#if 0
						std::cerr << printPair(SA[ia]) << " != " << printPair(SB[ib]) << std::endl;

						for ( uint64_t i = 0; i < SA.size(); ++i )
							std::cerr << SA[i].first << ";";
						std::cerr << MA.size();
						std::cerr << std::endl;
						for ( uint64_t i = 0; i < SB.size(); ++i )
							std::cerr << SB[i].first << ";";
						std::cerr << MB.size();
						std::cerr << std::endl;
						#endif

						return SA[ia] < SB[ib];
					}

				return ia < ib;
			}

			template<typename map_type>
			static bool compare(map_type const & MA, map_type const & MB)
			{
				std::vector < std::pair<std::string,std::string> > SA, SB;
				for ( typename map_type::const_iterator ita = MA.begin(); ita != MA.end(); ++ita )
					SA.push_back(std::pair<std::string,std::string>(ita->first,ita->second));
				for ( typename map_type::const_iterator ita = MB.begin(); ita != MB.end(); ++ita )
					SB.push_back(std::pair<std::string,std::string>(ita->first,ita->second));
				std::sort(SA.begin(),SA.end());
				std::sort(SB.begin(),SB.end());

				return compareSortedStringPairVectors(SA,SB);
			}
		};
	}
}
#endif
