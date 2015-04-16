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
#include <libmaus/graph/EdgeList.hpp>

unsigned int const libmaus::graph::EdgeListBase::maxweight = ::libmaus::math::MetaLowBits<8*sizeof(edge_weight_type)>::lowbits;
libmaus::graph::EdgeListBase::edge_target_type const libmaus::graph::EdgeListBase::edge_list_term 
	= std::numeric_limits<libmaus::graph::EdgeListBase::edge_target_type>::max(); // 0xFFFFFFFFUL;
