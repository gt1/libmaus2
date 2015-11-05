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

#if ! defined(EDGELISTLOCKED_HPP)
#define EDGELISTLOCKED_HPP

#include <libmaus2/graph/EdgeList.hpp>
#include <libmaus2/graph/EdgeListLocalLockedFlush.hpp>

namespace libmaus2
{
	namespace graph
	{
		struct EdgeListLocked
		{
			typedef EdgeListLocked this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			typedef ::libmaus2::graph::TripleEdge edge_type;

			EdgeList EL;
			EdgeListLocalLockedFlush FL;

			EdgeListLocked(uint64_t const redgelow, uint64_t const redgehigh, uint64_t const rmaxedges)
			: EL(redgelow,redgehigh,rmaxedges), FL(EL)
			{

			}
		};
	}
}
#endif
