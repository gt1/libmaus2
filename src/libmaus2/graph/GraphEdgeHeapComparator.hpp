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
#if !defined(LIBMAUS2_GRAPH_GRAPHEDGEHEAPCOMPARATOR_HPP)
#define LIBMAUS2_GRAPH_GRAPHEDGEHEAPCOMPARATOR_HPP

#include <libmaus2/graph/GraphEdge.hpp>
#include <map>

namespace libmaus2
{
	namespace graph
	{
		struct GraphEdgeHeapComparator
		{
			bool operator()(GraphEdge const & a, GraphEdge const & b)
			{
				return a > b;
			}

			bool operator()(
				std::pair<uint64_t,GraphEdge> const & a,
				std::pair<uint64_t,GraphEdge> const & b
			)
			{
				if ( a.second > b.second )
					return true;
				else if ( a.second < b.second )
					return false;
				else
					return a.first > b.first;
			}
		};
	}
}
#endif
