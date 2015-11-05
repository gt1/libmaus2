/*
    libmaus2
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#if ! defined(LIBMAUS2_FASTX_REFPATHTOKENVECTORSEQUENCE_HPP)
#define LIBMAUS2_FASTX_REFPATHTOKENVECTORSEQUENCE_HPP

#include <libmaus2/fastx/RefPathTokenVector.hpp>
#include <cassert>

namespace libmaus2
{
	namespace fastx
	{
		struct RefPathTokenVectorSequence
		{
			std::vector<RefPathTokenVector> V;

			RefPathTokenVectorSequence(std::string const & s = std::string())
			{
				size_t i = 0;
				while ( i < s.size() )
				{
					size_t j = i;
					std::ostringstream substr;
					while (
						((j  ) < s.size() && s[j  ] != ':')
						||
						((j+1) < s.size() && s[j  ] == ':' && s[j+1] == ':')
					)
					{
						if ( s[j] != ':' )
						{
							substr.put(s[j++]);
						}
						else
						{
							assert (
								j+1 < s.size() &&
								s[j] == ':' &&
								s[j+1] == ':' );
							substr.put(':');
							j += 2;
						}
					}

					if ( j < s.size() )
					{
						assert ( s[j] == ':' );
						j += 1;
					}

					std::string const sub = substr.str();
					if ( sub.size() )
					{
						V.push_back(RefPathTokenVector(sub));
					}

					i = j;
				}
			}

			std::vector<std::string> expand(std::string const & s) const
			{
				std::vector<std::string> E;
				for ( size_t i = 0; i < V.size(); ++i )
					E.push_back(V[i].expand(s));
				return E;
			}
		};
	}
}
#endif
