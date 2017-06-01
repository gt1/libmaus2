/*
    libmaus2
    Copyright (C) 2017 German Tischler

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
#if ! defined(LIBMAUS2_DAZZLER_STRINGGRAPH_STRINGGRAPH_HPP)
#define LIBMAUS2_DAZZLER_STRINGGRAPH_STRINGGRAPH_HPP

#include <libmaus2/dazzler/stringgraph/Edge.hpp>
#include <libmaus2/dazzler/db/DatabaseFile.hpp>
#include <libmaus2/aio/InputStreamInstance.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace stringgraph
		{
			struct StringGraph
			{
				typedef StringGraph this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

				std::vector < Edge > edges;
				std::vector < std::pair < OverlapNodeBase, uint64_t> > fromlist;
				std::vector < std::pair < OverlapNodeBase, uint64_t> > tolist;

				StringGraph() {}
				StringGraph(std::istream & in)
				{
					while ( in.peek() != std::istream::traits_type::eof() )
					{
						uint64_t const nextid = edges.size();

						edges.push_back(Edge(in));
						fromlist.push_back(std::pair < OverlapNodeBase, uint64_t>(edges.back().from,nextid));
						tolist.push_back(std::pair < OverlapNodeBase, uint64_t>(edges.back().to,nextid));
					}

					std::sort(fromlist.begin(),fromlist.end());
					std::sort(tolist.begin(),tolist.end());
				}

				struct OverlapNodeBasePairFirstComparator
				{
					bool operator()(std::pair < OverlapNodeBase, uint64_t> const & A, std::pair < OverlapNodeBase, uint64_t> const & B) const
					{
						return A.first < B.first;
					}
				};

				std::vector<uint64_t> searchFrom(OverlapNodeBase const & O) const
				{
					typedef std::pair < OverlapNodeBase, uint64_t> p_type;
					p_type const P(O,0);
					typedef std::vector<p_type>::const_iterator i_type;

					std::pair<i_type,i_type> const IP = std::equal_range(
						fromlist.begin(),
						fromlist.end(),
						P,
						OverlapNodeBasePairFirstComparator()
					);

					std::vector<uint64_t> V;
					for ( i_type i = IP.first; i != IP.second; ++i )
						V.push_back(i->second);

					return V;
				}

				std::vector<uint64_t> searchTo(OverlapNodeBase const & O) const
				{
					typedef std::pair < OverlapNodeBase, uint64_t> p_type;
					p_type const P(O,0);
					typedef std::vector<p_type>::const_iterator i_type;

					std::pair<i_type,i_type> const IP = std::equal_range(
						tolist.begin(),
						tolist.end(),
						P,
						OverlapNodeBasePairFirstComparator()
					);

					std::vector<uint64_t> V;
					for ( i_type i = IP.first; i != IP.second; ++i )
						V.push_back(i->second);

					return V;
				}

				static unique_ptr_type load(std::string const & fn)
				{
					libmaus2::aio::InputStreamInstance in(fn);
					unique_ptr_type tptr(new this_type(in));
					return UNIQUE_PTR_MOVE(tptr);
				}

				uint64_t size() const
				{
					return edges.size();
				}

				#if defined(STRING_GRAPH_DEBUG)
				static void search(std::string s, std::vector<std::string> const & Vtext, char const d)
				{
					if ( s.size() >= 32 )
						for ( uint64_t i = 0; i < Vtext.size(); ++i )
						{
							std::string::size_type p = 0;
							while ( (p=Vtext[i].find(s,p)) != std::string::npos )
							{
								std::cerr << d << ":" << i << ":" << p << std::endl;
								p += 1;
							}
						}
				}

				static void searchFR(std::string s, std::vector<std::string> const & Vtext)
				{
					search(s,Vtext,'f');
					search(libmaus2::fastx::reverseComplementUnmapped(s),Vtext,'r');
				}
				#endif

				std::string traverse(
					std::vector<uint64_t> const edgeids,
					libmaus2::dazzler::db::DatabaseFile const & DB
					#if defined(STRING_GRAPH_DEBUG)
						, std::vector<std::string> const & Vtext
					#endif
				) const
				{
					for ( uint64_t i = 1; i < edgeids.size(); ++i )
						assert (
							edges[i-1].to.id == edges[i].from.id
							&&
							edges[i-1].to.end != edges[i].from.end
						);

					std::ostringstream ostr;
					for ( uint64_t i = 0; i < edgeids.size(); ++i )
					{
						uint64_t const edgeid = edgeids[i];
						Edge const & E = edges[edgeid];

						if ( i == 0 )
						{
							OverlapNodeBase const first = E.from;

							bool const firstinv = (first.end==0);

							#if defined(STRING_GRAPH_DEBUG)
							std::cerr << first << std::endl;
							#endif

							std::string s;
							if ( firstinv )
								s = libmaus2::fastx::reverseComplementUnmapped(DB[first.id]);
							else
								s = DB[first.id];

							ostr << s;

							#if defined(STRING_GRAPH_DEBUG)
							searchFR(s,Vtext);
							#endif
						}

						for ( uint64_t j = 0; j < E.overlaps.size(); ++j )
						{
							OverlapNode const & next = E.overlaps[j];

							bool const nextinv = (next.end==0);

							std::string s =
								nextinv
								?
								libmaus2::fastx::reverseComplementUnmapped(DB[next.id])
								:
								DB[next.id]
								;

							s = s.substr(s.size() - next.length);

							#if defined(STRING_GRAPH_DEBUG)
							std::cerr << "next=" << next << " nextinv=" << nextinv << " s.size()=" << s.size() << std::endl;
							#endif

							#if defined(STRING_GRAPH_DEBUG)
							searchFR(s,Vtext);
							#endif

							ostr << s;
						}
					}

					return ostr.str();
				}

				std::string traverse(
					uint64_t const i,
					libmaus2::dazzler::db::DatabaseFile const & DB
					#if defined(STRING_GRAPH_DEBUG)
						, std::vector<std::string> const & text
					#endif
				) const
				{
					return traverse(std::vector<uint64_t>(1,i),DB
						#if defined(STRING_GRAPH_DEBUG)
						,text
						#endif
					);
				}
			};
			std::ostream & operator<<(std::ostream & out, StringGraph const & S);
		}
	}
}

inline std::ostream & libmaus2::dazzler::stringgraph::operator<<(std::ostream & out, libmaus2::dazzler::stringgraph::StringGraph const & S)
{
	out << "StringGraph(\n";
	for ( uint64_t i = 0; i < S.edges.size(); ++i )
	{
		out << "\tedges[" << i << "]=" << S.edges[i] << "\n";
	}
	out << ")\n";
	return out;
}
#endif
