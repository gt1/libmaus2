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
#if ! defined(LIBMAUS_LZ_BGZFINFLATEDEFLATEPARALLEL_HPP)
#define LIBMAUS_LZ_BGZFINFLATEDEFLATEPARALLEL_HPP

#include <libmaus/lz/BgzfInflateBlock.hpp>
#include <libmaus/parallel/TerminatableSynchronousQueue.hpp>
#include <libmaus/parallel/TerminatableSynchronousHeap.hpp>
#include <libmaus/parallel/PosixThread.hpp>
#include <libmaus/parallel/OMPNumThreadsScope.hpp>
#include <libmaus/lz/BgzfInflateBlockIdComparator.hpp>
#include <libmaus/lz/BgzfInflateBlockIdInfo.hpp>
#include <libmaus/lz/BgzfInflateParallelContext.hpp>
#include <libmaus/lz/BgzfDeflateParallelContext.hpp>
#include <libmaus/lz/BgzfInflateDeflateParallelThread.hpp>

namespace libmaus
{
	namespace lz
	{
		struct BgzfInflateDeflateParallel
		{
			typedef BgzfInflateDeflateParallel this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
					
			private:
			libmaus::parallel::TerminatableSynchronousHeap<BgzfThreadQueueElement,BgzfThreadQueueElementHeapComparator> globlist;
			BgzfInflateParallelContext inflatecontext;
			BgzfDeflateParallelContext deflatecontext;

			libmaus::autoarray::AutoArray<BgzfInflateDeflateParallelThread::unique_ptr_type> T;
			
			bool inflateterminated;

			void drain()
			{
				// handle last block
				{
					libmaus::parallel::ScopePosixMutex Q(deflatecontext.deflateqlock);
					if ( deflatecontext.deflateB[deflatecontext.deflatecurobject]->pc != deflatecontext.deflateB[deflatecontext.deflatecurobject]->pa )
					{
						deflatecontext.deflatecompqueue.push_back(deflatecontext.deflatecurobject);
						deflatecontext.deflategloblist.enque(
							BgzfThreadQueueElement(
								BgzfThreadOpBase::libmaus_lz_bgzf_op_compress_block,
								deflatecontext.deflatecurobject,
								0 /* block id */
							)
						);
					}
					else
					{
						deflatecontext.deflatefreelist.enque(deflatecontext.deflatecurobject);
					}
				}
					
				// wait until all threads are idle
				while ( deflatecontext.deflatefreelist.getFillState() < deflatecontext.deflateB.size() )
				{
					// wait for 1/100 s
					// struct timespec waittime = { 0, 10000000 };
					struct timespec waittime = { 1,0 };
					nanosleep(&waittime,0);
				}			
			}

			
			void init()
			{

				for ( uint64_t i = 0; i < T.size(); ++i )
				{
					BgzfInflateDeflateParallelThread::unique_ptr_type tTi(
						new BgzfInflateDeflateParallelThread(inflatecontext,deflatecontext)
					);
					T[i] = UNIQUE_PTR_MOVE(tTi);
					T[i]->start();
				}			
			}
			
			public:
			BgzfInflateDeflateParallel(
				std::istream & rinflatein, 
				std::ostream & rdeflateout,
				int const level,
				uint64_t const rnumthreads, 
				uint64_t const rnumblocks)
			: 
				globlist(2),
				inflatecontext(globlist,rinflatein,rnumblocks),
				deflatecontext(globlist,rdeflateout,rnumblocks,level,BgzfDeflateParallelContext::getDefaultDeflateGetCur()),
				T(rnumthreads),
				inflateterminated(false)
			{
				init();
			}

			BgzfInflateDeflateParallel(
				std::istream & rinflatein, 
				std::ostream & rcopyostr, 
				std::ostream & rdeflateout,
				int const level,
				uint64_t const rnumthreads, 
				uint64_t const rnumblocks)
			: 
				globlist(2),
				inflatecontext(globlist,rinflatein,rnumblocks,rcopyostr),
				deflatecontext(globlist,rdeflateout,rnumblocks,level,BgzfDeflateParallelContext::getDefaultDeflateGetCur()),
				T(rnumthreads),
				inflateterminated(false)
			{
				init();
			}

			BgzfInflateDeflateParallel(
				std::istream & rinflatein, 
				std::ostream & rdeflateout,
				int const level,
				uint64_t const rnumthreads = 
					std::max(std::max(libmaus::parallel::OMPNumThreadsScope::getMaxThreads(),static_cast<uint64_t>(1))-1,
						static_cast<uint64_t>(1))
			)
			: 
				globlist(2),
				inflatecontext(globlist,rinflatein,4*rnumthreads),
				deflatecontext(globlist,rdeflateout,4*rnumthreads,level,BgzfDeflateParallelContext::getDefaultDeflateGetCur()),
				T(rnumthreads),
				inflateterminated(false)
			{
				init();
			}

