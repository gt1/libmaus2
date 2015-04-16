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
#if ! defined(LIBMAUS_GRAPH_TOPOLOGICALSORTING_HPP)
#define LIBMAUS_GRAPH_TOPOLOGICALSORTING_HPP

#include <libmaus/graph/ReachableVertices.hpp>

namespace libmaus
{
	namespace graph
	{
		struct TopologicalSorting
		{
			template<typename edge_type, typename projector_type>
			static std::pair<bool,std::map<uint64_t,uint64_t> > topologicalSorting(
				std::map< uint64_t,std::vector<edge_type> > const & edges, 
				uint64_t rv, 
				projector_type projector = projector_type()
			)
			{
				std::set<uint64_t> vertexSet = libmaus::graph::ReachableVertices::reachableVertices(edges,rv,projector);
				
				std::map< uint64_t,std::set<uint64_t> > fedges;
				std::map< uint64_t,std::set<uint64_t> > redges;
				
				// extract forward and reverse edges
				for ( std::set<uint64_t>::const_iterator ita = vertexSet.begin(); ita != vertexSet.end(); ++ita )
					if ( edges.find(*ita) != edges.end() )
					{
						std::vector<edge_type> const & V = edges.find(*ita)->second;
						uint64_t const s = *ita;
						for ( uint64_t i = 0; i < V.size(); ++i )
						{
							uint64_t const t = projector(V[i]);
							fedges[s].insert(t);
							redges[t].insert(s);
						}
					}
					
				// vertices with no incoming edges
				std::deque<uint64_t> S;
				for ( std::set<uint64_t>::const_iterator ita = vertexSet.begin(); ita != vertexSet.end(); ++ita )
					if ( redges.find(*ita) == redges.end() )
						S.push_back(*ita);
				
				std::vector<uint64_t> L;		
				while ( S.size() )
				{
					uint64_t const s = S.front(); S.pop_front();
					L.push_back(s);
					
					if ( fedges.find(s) != fedges.end() )
					{
						std::set<uint64_t> const & fS = fedges.find(s)->second;
						std::vector<uint64_t> V(fS.begin(),fS.end());
						for ( uint64_t i = 0; i < V.size(); ++i )
						{
							uint64_t const t = V[i];
							
							// erase edge (s->t) and its reverse
							fedges[s].erase(fedges[s].find(t));
							if ( fedges.find(s)->second.size() == 0 )
								fedges.erase(fedges.find(s));
							redges[t].erase(redges[t].find(s));
							if ( redges.find(t)->second.size() == 0 )
								redges.erase(redges.find(t));
								
							if ( redges.find(t) == redges.end() )
								S.push_back(t);
						}
					}
				}
				
				if ( fedges.size() )
				{
					return std::pair<bool,std::map<uint64_t,uint64_t> >(false,std::map<uint64_t,uint64_t>());
				}
				else
				{
					std::map<uint64_t,uint64_t> M;
					for ( uint64_t i = 0; i < L.size(); ++i )
						M[L[i]] = i;
						
					return std::pair<bool,std::map<uint64_t,uint64_t> >(true,M);
				}
			}
		};
	}
}
#endif
