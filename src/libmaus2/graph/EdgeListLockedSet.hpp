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

#if ! defined(EDGELISTLOCKEDSET_HPP)
#define EDGELISTLOCKEDSET_HPP

#include <libmaus2/graph/EdgeListLocked.hpp>

namespace libmaus2
{
	namespace graph
	{
		struct EdgeListLockedSet
		{
			typedef EdgeListLockedSet this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			uint64_t const lowsrc;
			uint64_t const highsrc;
			uint64_t const numreads;
			uint64_t const readsperlist;
			uint64_t const numlists;

			typedef EdgeListLocked list_type;
			typedef list_type::unique_ptr_type list_ptr_type;
			::libmaus2::autoarray::AutoArray<list_ptr_type> edgelists;

			EdgeListLockedSet(
				uint64_t const rlowsrc,
				uint64_t const rhighsrc,
				uint64_t const rreadsperlist,
				uint64_t const maxedges
			)
			: lowsrc(rlowsrc), highsrc(rhighsrc), numreads(highsrc-lowsrc), readsperlist(rreadsperlist),
			  numlists( (numreads+readsperlist-1)/readsperlist ),
			  edgelists(numlists)
			{
				for ( uint64_t i = 0; i < numlists; ++i )
				{
					uint64_t const low = lowsrc + i*readsperlist;
					uint64_t const high = std::min(low+readsperlist,highsrc);
					edgelists[i] =  list_ptr_type ( new list_type(low,high,maxedges) );
				}
			}
		};
	}
}
#endif
