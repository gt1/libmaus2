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
				
				StringGraph() {}
				StringGraph(std::istream & in)
				{
					while ( in.peek() != std::istream::traits_type::eof() )
						edges.push_back(Edge(in));
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
