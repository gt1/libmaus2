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
#if ! defined(LIBMAUS_LZ_BGZFINFLATEPARALLELCONTEXT_HPP)
#define LIBMAUS_LZ_BGZFINFLATEPARALLELCONTEXT_HPP

#include <libmaus/lz/BgzfInflateBlock.hpp>
#include <libmaus/parallel/TerminatableSynchronousQueue.hpp>
#include <libmaus/parallel/TerminatableSynchronousHeap.hpp>
#include <libmaus/parallel/PosixThread.hpp>
#include <libmaus/parallel/OMPNumThreadsScope.hpp>
#include <libmaus/lz/BgzfInflateBlockIdComparator.hpp>
#include <libmaus/lz/BgzfInflateBlockIdInfo.hpp>

namespace libmaus
{
	namespace lz
	{
		struct BgzfInflateParallelContext
		{
			libmaus::parallel::TerminatableSynchronousQueue<uint64_t> inflategloblist;

			std::istream & inflatein;
			uint64_t inflateinid;
			libmaus::parallel::PosixMutex inflateinlock;
			
			libmaus::parallel::PosixMutex inflateqlock;

			libmaus::autoarray::AutoArray<libmaus::lz::BgzfInflateBlock::unique_ptr_type> inflateB;
			
			std::deque<uint64_t> inflatefreelist;
			std::deque<uint64_t> inflatereadlist;
			BgzfInflateBlockIdComparator inflateheapcomp;
			BgzfInflateBlockIdInfo inflateheapinfo;
			libmaus::parallel::SynchronousConsecutiveHeap<uint64_t,BgzfInflateBlockIdInfo,BgzfInflateBlockIdComparator> inflatedecompressedlist;
			
			uint64_t inflateeb;
			uint64_t inflategcnt;
		
			BgzfInflateParallelContext(std::istream & rinflatein, uint64_t const rnumblocks)
			:
				inflatein(rinflatein), inflateinid(0), inflateB(rnumblocks), 
				inflateheapcomp(inflateB), inflateheapinfo(inflateB), 
				inflatedecompressedlist(inflateheapcomp,inflateheapinfo),
				inflateeb(0), inflategcnt(0)
			{
				for ( uint64_t i = 0; i < inflateB.size(); ++i )
				{
					inflateB[i] = UNIQUE_PTR_MOVE(libmaus::lz::BgzfInflateBlock::unique_ptr_type(new libmaus::lz::BgzfInflateBlock(i)));
					inflatefreelist.push_back(i);
				}			

				for ( uint64_t i = 0; i < inflateB.size(); ++i )
					inflategloblist.enque(i);
			}
		};
	}
}
#endif
