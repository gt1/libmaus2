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
#if ! defined(LIBMAUS_LZ_BGZFINFLATEPARALLEL_HPP)
#define LIBMAUS_LZ_BGZFINFLATEPARALLEL_HPP

#include <libmaus2/lz/BgzfInflateBlock.hpp>
#include <libmaus2/parallel/TerminatableSynchronousQueue.hpp>
#include <libmaus2/parallel/TerminatableSynchronousHeap.hpp>
#include <libmaus2/parallel/PosixThread.hpp>
#include <libmaus2/parallel/OMPNumThreadsScope.hpp>
#include <libmaus2/lz/BgzfInflateBlockIdComparator.hpp>
#include <libmaus2/lz/BgzfInflateBlockIdInfo.hpp>
#include <libmaus2/lz/BgzfInflateParallelContext.hpp>
#include <libmaus2/lz/BgzfInflateParallelThread.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct BgzfInflateParallel
		{
			private:
			libmaus2::parallel::TerminatableSynchronousHeap<BgzfThreadQueueElement,BgzfThreadQueueElementHeapComparator> inflategloblist;
			BgzfInflateParallelContext inflatecontext;

			libmaus2::autoarray::AutoArray<BgzfInflateParallelThread::unique_ptr_type> T;
			
			bool terminated;
			
			void init()
			{

				for ( uint64_t i = 0; i < T.size(); ++i )
				{
					BgzfInflateParallelThread::unique_ptr_type tTi(new BgzfInflateParallelThread(inflatecontext));
					T[i] = UNIQUE_PTR_MOVE(tTi);
					T[i]->start();
				}			
			}
			
			public:
			BgzfInflateParallel(std::istream & rinflatein, uint64_t const rnumthreads, uint64_t const rnumblocks)
			: 
				inflategloblist(),
				inflatecontext(inflategloblist,rinflatein,rnumblocks),
				T(rnumthreads),
				terminated(false)
			{
				init();
			}

			BgzfInflateParallel(std::istream & rinflatein, std::ostream & rcopyostr, uint64_t const rnumthreads, uint64_t const rnumblocks)
			: 
				inflatecontext(inflategloblist,rinflatein,rnumblocks,rcopyostr),
				T(rnumthreads),
				terminated(false)
			{
				init();
			}

			BgzfInflateParallel(
				std::istream & rinflatein, 
				uint64_t const rnumthreads = 
					std::max(std::max(libmaus2::parallel::OMPNumThreadsScope::getMaxThreads(),static_cast<uint64_t>(1))-1,
						static_cast<uint64_t>(1))
			)
			: 
				inflatecontext(inflategloblist,rinflatein,4*rnumthreads),
				T(rnumthreads),
				terminated(false)
			{
				init();
			}

			BgzfInflateParallel(
				std::istream & rinflatein, 
				std::ostream & rcopyostr,
				uint64_t const rnumthreads = 
					std::max(std::max(libmaus2::parallel::OMPNumThreadsScope::getMaxThreads(),static_cast<uint64_t>(1))-1,
						static_cast<uint64_t>(1))
			)
			: 
				inflatecontext(inflategloblist,rinflatein,4*rnumthreads,rcopyostr),
				T(rnumthreads),
				terminated(false)
			{
				init();
			}
			
			uint64_t gcount() const
			{
				return inflatecontext.inflategcnt;
			}

			BgzfInflateInfo readAndInfo(char * const data, uint64_t const n)
			{
				inflatecontext.inflategcnt = 0;
					
				if ( n < libmaus2::lz::BgzfInflateBlock::getBgzfMaxBlockSize() )
				{
					libmaus2::exception::LibMausException se;
					se.getStream() << "BgzfInflateParallel::read(): buffer provided is too small: " << n << " < " << libmaus2::lz::BgzfInflateBlock::getBgzfMaxBlockSize() << std::endl;
					se.finish();
					throw se;
				}

				if ( terminated )
				{
					return BgzfInflateInfo(0,0,true);
				}
			
				/* get object id */
				BgzfThreadQueueElement const btqe = inflatecontext.inflatedecompressedlist.deque();
				uint64_t objectid = btqe.objectid;
				
				/* we have an exception, terminate readers and throw it at caller */
				if ( inflatecontext.inflateB[objectid]->failed() )
				{
					libmaus2::parallel::ScopePosixMutex Q(inflatecontext.inflateqlock);
					inflatecontext.inflategloblist.terminate();
					terminated = true;
					throw inflatecontext.inflateB[objectid]->getException();
				}
				/* we have what we want */
				else
				{
					assert ( inflatecontext.inflateB[objectid]->blockid == inflatecontext.inflateeb );
				}

				BgzfInflateInfo const info = inflatecontext.inflateB[objectid]->blockinfo;
				
				/* empty block (EOF) */
				if ( 
					! info.uncompressed
					&&
					inflatecontext.inflateB[objectid]->blockinfo.streameof
				)
				{
					libmaus2::parallel::ScopePosixMutex Q(inflatecontext.inflateqlock);
					inflatecontext.inflategloblist.terminate();
					terminated = true;
					
					return info;
				}
				/* block contains data */
				else
				{
					uint64_t const blockid = inflatecontext.inflateB[objectid]->blockid;
					assert ( blockid == inflatecontext.inflateeb );
					inflatecontext.inflateeb += 1;

					std::copy(inflatecontext.inflateB[objectid]->data.begin(), inflatecontext.inflateB[objectid]->data.begin()+info.uncompressed, reinterpret_cast<uint8_t *>(data));

					libmaus2::parallel::ScopePosixMutex Q(inflatecontext.inflateqlock);
					inflatecontext.inflatefreelist.push_back(objectid);
					// read next block
					inflatecontext.inflategloblist.enque(
						BgzfThreadQueueElement(
							libmaus2::lz::BgzfThreadOpBase::libmaus2_lz_bgzf_op_read_block,
							objectid,
							0
						)
					);
					
					inflatecontext.inflategcnt = info.uncompressed;
					
					inflatecontext.inflatedecompressedlist.setReadyFor(
						BgzfThreadQueueElement(
							libmaus2::lz::BgzfThreadOpBase::libmaus2_lz_bgzf_op_none,
							0,
							blockid+1
						)
					);
					
					return info;
				}				
			}
			
			uint64_t read(char * const data, uint64_t const n)
			{
				return readAndInfo(data,n).uncompressed;
			}
		};
	}
}
#endif
