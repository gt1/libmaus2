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
#if ! defined(LIBMAUS_LZ_BGZFDEFLATEPARALLEL_HPP)
#define LIBMAUS_LZ_BGZFDEFLATEPARALLEL_HPP

#include <libmaus/lz/BgzfDeflateBase.hpp>
#include <libmaus/parallel/TerminatableSynchronousQueue.hpp>
#include <libmaus/parallel/TerminatableSynchronousHeap.hpp>
#include <libmaus/parallel/PosixThread.hpp>
#include <libmaus/parallel/OMPNumThreadsScope.hpp>

namespace libmaus
{
	namespace lz
	{
		struct BgzfDeflateBlockIdInfo
		{
			libmaus::autoarray::AutoArray<libmaus::lz::BgzfDeflateBase::unique_ptr_type> const & deflateB;
					
			BgzfDeflateBlockIdInfo(libmaus::autoarray::AutoArray<libmaus::lz::BgzfDeflateBase::unique_ptr_type> const & rdeflateB) : deflateB(rdeflateB)
			{
			
			}

			uint64_t operator()(uint64_t const i) const
			{
				return deflateB[i]->blockid;
			}		
		};
	
		struct BgzfDeflateBlockIdComparator
		{
			libmaus::autoarray::AutoArray<libmaus::lz::BgzfDeflateBase::unique_ptr_type> const & deflateB;
					
			BgzfDeflateBlockIdComparator(libmaus::autoarray::AutoArray<libmaus::lz::BgzfDeflateBase::unique_ptr_type> const & rdeflateB) : deflateB(rdeflateB)
			{
			
			}

			bool operator()(uint64_t const i, uint64_t const j) const
			{
				return deflateB[i]->blockid > deflateB[j]->blockid;
			}		
		};

		struct BgzfDeflateParallelContext
		{
			typedef BgzfDeflateParallelContext this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			libmaus::parallel::TerminatableSynchronousQueue<uint64_t> deflategloblist;
		
			libmaus::parallel::PosixMutex deflateoutlock;
			// next block id to be filled with input
			uint64_t deflateoutid;
			// next block id to be written to compressed stream
			uint64_t deflatenextwriteid;
			std::ostream & deflateout;
			bool deflateoutflushed;

			libmaus::autoarray::AutoArray<libmaus::lz::BgzfDeflateBase::unique_ptr_type> deflateB;
			libmaus::parallel::SynchronousQueue<uint64_t> deflatefreelist;
			
			int64_t deflatecurobject;

			// queue for blocks to be compressed
			std::deque<uint64_t> deflatecompqueue;
			// queue for compressed blocks to be written to output stream
			BgzfDeflateBlockIdComparator deflateheapcomp;
			BgzfDeflateBlockIdInfo deflateheapinfo;
			libmaus::parallel::SynchronousConsecutiveHeap<uint64_t,BgzfDeflateBlockIdInfo,BgzfDeflateBlockIdComparator> deflatewritequeue;
			
			uint64_t deflateexceptionid;
			libmaus::exception::LibMausException::unique_ptr_type deflatepse;
			libmaus::parallel::PosixMutex deflateexlock;

			libmaus::parallel::PosixMutex deflateqlock;


			BgzfDeflateParallelContext(
				std::ostream & rdeflateout, uint64_t const rnumbuffers				
			)
			: deflateoutid(0), deflatenextwriteid(0), deflateout(rdeflateout), deflateoutflushed(false), 
			  deflateB(rnumbuffers), 
			  deflatecurobject(-1),
			  deflateheapcomp(deflateB), 
			  deflateheapinfo(deflateB),
			  deflatewritequeue(deflateheapcomp,deflateheapinfo,&deflategloblist),
			  deflateexceptionid(std::numeric_limits<uint64_t>::max())
			{
				for ( uint64_t i = 0; i < deflateB.size(); ++i )
				{
					deflateB[i] = UNIQUE_PTR_MOVE(libmaus::lz::BgzfDeflateBase::unique_ptr_type(
						new libmaus::lz::BgzfDeflateBase()
					));
					// completely empty buffer on flush
					deflateB[i]->flushmode = true;
					deflateB[i]->objectid = i;
					deflatefreelist.enque(i);
				}
					
				deflatecurobject = deflatefreelist.deque();
				deflateB[deflatecurobject]->blockid = deflateoutid++;
			}
		};

