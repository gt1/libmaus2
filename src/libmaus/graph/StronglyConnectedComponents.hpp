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
#if ! defined(LIBMAUS_GRAPH_STRONGLYCONNECTEDCOMPONENTS_HPP)
#define LIBMAUS_GRAPH_STRONGLYCONNECTEDCOMPONENTS_HPP

#include <vector>
#include <map>
#include <stack>
#include <algorithm>
#include <cassert>
#include <libmaus/types/types.hpp>

namespace libmaus
{
	namespace graph
	{
		struct StronglyConnectedComponents
		{
			struct StrongConnectInfo
			{
				uint64_t index;
				uint64_t lowlink;
				
				StrongConnectInfo() : index(0), lowlink(0) {}
				StrongConnectInfo(uint64_t rv) : index(rv), lowlink(rv) {}
				StrongConnectInfo(
					uint64_t const rindex,
					uint64_t const rlowlink
				) : index(rindex), lowlink(rlowlink)
				{
				
				}
			};
			
			struct StrongConnectStackElement
			{
				enum algorithm_stage_type { stage_initial, stage_enum_edges_call, stage_enum_edges_return, stage_stack_pop };
				
				uint64_t v;
				algorithm_stage_type algorithm_stage;
				uint64_t edgeid;
				
				StrongConnectStackElement()
				: v(0), algorithm_stage(stage_initial), edgeid(0)
				{
				
				}
				
				StrongConnectStackElement(uint64_t const rv, algorithm_stage_type const & ralgorithm_stage, uint64_t const redgeid = 0)
				: v(rv), algorithm_stage(ralgorithm_stage), edgeid(redgeid)
				{
				
				}
			};
				
			template<typename edge_type, typename projector_type>
			static std::pair< std::vector< uint64_t >, std::vector< uint64_t > > strongConnect(std::map< uint64_t,std::vector<edge_type> > const & edges, uint64_t rv, projector_type projector = projector_type())
			{
				uint64_t index = 0;
				uint64_t compid = 0;
						
				std::stack< StrongConnectStackElement > T;
				T.push(StrongConnectStackElement(rv,StrongConnectStackElement::stage_initial));

				std::stack<uint64_t> S;
				std::map<uint64_t,StrongConnectInfo> info;
				
				std::vector< uint64_t > components;
				std::vector< uint64_t > componentsizes;
					
				while ( ! T.empty() )
				{
					StrongConnectStackElement const el = T.top(); T.pop();

					switch ( el.algorithm_stage )
					{
						case StrongConnectStackElement::stage_initial:
						{
							info[el.v] = StrongConnectInfo(index++);
							S.push(el.v);
							
							if ( edges.find(el.v) != edges.end() && edges.find(el.v)->second.size() )
								T.push(StrongConnectStackElement(el.v,StrongConnectStackElement::stage_enum_edges_call,0));
							else
								T.push(StrongConnectStackElement(el.v,StrongConnectStackElement::stage_stack_pop));
						}
						break;
						case StrongConnectStackElement::stage_enum_edges_call:
						{
							assert ( edges.find(el.v) != edges.end() );
							std::vector<edge_type> const & V = edges.find(el.v)->second;
							assert ( el.edgeid < V.size() );
							edge_type const & edge = V[el.edgeid];

							if ( info.find(projector(edge)) == info.end() )
							{
								T.push(StrongConnectStackElement(el.v,StrongConnectStackElement::stage_enum_edges_return,el.edgeid));
								T.push(StrongConnectStackElement(projector(edge),StrongConnectStackElement::stage_initial));
							}
							else
							{
								info[el.v].lowlink = std::min(info[el.v].lowlink,info[projector(edge)].index);
								
								if ( el.edgeid+1 < V.size() )
									T.push(StrongConnectStackElement(el.v,StrongConnectStackElement::stage_enum_edges_call,el.edgeid+1));
								else
									T.push(StrongConnectStackElement(el.v,StrongConnectStackElement::stage_stack_pop));					
							}
						}
						break;
						case StrongConnectStackElement::stage_enum_edges_return:
						{
							assert ( edges.find(el.v) != edges.end() );
							std::vector<edge_type> const & V = edges.find(el.v)->second;
							assert ( el.edgeid < V.size() );
							edge_type const & edge = V[el.edgeid];
							
							info[el.v].lowlink = std::min(info[el.v].lowlink,info[projector(edge)].lowlink);
							
							if ( el.edgeid + 1 < V.size() )
								T.push(StrongConnectStackElement(el.v,StrongConnectStackElement::stage_enum_edges_call,el.edgeid+1));
							else
								T.push(StrongConnectStackElement(el.v,StrongConnectStackElement::stage_stack_pop));
						}
						break;
						case StrongConnectStackElement::stage_stack_pop:
						{
							if ( info[el.v].index == info[el.v].lowlink )
							{
								// std::cerr << "pop for " << el.v << std::endl;
							
								uint64_t v = el.v+1;
								uint64_t s = 0;
								
								while ( v != el.v )
								{
									v = S.top();
									components.push_back(v);
									s += 1;
									// std::cerr << "add " << v << std::endl;
									S.pop();							
								}
								
								std::sort(components.end()-s,components.end());
								componentsizes.push_back(s);
								
								compid += 1;
							}
						}
						break;
					}
				}
				
				// compute prefix sums over componentsizes
				uint64_t acc = 0;
				for ( uint64_t i = 0; i < componentsizes.size(); ++i )
				{
					uint64_t const t = componentsizes[i];
					componentsizes[i] = acc;
					acc += t;
				}
				// store sum over all sizes
				componentsizes.push_back(acc);

				return std::pair< std::vector< uint64_t >, std::vector< uint64_t > >(components,componentsizes);
			}
		};
	}
}
#endif
