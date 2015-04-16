/*
    libmaus
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
#if ! defined(LIBMAUS_GRAPH_FILTEREDGRAPHTYPES_HPP)
#define LIBMAUS_GRAPH_FILTEREDGRAPHTYPES_HPP

#include <libmaus/types/types.hpp>
#include <libmaus/graph/EdgeList.hpp>

namespace libmaus
{
	namespace graph
	{
		struct FilteredGraphTypes
		{
			typedef libmaus::graph::EdgeListBase::edge_target_type filtered_graph_target_type;
			typedef libmaus::graph::EdgeListBase::edge_count_type filtered_graph_num_edge_type;
			typedef uint8_t  filtered_graph_orientation_type;
			typedef libmaus::graph::EdgeListBase::edge_weight_type filtered_graph_weight_type;
		};
	}
}
#endif
