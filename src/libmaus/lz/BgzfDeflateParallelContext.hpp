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
#if ! defined(LIBMAUS_LZ_BGZFDEFLATEPARALLELCONTEXT_HPP)
#define LIBMAUS_LZ_BGZFDEFLATEPARALLELCONTEXT_HPP

#include <libmaus/lz/BgzfDeflateBase.hpp>
#include <libmaus/parallel/TerminatableSynchronousQueue.hpp>
#include <libmaus/parallel/TerminatableSynchronousHeap.hpp>
#include <libmaus/parallel/PosixThread.hpp>
#include <libmaus/parallel/OMPNumThreadsScope.hpp>
#include <libmaus/lz/BgzfDeflateBlockIdInfo.hpp>
#include <libmaus/lz/BgzfDeflateBlockIdComparator.hpp>

namespace libmaus
{
	namespace lz
	{
		struct BgzfDeflateParallelContext
		{
			typedef BgzfDeflateParallelContext this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			
			libmaus::parallel::TerminatableSynchronousHeap<BgzfThreadQueueElement,BgzfThreadQueueElementHeapComparator>
				& deflategloblist;

			libmaus::parallel::PosixMutex deflateoutlock;
			// next block id to be filled with input
			uint64_t deflateoutid;
			// next block id to be written to compressed stream
			uint64_t deflatenextwriteid;
			// output stream
			std::ostream & deflateout;
			bool deflateoutflushed;
			// number of output bytes
			uint64_t deflateoutbytes;

			libmaus::autoarray::AutoArray<libmaus::lz::BgzfDeflateBase::unique_ptr_type> deflateB;
			libmaus::parallel::SynchronousQueue<uint64_t> deflatefreelist;

			//! current object handled by input/caller
			int64_t deflatecurobject;

			// queue for blocks to be compressed
			std::deque<uint64_t> deflatecompqueue;
			// queue for compressed blocks to be written to output stream
			BgzfDeflateBlockIdComparator deflateheapcomp;
			BgzfDeflateBlockIdInfo deflateheapinfo;
			libmaus::parallel::SynchronousConsecutiveHeap<
				BgzfThreadQueueElement,
				BgzfDeflateBlockIdInfo,
				BgzfDeflateBlockIdComparator
			> deflatewritequeue;
			
			// exception data
			uint64_t deflateexceptionid;
			libmaus::exception::LibMausException::unique_ptr_type deflatepse;
			libmaus::parallel::PosixMutex deflateexlock;

			//! queues lock
			libmaus::parallel::PosixMutex deflateqlock;
			
			//! index stream
			std::ostream * deflateindexstr;
			
			static bool getDefaultDeflateGetCur()
			{
				return true;
			}

			BgzfDeflateParallelContext(
				libmaus::parallel::TerminatableSynchronousHeap<
					BgzfThreadQueueElement,
					BgzfThreadQueueElementHeapComparator
				>
				& rdeflategloblist,		
				std::ostream & rdeflateout, 
				uint64_t const rnumbuffers,
				int level,
				bool const deflategetcur = getDefaultDeflateGetCur(),
				std::ostream * rdeflateindexstr = 0
			)
			: deflategloblist(rdeflategloblist), 
			  deflateoutid(0), deflatenextwriteid(0), deflateout(rdeflateout), deflateoutflushed(false), 
			  deflateoutbytes(0),
			  deflateB(rnumbuffers),
			  deflatecurobject(-1),
			  deflateheapcomp(deflateB), 
			  deflateheapinfo(deflateB),
			  deflatewritequeue(deflateheapcomp,deflateheapinfo),
			  deflateexceptionid(std::numeric_limits<uint64_t>::max()),
			  deflateindexstr(rdeflateindexstr)
			{
				for ( uint64_t i = 0; i < deflateB.size(); ++i )
				{
					libmaus::lz::BgzfDeflateBase::unique_ptr_type tdeflateBi(
                                                new libmaus::lz::BgzfDeflateBase(level,true)
                                        );
					deflateB[i] = UNIQUE_PTR_MOVE(tdeflateBi);
					// completely empty buffer on flush
					deflateB[i]->flushmode = true;
					deflateB[i]->objectid = i;
					deflatefreelist.enque(i);
				}
				
				if ( deflategetcur )
				{
					deflatecurobject = deflatefreelist.deque();
					deflateB[deflatecurobject]->blockid = deflateoutid++;
				}
			}
			~BgzfDeflateParallelContext()
			{
				if ( deflateindexstr )
					deflateindexstr->flush();
			}
		};
	}
}
#endif
