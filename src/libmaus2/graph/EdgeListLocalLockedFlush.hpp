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

#if ! defined(EDGELISTLOCALLOCKEDFLUSH_HPP)
#define EDGELISTLOCALLOCKEDFLUSH_HPP

#include <libmaus2/graph/EdgeList.hpp>

namespace libmaus2
{
	namespace graph
	{
		struct EdgeListLocalLockedFlush
		{
			typedef EdgeListLocalLockedFlush this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
		
			typedef ::libmaus2::graph::TripleEdge edge_type;
			
			EdgeList & EL;
			::libmaus2::parallel::OMPLock lock;
			
			EdgeListLocalLockedFlush(EdgeList & rEL)
			: EL(rEL)
			{
			}
			
			void operator()(edge_type const * pa, uint64_t const n)
			{
				lock.lock();
				EL(pa,n);
				lock.unlock();
			}
		};
	}
}
#endif
