/*
    libmaus
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS_GRAPH_REACHABLEVERTICES_HPP)
#define LIBMAUS_GRAPH_REACHABLEVERTICES_HPP

#include <set>
#include <map>
#include <vector>
#include <stack>
#include <libmaus/types/types.hpp>

namespace libmaus
{
	namespace graph
	{
		struct ReachableVertices
		{
			template<typename edge_type, typename projector_type>
			static std::set<uint64_t> reachableVertices(
				std::map< uint64_t,std::vector<edge_type> > const & edges, uint64_t rv,
				projector_type projector = projector_type()
			)
			{
				std::set<uint64_t> seen;
				std::stack<uint64_t> S;
				S.push(rv);
				seen.insert(rv);
				
				while ( !S.empty() )
				{
					uint64_t const v = S.top();
					S.pop();
					
					if ( edges.find(v) != edges.end() )
					{
						std::vector<edge_type> const & V = edges.find(v)->second;
						
						for ( uint64_t i = 0; i < V.size(); ++i )
						{
							uint64_t const t = projector(V[i]);
							if ( seen.find(t) == seen.end() )
							{
								seen.insert(t);
								S.push(t);
							}
						}
					}
				}
				
				return seen;
			}
		};
	}
}
#endif