			BgzfInflateDeflateParallel(
				std::istream & rinflatein, 
				std::ostream & rcopyostr,
				std::ostream & rdeflateout,
				int const level,
				uint64_t const rnumthreads = 
					std::max(std::max(libmaus::parallel::OMPNumThreadsScope::getMaxThreads(),static_cast<uint64_t>(1))-1,
						static_cast<uint64_t>(1))
			)
			: 
				globlist(2),
				inflatecontext(globlist,rinflatein,4*rnumthreads,rcopyostr),
				deflatecontext(globlist,rdeflateout,4*rnumthreads,level,BgzfDeflateParallelContext::getDefaultDeflateGetCur()),
				T(rnumthreads),
				inflateterminated(false)
			{
				init();
			}

			~BgzfInflateDeflateParallel()
			{
				flush();
			}

			void registerBlockOutputCallback(::libmaus::lz::BgzfDeflateOutputCallback * cb)
			{
				deflatecontext.blockoutputcallbacks.push_back(cb);
			}

			uint64_t gcount() const
			{
				return inflatecontext.inflategcnt;
			}
			
			uint64_t read(char * const data, uint64_t const n)
			{
				inflatecontext.inflategcnt = 0;
					
				if ( n < libmaus::lz::BgzfInflateBlock::getBgzfMaxBlockSize() )
				{
					libmaus::exception::LibMausException se;
					se.getStream() << "BgzfInflateParallel::read(): buffer provided is too small: " << n << " < " << libmaus::lz::BgzfInflateBlock::getBgzfMaxBlockSize() << std::endl;
					se.finish();
					throw se;
				}

				if ( inflateterminated )
					return 0;
			
				/* get object id */
				BgzfThreadQueueElement const btqe = inflatecontext.inflatedecompressedlist.deque();
				uint64_t objectid = btqe.objectid;
				
				/* we have an exception, terminate readers and throw it at caller */
				if ( inflatecontext.inflateB[objectid]->failed() )
				{
					libmaus::parallel::ScopePosixMutex Q(inflatecontext.inflateqlock);
					inflatecontext.inflategloblist.terminate();
					inflateterminated = true;
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
					inflateterminated = true;
					
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
			
			void put(uint8_t const c)
			{
				write(reinterpret_cast<char const *>(&c),1);
			}

			void write(char const * c, uint64_t n)
			{
				while ( n )
				{
					uint64_t const freespace = deflatecontext.deflateB[deflatecontext.deflatecurobject]->pe - deflatecontext.deflateB[deflatecontext.deflatecurobject]->pc;
					uint64_t const towrite = std::min(n,freespace);
					std::copy(reinterpret_cast<uint8_t const *>(c),reinterpret_cast<uint8_t const *>(c)+towrite,deflatecontext.deflateB[deflatecontext.deflatecurobject]->pc);

					c += towrite;
					deflatecontext.deflateB[deflatecontext.deflatecurobject]->pc += towrite;
					n -= towrite;

					// if block is now full
					if ( deflatecontext.deflateB[deflatecontext.deflatecurobject]->pc == deflatecontext.deflateB[deflatecontext.deflatecurobject]->pe )
					{
						// check for exceptions on output
						{
							deflatecontext.deflateexlock.lock();

							if ( deflatecontext.deflateexceptionid != std::numeric_limits<uint64_t>::max() )
							{
								deflatecontext.deflateexlock.unlock();
								
								drain();

								libmaus::parallel::ScopePosixMutex Q(deflatecontext.deflateexlock);
								throw (*(deflatecontext.deflatepse));
							}
							else
							{
								deflatecontext.deflateexlock.unlock();
							}
						}
					
						// push data object id into deflate queue
						{
							libmaus::parallel::ScopePosixMutex Q(deflatecontext.deflateqlock);
							deflatecontext.deflatecompqueue.push_back(deflatecontext.deflatecurobject);
						}

						// register task in global todo list
						deflatecontext.deflategloblist.enque(
							BgzfThreadQueueElement(
								BgzfThreadOpBase::libmaus_lz_bgzf_op_compress_block,
								deflatecontext.deflatecurobject,
								0 /* block id */
							)
						);
						
						// get next object
						deflatecontext.deflatecurobject = deflatecontext.deflatefreelist.deque();
						// set block id of next object
						deflatecontext.deflateB[deflatecontext.deflatecurobject]->blockid = deflatecontext.deflateoutid++;
					}
				}
			}
			
			
			void flush()
			{
				if ( ! deflatecontext.deflateoutflushed )
				{
					drain();

					deflatecontext.deflategloblist.terminate();

					{
						deflatecontext.deflateexlock.lock();

						if ( deflatecontext.deflateexceptionid != std::numeric_limits<uint64_t>::max() )
						{
							deflatecontext.deflateexlock.unlock();
								
							libmaus::parallel::ScopePosixMutex Q(deflatecontext.deflateexlock);
							throw (*(deflatecontext.deflatepse));
						}
						else
						{
							deflatecontext.deflateexlock.unlock();
						}
					}

					// write default compressed block with size 0 (EOF marker)
					libmaus::lz::BgzfDeflateBase eofBase;
					BgzfDeflateZStreamBaseFlushInfo const eofflushsize = eofBase.flush(true /* full flush */);
					assert ( ! eofflushsize.movesize );
					deflatecontext.streamWrite(eofBase.inbuf.begin(),eofBase.outbuf.begin(),eofflushsize);
					
					deflatecontext.deflateoutflushed = true;
				}
			}

		};
	}
}
#endif
