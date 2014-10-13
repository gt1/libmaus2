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
#include <set>
#include <algorithm>
#include <cassert>
#include <libmaus/types/types.hpp>

#include <iostream>

namespace libmaus
{
	namespace graph
	{
		struct StronglyConnectedComponents
		{
			#if 0
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
			static std::pair< std::vector< uint64_t >, std::vector< uint64_t > > strongConnectKosaraju(
				std::map< uint64_t,std::vector<edge_type> > const & edges, uint64_t rv, projector_type projector = projector_type()
			)
			{
				std::pair< std::vector< uint64_t >, std::vector< uint64_t > > C;
				
				// generate forward and reverse edges
				std::map<uint64_t, std::vector< uint64_t > > F;
				std::map<uint64_t, std::vector< uint64_t > > R;
				for ( typename std::map< uint64_t,std::vector<edge_type> >::const_iterator ita = edges.begin(); ita != edges.end(); ++ita )
				{
					uint64_t const s = ita->first;
					std::vector<edge_type> const & V = ita->second;
					for ( uint64_t i = 0; i < V.size(); ++i )
					{
						uint64_t const t = projector(V[i]);
						F[s].push_back(t);
						R[t].push_back(s);
					}
				}
				
				std::set<uint64_t> seen;
				seen.insert(rv);
				std::stack<uint64_t> P;
				
				enum visit_type { visit_first, visit_second };
				typedef std::pair<uint64_t,visit_type> stack_element_type;
				std::stack<stack_element_type> S;
				S.push(stack_element_type(rv,visit_second));
				S.push(stack_element_type(rv,visit_first));
				
				while ( ! S.empty() )
				{
					stack_element_type se = S.top(); S.pop();
					
					if ( se.second == visit_first && F.find(se.first) != F.end() )
					{
						std::vector< uint64_t > const & V = F.find(se.first)->second;
							
						for ( uint64_t i = 0; i < V.size(); ++i )
							if ( seen.find(V[i]) == seen.end() )
							{
								S.push(stack_element_type(V[i],visit_second));
								S.push(stack_element_type(V[i],visit_first));			
								seen.insert(V[i]);
							}
					}
					else if ( se.second == visit_second )
						P.push(se.first);
				}
				
				seen.clear();
				while ( P.size() )
				{
					std::stack<uint64_t> T;
					T.push(P.top());
					seen.insert(P.top());
					uint64_t c = 0;
					
					while ( ! T.empty() )
					{
						++c;
						uint64_t const node = T.top(); T.pop();
						C.first.push_back(node);
						
						if ( R.find(node) != R.end() )
						{
							std::vector<uint64_t> const & V = R.find(node)->second;
							
							for ( uint64_t i = 0; i < V.size(); ++i )
								if ( seen.find(V[i]) == seen.end() )
								{
									T.push(V[i]);
									seen.insert(V[i]);
								}
						}
					}
					
					std::sort(C.first.end()-c,C.first.end());
					
					C.second.push_back(c);
					
					while( P.size() && (seen.find(P.top()) != seen.end()) )
						P.pop();
				}
				
				uint64_t acc = 0;
				for ( uint64_t i = 0; i < C.second.size(); ++i )
				{
					uint64_t const t = C.second[i];
					C.second[i] = acc;
					acc += t;
				}
				C.second.push_back(acc);
				
				return C;
			}

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
			#endif

			enum strongly_connected_component_contract_visit_type { visit_first, visit_second };

