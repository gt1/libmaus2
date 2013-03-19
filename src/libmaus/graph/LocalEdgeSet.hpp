/**
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
**/

#if ! defined(LOCALEDGESET_HPP)
#define LOCALEDGESET_HPP

#include <libmaus/clustering/EdgeList.hpp>
#include <libmaus/clustering/EdgeBufferLocal.hpp>

namespace libmaus
{
	namespace graph
	{
		struct LocalEdgeSet
		{
			uint64_t const srclow;
			uint64_t const srchigh;
			uint64_t const numreads;
			uint64_t const readsperlist;
			uint64_t const numlists;
			
			::libmaus::autoarray::AutoArray < EdgeList::unique_ptr_type > edgelists;
			::libmaus::autoarray::AutoArray < EdgeBufferLocal::unique_ptr_type > edgebuffers;
			
			LocalEdgeSet(
				uint64_t const rsrclow,
				uint64_t const rsrchigh,
				uint64_t const rreadsperlist,
				uint64_t const maxedges,
				uint64_t const bufsize
			)
			: srclow(rsrclow), srchigh(rsrchigh), numreads(srchigh-srclow), 
			  readsperlist(rreadsperlist), 
			  numlists( (numreads+readsperlist-1)/readsperlist ),
			  edgelists(numlists),
			  edgebuffers(numlists)
			{
				for ( uint64_t i = 0; i < numlists; ++i )
				{
					uint64_t const low = srclow + i*readsperlist;
					uint64_t const high = std::min(low+readsperlist,srchigh);
					
					edgelists[i] =  EdgeList::unique_ptr_type ( new EdgeList(low,high,maxedges) );
					
					edgebuffers[i] =  EdgeBufferLocal::unique_ptr_type( 
						new EdgeBufferLocal(*edgelists[i], bufsize) 
					);
				}
			}
			
			void operator()(::libmaus::graph::TripleEdge const & T)
			{
				if ( T.a >= srclow && T.a < srchigh )
					edgebuffers [ (T.a-srclow) / readsperlist ]->put(T);
			}
			
			void flush()
			{
				for ( uint64_t i = 0; i < edgebuffers.size(); ++i )
					edgebuffers[i]->flush();
			}
			
			void print(std::ostream & out) const
			{
				for ( uint64_t i = 0; i < numlists; ++i )
					edgelists[i]->print(out);
			}
		};
	}
}
#endif
