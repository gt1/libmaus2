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
#if ! defined(LIBMAUS_LZ_BGZFINFLATEPARALLEL_HPP)
#define LIBMAUS_LZ_BGZFINFLATEPARALLEL_HPP

#include <libmaus/lz/BgzfInflateBlock.hpp>
#include <libmaus/parallel/TerminatableSynchronousQueue.hpp>
#include <libmaus/parallel/TerminatableSynchronousHeap.hpp>
#include <libmaus/parallel/PosixThread.hpp>
#include <libmaus/parallel/OMPNumThreadsScope.hpp>
#include <libmaus/lz/BgzfInflateBlockIdComparator.hpp>
#include <libmaus/lz/BgzfInflateBlockIdInfo.hpp>
#include <libmaus/lz/BgzfInflateParallelContext.hpp>
#include <libmaus/lz/BgzfInflateParallelThread.hpp>

namespace libmaus
{
	namespace lz
	{
		struct BgzfInflateParallel
		{
			
			BgzfInflateParallelContext inflatecontext;

			libmaus::autoarray::AutoArray<BgzfInflateParallelThread::unique_ptr_type> T;
			
			void init()
			{

				for ( uint64_t i = 0; i < T.size(); ++i )
				{
					T[i] = UNIQUE_PTR_MOVE(BgzfInflateParallelThread::unique_ptr_type(new BgzfInflateParallelThread(inflatecontext)));
					T[i]->start();
				}			
			}
			
			BgzfInflateParallel(std::istream & rinflatein, uint64_t const rnumthreads, uint64_t const rnumblocks)
			: 
				inflatecontext(rinflatein,rnumblocks),
				T(rnumthreads)
			{
				init();
			}

			BgzfInflateParallel(std::istream & rinflatein, std::ostream & rcopyostr, uint64_t const rnumthreads, uint64_t const rnumblocks)
			: 
				inflatecontext(rinflatein,rnumblocks,rcopyostr),
				T(rnumthreads)
			{
				init();
			}

			BgzfInflateParallel(
				std::istream & rinflatein, 
				uint64_t const rnumthreads = 
					std::max(std::max(libmaus::parallel::OMPNumThreadsScope::getMaxThreads(),static_cast<uint64_t>(1))-1,
						static_cast<uint64_t>(1))
			)
			: 
				inflatecontext(rinflatein,4*rnumthreads),
				T(rnumthreads)
			{
				init();
			}

			BgzfInflateParallel(
				std::istream & rinflatein, 
				std::ostream & rcopyostr,
				uint64_t const rnumthreads = 
					std::max(std::max(libmaus::parallel::OMPNumThreadsScope::getMaxThreads(),static_cast<uint64_t>(1))-1,
						static_cast<uint64_t>(1))
			)
			: 
				inflatecontext(rinflatein,4*rnumthreads,rcopyostr),
				T(rnumthreads)
			{
				init();
			}
			
			uint64_t gcount() const
			{
				return inflatecontext.inflategcnt;
			}
			
			uint64_t read(char * const data, uint64_t const n)
			{
				inflatecontext.inflategcnt = 0;
			
				if ( n < libmaus::lz::BgzfInflateBlock::maxblocksize )
				{
					libmaus::exception::LibMausException se;
					se.getStream() << "BgzfInflateParallel::read(): buffer provided is too small: " << n << " < " << libmaus::lz::BgzfInflateBlock::maxblocksize << std::endl;
					se.finish();
					throw se;
				}
			
				/* get object id */
				BgzfThreadQueueElement const btqe = inflatecontext.inflatedecompressedlist.deque();
				uint64_t objectid = btqe.objectid;
				
				/* we have an exception, terminate readers and throw it at caller */
				if ( inflatecontext.inflateB[objectid]->failed() )
				{
					libmaus::parallel::ScopePosixMutex Q(inflatecontext.inflateqlock);
					inflatecontext.inflategloblist.terminate();
					throw inflatecontext.inflateB[objectid]->getException();
				}
				/* we have what we want */
				else
				{
					assert ( inflatecontext.inflateB[objectid]->blockid == inflatecontext.inflateeb );
				}

				uint64_t const blocksize = inflatecontext.inflateB[objectid]->blockinfo.second;
				uint64_t ret = 0;
				
				/* empty block (EOF) */
				if ( ! blocksize )
				{
					libmaus::parallel::ScopePosixMutex Q(inflatecontext.inflateqlock);
					inflatecontext.inflategloblist.terminate();
					
					ret = 0;
				}
				/* block contains data */
				else
				{
					uint64_t const blockid = inflatecontext.inflateB[objectid]->blockid;
					assert ( blockid == inflatecontext.inflateeb );
					inflatecontext.inflateeb += 1;

					std::copy(inflatecontext.inflateB[objectid]->data.begin(), inflatecontext.inflateB[objectid]->data.begin()+blocksize, reinterpret_cast<uint8_t *>(data));

					libmaus::parallel::ScopePosixMutex Q(inflatecontext.inflateqlock);
					inflatecontext.inflatefreelist.push_back(objectid);
					// read next block
					inflatecontext.inflategloblist.enque(
						BgzfThreadQueueElement(
							libmaus::lz::BgzfThreadOpBase::libmaus_lz_bgzf_op_read_block,
							objectid,
							0
						)
					);
					
					inflatecontext.inflategcnt = blocksize;
					
					ret = blocksize;
					
					inflatecontext.inflatedecompressedlist.setReadyFor(
						BgzfThreadQueueElement(
							libmaus::lz::BgzfThreadOpBase::libmaus_lz_bgzf_op_none,
							0,
							blockid+1
						)
					);
				}
				
				return ret;
			}
		};
	}
}
#endif