			template<typename edge_type, typename projector_type>
			static std::pair< std::vector< uint64_t >, std::vector< uint64_t > > strongConnectContract(
				std::map< uint64_t,std::vector<edge_type> > const & inedges, 
				uint64_t rv, projector_type projector = projector_type()
			)
			{
				std::map< uint64_t,std::vector<uint64_t> > edges;

				for ( typename std::map< uint64_t,std::vector<edge_type> >::const_iterator ita = inedges.begin(); ita != inedges.end(); ++ita )
				{
					std::vector<edge_type> const & V = ita->second;
					
					for ( uint64_t i = 0; i < V.size(); ++i )
						edges[ita->first].push_back(projector(V[i]));
				}

				bool changed = true;
				std::map<uint64_t,uint64_t> vertmap;
				
				while ( changed )
				{	
					#if 0
					std::cerr << "digraph {\n";
				
					for ( std::map< uint64_t,std::vector<uint64_t> >::const_iterator ita = edges.begin(); ita != edges.end(); ++ita )
					{
						std::vector<uint64_t> const & V = ita->second;
						for ( uint64_t i = 0; i < V.size(); ++i )
							std::cerr << ita->first << " -> " << V[i] << "\n";
					}
				
					std::cerr << "}\n";
					#endif

					changed = false;

					while ( vertmap.find(rv) != vertmap.end() )
						rv = vertmap.find(rv)->second;

					std::set<uint64_t> unused;
					std::stack<uint64_t> PS;
					PS.push(rv);
					unused.insert(rv);
					
					while ( !PS.empty() )
					{
						uint64_t const v = PS.top(); PS.pop();
						
						if ( edges.find(v) != edges.end() )
						{
							std::vector<uint64_t> const & V = edges.find(v)->second;
							for ( uint64_t i = 0; i < V.size(); ++i )
								if ( unused.find(V[i]) == unused.end() )
								{
									PS.push(V[i]);
									unused.insert(V[i]);
								}
						}
					}
					
					for ( std::map< uint64_t,std::vector<uint64_t> >::const_iterator ita = edges.begin(); ita != edges.end(); ++ita )
						unused.insert(ita->first);
					
					while ( (!changed) && unused.size() )
					{
						uint64_t root = *(unused.begin());
						typedef std::pair<uint64_t,strongly_connected_component_contract_visit_type> st;
						std::stack< st > S;
						S.push(st(root,visit_first));
						std::set<uint64_t> onstack;
						bool foundloop = false;
						uint64_t onstackrec = 0;
						
						while ( (!foundloop) && (! S.empty()) )
						{
							st const el = S.top(); S.pop();
							
							switch ( el.second )
							{
								case visit_first:
									onstack.insert(el.first);
									S.push(st(el.first,visit_second));
									
									if ( unused.find(el.first) != unused.end() )
										unused.erase(unused.find(el.first));

									if ( edges.find(el.first) != edges.end() )
									{
										std::vector<uint64_t> const & V = edges.find(el.first)->second;
										
										for ( uint64_t i = 0; i < V.size(); ++i )
											if ( onstack.find(V[i]) != onstack.end() )
											{
												foundloop = true;
												onstackrec = V[i];
											}
											
										if ( ! foundloop )
										{
											for ( uint64_t i = 0; i < V.size(); ++i )
												S.push(st(V[i],visit_first));						
										}
									}
									
									break;
								case visit_second:
									assert ( onstack.find(el.first) != onstack.end() );
									onstack.erase(onstack.find(el.first));
									break;
							}
						}
						
						if ( foundloop )
						{
							// std::cerr << "foundloop size " << S.size() << std::endl;
						
							std::set<uint64_t> L;
							
							while ( (!S.empty()) && (S.top() != st(onstackrec,visit_second)) )
							{
								if ( S.top().second == visit_second )
									L.insert(S.top().first);
								S.pop();
							}
							
							assert ( S.size() && S.top() == st(onstackrec,visit_second) );
							
							L.insert(onstackrec);
							
							if ( L.size() > 1 )
							{
								#if 0
								std::cerr << "found loop of size " << L.size() << ": ";
								for ( std::set<uint64_t>::const_iterator ita = L.begin(); ita != L.end(); ++ita )
									std::cerr << *ita << ";";
								std::cerr << std::endl;
								#endif
								
								uint64_t const mini = *(L.begin());
								for ( std::set<uint64_t>::const_iterator ita = L.begin(); ita != L.end(); ++ita )
								{
									#if 0
									if ( *ita != mini )
										std::cerr << *ita << " -> " << mini << std::endl;
									#endif
									
									if ( *ita != mini )
										vertmap[*ita] = mini;
								}
								
								std::set<uint64_t> alltargets;
								for ( std::map< uint64_t,std::vector<uint64_t> >::iterator ita = edges.begin(); ita != edges.end(); ++ita )
									if ( L.find(ita->first) != L.end() )
									{
										std::vector<uint64_t> const & V = ita->second;
										for ( uint64_t i = 0; i < V.size(); ++i )
											if ( L.find(V[i]) == L.end() )
												alltargets.insert(V[i]);
									}
									else
									{
										std::vector<uint64_t> & V = ita->second;
										for ( uint64_t i = 0; i < V.size(); ++i )
											if ( L.find(V[i]) != L.end() )
												V[i] = mini;
										
										std::sort(V.begin(),V.end());							
										V.resize(
											std::unique(V.begin(),V.end())-V.begin()
										);
									}

								for ( std::set<uint64_t>::const_iterator ita = L.begin(); ita != L.end(); ++ita )
									if ( edges.find(*ita) != edges.end() )
										edges.erase(edges.find(*ita));
										
								for ( std::set<uint64_t>::const_iterator ita = alltargets.begin(); ita != alltargets.end(); ++ita )
									edges[mini].push_back(*ita);
								
								changed = true;
							}
						}
					}
				}
				
				changed = true;
				while ( changed )
				{
					changed = false;
					
					for ( std::map<uint64_t,uint64_t>::iterator ita = vertmap.begin(); ita != vertmap.end(); ++ita )
						if ( vertmap.find(ita->second) != vertmap.end() )
						{
							ita->second = vertmap.find(ita->second)->second;
							changed = true;
						}
				}

				#if 0
				std::cerr << std::string(80,'-') << std::endl;
				for ( std::map<uint64_t,uint64_t>::const_iterator ita = vertmap.begin(); ita != vertmap.end(); ++ita )
				{
					std::cerr << ita->first << " -> " << ita->second <<  ::std::endl;
				}
				#endif

				std::map< uint64_t,std::vector<uint64_t> > RM;
				for ( std::map<uint64_t,uint64_t>::const_iterator ita = vertmap.begin(); ita != vertmap.end(); ++ita )
					RM[ita->second].push_back(ita->first);
					
				std::vector<uint64_t> RV;
				std::vector<uint64_t> RA;
				for ( std::map< uint64_t,std::vector<uint64_t> >::const_iterator ita = RM.begin(); ita != RM.end(); ++ita )
				{
					RV.push_back(ita->first);
					std::vector<uint64_t> const & V = ita->second;
					for ( uint64_t i = 0; i < V.size(); ++i )
						RV.push_back(V[i]);
					RA.push_back(V.size()+1);
					std::sort(RV.end()-(RA.back()),RV.end());
				}
				
				uint64_t acc = 0;
				for ( uint64_t i = 0; i < RA.size(); ++i )
				{
					uint64_t const t = RA[i];
					RA[i] = acc;
					acc += t;
				}
				RA.push_back(acc);
				
				return std::pair< std::vector<uint64_t>, std::vector<uint64_t> >(RV,RA);
			}
		};

	}
}
#endif
