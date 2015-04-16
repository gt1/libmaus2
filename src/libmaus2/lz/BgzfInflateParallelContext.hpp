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
#if ! defined(LIBMAUS_LZ_BGZFINFLATEPARALLELCONTEXT_HPP)
#define LIBMAUS_LZ_BGZFINFLATEPARALLELCONTEXT_HPP

#include <libmaus2/lz/BgzfInflateBlock.hpp>
#include <libmaus2/parallel/TerminatableSynchronousQueue.hpp>
#include <libmaus2/parallel/TerminatableSynchronousHeap.hpp>
#include <libmaus2/parallel/PosixThread.hpp>
#include <libmaus2/parallel/OMPNumThreadsScope.hpp>
#include <libmaus2/lz/BgzfInflateBlockIdComparator.hpp>
#include <libmaus2/lz/BgzfInflateBlockIdInfo.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct BgzfInflateParallelContext
		{
			libmaus2::parallel::TerminatableSynchronousHeap<BgzfThreadQueueElement,BgzfThreadQueueElementHeapComparator>
				& inflategloblist;

			std::istream & inflatein;
			uint64_t inflateinid;
			libmaus2::parallel::PosixMutex inflateinlock;
			
			libmaus2::parallel::PosixMutex inflateqlock;

			libmaus2::autoarray::AutoArray<libmaus2::lz::BgzfInflateBlock::unique_ptr_type> inflateB;
			
			std::deque<uint64_t> inflatefreelist;
			std::deque<uint64_t> inflatereadlist;
			BgzfInflateBlockIdComparator inflateheapcomp;
			BgzfInflateBlockIdInfo inflateheapinfo;
			libmaus2::parallel::SynchronousConsecutiveHeap<
				BgzfThreadQueueElement,
				BgzfInflateBlockIdInfo,
				BgzfInflateBlockIdComparator
			>
				inflatedecompressedlist;
			
			uint64_t inflateeb;
			uint64_t inflategcnt;
			
			std::ostream * copyostr;
			
			void init()
			{
				for ( uint64_t i = 0; i < inflateB.size(); ++i )
				{
					libmaus2::lz::BgzfInflateBlock::unique_ptr_type tinflateB(new libmaus2::lz::BgzfInflateBlock(i));
					inflateB[i] = UNIQUE_PTR_MOVE(tinflateB);
					inflatefreelist.push_back(i);
				}			

				for ( uint64_t i = 0; i < inflateB.size(); ++i )
					inflategloblist.enque(
						BgzfThreadQueueElement(
					        	libmaus2::lz::BgzfThreadOpBase::libmaus2_lz_bgzf_op_read_block,
					        	i,
					        	0
						)
					);
			}
		
			BgzfInflateParallelContext(
				libmaus2::parallel::TerminatableSynchronousHeap<BgzfThreadQueueElement,BgzfThreadQueueElementHeapComparator>
					& rinflategloblist,
				std::istream & rinflatein, uint64_t const rnumblocks
			)
			:
				inflategloblist(rinflategloblist),
				inflatein(rinflatein), inflateinid(0), inflateB(rnumblocks), 
				inflateheapcomp(inflateB), inflateheapinfo(inflateB), 
				inflatedecompressedlist(inflateheapcomp,inflateheapinfo),
				inflateeb(0), inflategcnt(0), copyostr(0)
			{
				init();
			}

			BgzfInflateParallelContext(
				libmaus2::parallel::TerminatableSynchronousHeap<BgzfThreadQueueElement,BgzfThreadQueueElementHeapComparator>
					& rinflategloblist,
				std::istream & rinflatein, uint64_t const rnumblocks, std::ostream & rcopyostr
			)
			:
				inflategloblist(rinflategloblist),
				inflatein(rinflatein), inflateinid(0), inflateB(rnumblocks), 
				inflateheapcomp(inflateB), inflateheapinfo(inflateB), 
				inflatedecompressedlist(inflateheapcomp,inflateheapinfo),
				inflateeb(0), inflategcnt(0), copyostr(&rcopyostr)
			{
				init();
			}
		};
	}
}
#endif
