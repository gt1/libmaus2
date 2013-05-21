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
#if ! defined(LIBMAUS_LZ_BGZFINFLATEPARALLEL_HPP)
#define LIBMAUS_LZ_BGZFINFLATEPARALLEL_HPP

#include <libmaus/lz/BgzfInflateBlock.hpp>
#include <libmaus/parallel/TerminatableSynchronousQueue.hpp>
#include <libmaus/parallel/TerminatableSynchronousHeap.hpp>
#include <libmaus/parallel/PosixThread.hpp>
#include <libmaus/parallel/OMPNumThreadsScope.hpp>

namespace libmaus
{
	namespace lz
	{
		struct BgzfInflateBlockIdInfo
		{
			libmaus::autoarray::AutoArray<libmaus::lz::BgzfInflateBlock::unique_ptr_type> const & inflateB;
					
			BgzfInflateBlockIdInfo(
				libmaus::autoarray::AutoArray<libmaus::lz::BgzfInflateBlock::unique_ptr_type> const & rinflateB
			) : inflateB(rinflateB)
			{
			
			}

			uint64_t operator()(uint64_t const i) const
			{
				return inflateB[i]->blockid;
			}		
		};

		struct BgzfInflateBlockIdComparator
		{
			libmaus::autoarray::AutoArray<libmaus::lz::BgzfInflateBlock::unique_ptr_type> const & inflateB;
					
			BgzfInflateBlockIdComparator(
				libmaus::autoarray::AutoArray<libmaus::lz::BgzfInflateBlock::unique_ptr_type> const & rinflateB
			) : inflateB(rinflateB)
			{
			
			}

			bool operator()(uint64_t const i, uint64_t const j) const
			{
				return inflateB[i]->blockid > inflateB[j]->blockid;
			}		
		};
		
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

		struct BgzfInflateParallelThread : public libmaus::parallel::PosixThread
		{
			typedef BgzfInflateParallelThread this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			
			enum op_type { op_read_block, op_decompress_block, op_none };
		
			BgzfInflateParallelContext & inflatecontext;
			
			BgzfInflateParallelThread(BgzfInflateParallelContext & rinflatecontext) 
			: inflatecontext(rinflatecontext)
			{
			}
			~BgzfInflateParallelThread()
			{
			}
		
			void * run()
			{
				while ( true )
				{
					/* get any id from global list */
					try
					{
						inflatecontext.inflategloblist.deque();
					}
					catch(std::exception const & ex)
					{
						/* queue is terminated, break loop */
						break;
					}
					
					/* check which operation we are to perform */
					op_type op = op_none;
					uint64_t objectid = 0;
					
					{
						libmaus::parallel::ScopePosixMutex S(inflatecontext.inflateqlock);
						
						if ( inflatecontext.inflatefreelist.size() )
						{
						
							op = op_read_block;
							objectid = inflatecontext.inflatefreelist.front();
							inflatecontext.inflatefreelist.pop_front();
						}
						else if ( inflatecontext.inflatereadlist.size() )
						{
							op = op_decompress_block;
							objectid = inflatecontext.inflatereadlist.front();
							inflatecontext.inflatereadlist.pop_front();
						}
					}
					
					/*   */
					switch ( op )
					{
						/* read a block */
						case op_read_block:
						{
							libmaus::parallel::ScopePosixMutex I(inflatecontext.inflateinlock);
							inflatecontext.inflateB[objectid]->blockid = inflatecontext.inflateinid++;
							inflatecontext.inflateB[objectid]->readBlock(inflatecontext.inflatein);

							libmaus::parallel::ScopePosixMutex Q(inflatecontext.inflateqlock);
							inflatecontext.inflatereadlist.push_back(objectid);
							inflatecontext.inflategloblist.enque(objectid);
							break;
						}
						/* decompress block */
						case op_decompress_block:
						{
							inflatecontext.inflateB[objectid]->decompressBlock();
							libmaus::parallel::ScopePosixMutex Q(inflatecontext.inflateqlock);
							inflatecontext.inflatedecompressedlist.enque(objectid);
							break;
						}
						default:
							break;
					}
				}
				
				return 0;		
			}
		};

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
			
				std::vector < uint64_t > putback;
				uint64_t objectid = 0;
				
				/* get object ids until we have the next expected one, then put out of order entries back */
				while ( true )
				{
					objectid = inflatecontext.inflatedecompressedlist.deque();
					
					/* we have an exception, terminate readers and throw it at caller */
					if ( inflatecontext.inflateB[objectid]->failed() )
					{
						libmaus::parallel::ScopePosixMutex Q(inflatecontext.inflateqlock);
						inflatecontext.inflategloblist.terminate();
						throw inflatecontext.inflateB[objectid]->getException();
					}
					/* the obtained id is not the next expected one, register it for requeuing */
					else if ( inflatecontext.inflateB[objectid]->blockid != inflatecontext.inflateeb )
						putback.push_back(objectid);
					/* we have what we wanted, break loop */
					else
						break;
				}
				
				/* put unwanted ids back */
				for ( uint64_t i = 0; i < putback.size(); ++i )
					inflatecontext.inflatedecompressedlist.enque(putback[i]);

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
					inflatecontext.inflategloblist.enque(objectid);
					
					inflatecontext.inflategcnt = blocksize;
					
					ret = blocksize;
					
					inflatecontext.inflatedecompressedlist.setReadyFor(blockid+1);
				}
				
				return ret;
			}
		};
	}
}
#endif