		struct BgzfDeflateParallelThread : public libmaus::parallel::PosixThread
		{
			typedef BgzfDeflateParallelThread this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			
			enum op_type { op_compress_block = 0, op_write_block = 1, op_none = 2 };

			BgzfDeflateParallelContext & deflatecontext;
			
			BgzfDeflateParallelThread(BgzfDeflateParallelContext & rdeflatecontext) 
			: deflatecontext(rdeflatecontext)
			{
			}
			~BgzfDeflateParallelThread()
			{
			}
		
			void * run()
			{
				while ( true )
				{
					/* get any id from global list */
					try
					{
						deflatecontext.deflategloblist.deque();
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
						libmaus::parallel::ScopePosixMutex S(deflatecontext.deflateqlock);
				
						if ( deflatecontext.deflatecompqueue.size() )
						{
							objectid = deflatecontext.deflatecompqueue.front();
							deflatecontext.deflatecompqueue.pop_front();
							op = op_compress_block;
						}
						else if ( deflatecontext.deflatewritequeue.getFillState() )
						{
							objectid = deflatecontext.deflatewritequeue.deque();
							libmaus::parallel::ScopePosixMutex O(deflatecontext.deflateoutlock);								
							assert ( deflatecontext.deflateB[objectid]->blockid == deflatecontext.deflatenextwriteid );
							op = op_write_block;
						}									

						assert ( op != op_none );
					}
					
					switch ( op )
					{
						case op_compress_block:
						{
							try
							{
								deflatecontext.deflateB[objectid]->compsize = deflatecontext.deflateB[objectid]->flush(true /* full flush */);								
							}
							catch(libmaus::exception::LibMausException const & ex)
							{								
								libmaus::parallel::ScopePosixMutex S(deflatecontext.deflateexlock);
					
								if ( deflatecontext.deflateB[objectid]->blockid * 2 + 0 < deflatecontext.deflateexceptionid )
								{
									deflatecontext.deflateexceptionid = deflatecontext.deflateB[objectid]->blockid * 2 + 0;
									deflatecontext.deflatepse = UNIQUE_PTR_MOVE(ex.uclone());
								}
								
								deflatecontext.deflateB[objectid]->compsize = 0;
							}
							catch(std::exception const & ex)
							{
								libmaus::parallel::ScopePosixMutex S(deflatecontext.deflateexlock);

								if ( deflatecontext.deflateB[objectid]->blockid * 2 + 0 < deflatecontext.deflateexceptionid )
								{
									deflatecontext.deflateexceptionid = deflatecontext.deflateB[objectid]->blockid * 2 + 0;
								
									libmaus::exception::LibMausException se;
									se.getStream() << ex.what() << std::endl;
									se.finish();
									deflatecontext.deflatepse = UNIQUE_PTR_MOVE(se.uclone());
								}
								
								deflatecontext.deflateB[objectid]->compsize = 0;
							}

							libmaus::parallel::ScopePosixMutex S(deflatecontext.deflateqlock);
							deflatecontext.deflatewritequeue.enque(objectid);
							//deflatecontext.deflategloblist.enque(objectid);
							break;
						}
						case op_write_block:
						{
							libmaus::parallel::ScopePosixMutex O(deflatecontext.deflateoutlock);
							try
							{
								deflatecontext.deflateout.write(
									reinterpret_cast<char const *>(deflatecontext.deflateB[objectid]->outbuf.begin()),
									deflatecontext.deflateB[objectid]->compsize
								);
								
								if ( ! deflatecontext.deflateout )
								{
									libmaus::exception::LibMausException se;
									se.getStream() << "BgzfDeflateParallel: output error on output stream." << std::endl;
									se.finish();
									throw se;
								}
							}
							catch(libmaus::exception::LibMausException const & ex)
							{								
								libmaus::parallel::ScopePosixMutex S(deflatecontext.deflateexlock);

								if ( deflatecontext.deflateB[objectid]->blockid * 2 + 1 < deflatecontext.deflateexceptionid )
								{
									deflatecontext.deflateexceptionid = deflatecontext.deflateB[objectid]->blockid * 2 + 1;
									deflatecontext.deflatepse = UNIQUE_PTR_MOVE(ex.uclone());									
								}
							}
							catch(std::exception const & ex)
							{
								libmaus::parallel::ScopePosixMutex S(deflatecontext.deflateexlock);

								if ( deflatecontext.deflateB[objectid]->blockid * 2 + 1 < deflatecontext.deflateexceptionid )
								{
									deflatecontext.deflateexceptionid = deflatecontext.deflateB[objectid]->blockid * 2 + 1;
								
									libmaus::exception::LibMausException se;
									se.getStream() << ex.what() << std::endl;
									se.finish();
									deflatecontext.deflatepse = UNIQUE_PTR_MOVE(se.uclone());
								}
							}

							deflatecontext.deflatenextwriteid += 1;								
							deflatecontext.deflatewritequeue.setReadyFor(deflatecontext.deflatenextwriteid);
							deflatecontext.deflatefreelist.enque(objectid);
							break;
						}
						default:
						{
							break;
						}
					}						
				}
				
				return 0;		
			}
		};

