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
#if ! defined(LIBMAUS2_DAZZLER_STRINGGRAPH_EDGE_HPP)
#define LIBMAUS2_DAZZLER_STRINGGRAPH_EDGE_HPP

#include <libmaus2/dazzler/stringgraph/OverlapNode.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace stringgraph
		{
			struct Edge
			{
				typedef Edge this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

				OverlapNode from;
				OverlapNode to;
				uint64_t length;
				std::vector < OverlapNode > overlaps;

				Edge() {}
				Edge(std::istream & in)
				: from(in), to(in),
				  length(libmaus2::util::NumberSerialisation::deserialiseNumber(in,4))
				{
					uint64_t const numoverlaps = libmaus2::util::NumberSerialisation::deserialiseNumber(in,4);
					for ( uint64_t i = 0; i < numoverlaps; ++i )
						overlaps.push_back(OverlapNode(in));
				}

				bool operator<(Edge const & E) const
				{
					if ( from != E.from )
						return from < E.from;
					else if ( to != E.to )
						return to < E.to;
					else
						return false;
				}

				bool operator==(Edge const & E) const
				{
					return from == E.from && to == E.to;
				}

				bool operator!=(Edge const & O) const
				{
					return !operator==(O);
				}
			};
			std::ostream & operator<<(std::ostream & out, Edge const & E);
		}
	}
}

inline std::ostream & libmaus2::dazzler::stringgraph::operator<<(std::ostream & out, libmaus2::dazzler::stringgraph::Edge const & E)
{
	out << "Edge(";
	out << "from=" << E.from;
	out << ",";
	out << "to=" << E.to;
	out << ",";
	out << "length=" << E.length;
	out << ",";
	out << "overlaps=[";
	for ( uint64_t i = 0; i < E.overlaps.size(); ++i )
		out << (i?",":"") << E.overlaps[i];
	out << "]";
	out << ")";
	return out;
}
#endif