		struct BgzfDeflateParallel
		{
			BgzfDeflateParallelContext deflatecontext;

			libmaus::autoarray::AutoArray < BgzfDeflateParallelThread::unique_ptr_type > T;

			BgzfDeflateParallel(std::ostream & rdeflateout, uint64_t const rnumthreads, uint64_t const rnumbuffers)
			: deflatecontext(rdeflateout,rnumbuffers), T(rnumthreads)
			{
				for ( uint64_t i = 0; i < T.size(); ++i )
				{
					T[i] = UNIQUE_PTR_MOVE(BgzfDeflateParallelThread::unique_ptr_type(new BgzfDeflateParallelThread(deflatecontext)));
					T[i]->start();
				}
			}
			~BgzfDeflateParallel()
			{
				flush();
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

					if ( deflatecontext.deflateB[deflatecontext.deflatecurobject]->pc == deflatecontext.deflateB[deflatecontext.deflatecurobject]->pe )
					{
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
					
						{
							libmaus::parallel::ScopePosixMutex Q(deflatecontext.deflateqlock);
							deflatecontext.deflatecompqueue.push_back(deflatecontext.deflatecurobject);
						}

						deflatecontext.deflategloblist.enque(deflatecontext.deflatecurobject);
						
						deflatecontext.deflatecurobject = deflatecontext.deflatefreelist.deque();
						deflatecontext.deflateB[deflatecontext.deflatecurobject]->blockid = deflatecontext.deflateoutid++;
					}
				}
			}
			
			void drain()
			{
				{
					libmaus::parallel::ScopePosixMutex Q(deflatecontext.deflateqlock);
					if ( deflatecontext.deflateB[deflatecontext.deflatecurobject]->pc != deflatecontext.deflateB[deflatecontext.deflatecurobject]->pa )
					{
						deflatecontext.deflatecompqueue.push_back(deflatecontext.deflatecurobject);
						deflatecontext.deflategloblist.enque(deflatecontext.deflatecurobject);
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
					uint64_t const eofflushsize = eofBase.flush(true /* full flush */);
					deflatecontext.deflateout.write(reinterpret_cast<char const *>(eofBase.outbuf.begin()),eofflushsize);
					
					deflatecontext.deflateoutflushed = true;
				}
			}
		};
	}
}
#endif
