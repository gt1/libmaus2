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
		struct BgzfDeflateParallel
		{			
			struct BgzfDeflateBlockBlockIdComparator
			{
				BgzfDeflateParallel * parent;
						
				BgzfDeflateBlockBlockIdComparator(BgzfDeflateParallel * rparent) : parent(rparent)
				{
				
				}

				bool operator()(uint64_t const i, uint64_t const j) const
				{
					return parent->B[i]->blockid > parent->B[j]->blockid;
				}		
			};

			struct BgzfDeflateParallelThread : public libmaus::parallel::PosixThread
			{
				typedef BgzfDeflateParallelThread this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				
				enum op_type { op_compress_block = 0, op_write_block = 1, op_none = 2 };

				BgzfDeflateParallel * parent;
				
				BgzfDeflateParallelThread(BgzfDeflateParallel * rparent) : parent(rparent)
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
							parent->globlist.deque();
						}
						catch(std::exception const & ex)
						{
							/* queue is terminated, break loop */
							break;
						}
						
						/* check which operation we are to perform */
						op_type op = op_none;
						uint64_t objectid = 0;
						
						while ( op == op_none )
						{
							{
								libmaus::parallel::ScopePosixMutex S(parent->qlock);
							
								if ( parent->deflatecompqueue.size() )
								{
									op = op_compress_block;
									objectid = parent->deflatecompqueue.front();
									parent->deflatecompqueue.pop_front();
								}
								else if ( parent->deflatewritequeue.size() )
								{
									objectid = parent->deflatewritequeue.top();
									parent->deflatewritequeue.pop();

									libmaus::parallel::ScopePosixMutex O(parent->outlock);
									
									if ( parent->B[objectid]->blockid == parent->nextwriteid )
										op = op_write_block;
									else
										parent->deflatewritequeue.push(objectid);
								}
							}

							if ( op == op_none )
							{
								// wait for 1/100 s
								struct timespec waittime = { 0, 10000000 };
								nanosleep(&waittime,0);
							}
						}
						
						switch ( op )
						{
							case op_compress_block:
							{
								try
								{
									parent->B[objectid]->compsize = parent->B[objectid]->flush(true /* full flush */);								
								}
								catch(libmaus::exception::LibMausException const & ex)
								{								
									libmaus::parallel::ScopePosixMutex S(parent->exlock);
						
									if ( parent->B[objectid]->blockid * 2 + 0 < parent->exceptionid )
									{
										parent->exceptionid = parent->B[objectid]->blockid * 2 + 0;
										parent->pse = UNIQUE_PTR_MOVE(ex.uclone());
									}
									
									parent->B[objectid]->compsize = 0;
								}
								catch(std::exception const & ex)
								{
									libmaus::parallel::ScopePosixMutex S(parent->exlock);

									if ( parent->B[objectid]->blockid * 2 + 0 < parent->exceptionid )
									{
										parent->exceptionid = parent->B[objectid]->blockid * 2 + 0;
									
										libmaus::exception::LibMausException se;
										se.getStream() << ex.what() << std::endl;
										se.finish();
										parent->pse = UNIQUE_PTR_MOVE(se.uclone());
									}
									
									parent->B[objectid]->compsize = 0;
								}

								libmaus::parallel::ScopePosixMutex S(parent->qlock);
								parent->deflatewritequeue.push(objectid);
								parent->globlist.enque(objectid);
								break;
							}
							case op_write_block:
							{
								libmaus::parallel::ScopePosixMutex O(parent->outlock);
								try
								{
									parent->out.write(
										reinterpret_cast<char const *>(parent->B[objectid]->outbuf.begin()),
										parent->B[objectid]->compsize
									);
									
									if ( ! parent->out )
									{
										libmaus::exception::LibMausException se;
										se.getStream() << "BgzfDeflateParallel: output error on output stream." << std::endl;
										se.finish();
										throw se;
									}
								}
								catch(libmaus::exception::LibMausException const & ex)
								{								
									libmaus::parallel::ScopePosixMutex S(parent->exlock);

									if ( parent->B[objectid]->blockid * 2 + 1 < parent->exceptionid )
									{
										parent->exceptionid = parent->B[objectid]->blockid * 2 + 1;
										parent->pse = UNIQUE_PTR_MOVE(ex.uclone());									
									}
								}
								catch(std::exception const & ex)
								{
									libmaus::parallel::ScopePosixMutex S(parent->exlock);

									if ( parent->B[objectid]->blockid * 2 + 1 < parent->exceptionid )
									{
										parent->exceptionid = parent->B[objectid]->blockid * 2 + 1;
									
										libmaus::exception::LibMausException se;
										se.getStream() << ex.what() << std::endl;
										se.finish();
										parent->pse = UNIQUE_PTR_MOVE(se.uclone());
									}
								}

								parent->nextwriteid += 1;								
								parent->deflatefreelist.enque(objectid);
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

			libmaus::parallel::PosixMutex outlock;
			// next block id to be filled with input
			uint64_t outid;
			// next block id to be written to compressed stream
			uint64_t nextwriteid;
			std::ostream & out;
			bool outflushed;

			libmaus::parallel::PosixMutex qlock;

			libmaus::autoarray::AutoArray<libmaus::lz::BgzfDeflateBase::unique_ptr_type> B;
			libmaus::parallel::SynchronousQueue<uint64_t> deflatefreelist;
			
			int64_t curobject;
			
			// queue for blocks to be compressed
			libmaus::parallel::TerminatableSynchronousQueue<uint64_t> globlist;
			std::deque<uint64_t> deflatecompqueue;
			// queue for compressed blocks to be written to output stream
			BgzfDeflateBlockBlockIdComparator deflateheapcomp;
			std::priority_queue<uint64_t,std::vector<uint64_t>,BgzfDeflateBlockBlockIdComparator> deflatewritequeue;
			
			libmaus::autoarray::AutoArray < BgzfDeflateParallelThread::unique_ptr_type > T;
			
			uint64_t exceptionid;
			libmaus::exception::LibMausException::unique_ptr_type pse;
			libmaus::parallel::PosixMutex exlock;

			BgzfDeflateParallel(std::ostream & rout, uint64_t const rnumthreads, uint64_t const rnumbuffers)
			: outid(0), nextwriteid(0), out(rout), outflushed(false), B(rnumbuffers), curobject(-1),
			  deflateheapcomp(this), deflatewritequeue(deflateheapcomp),
			  T(rnumthreads),
			  exceptionid(std::numeric_limits<uint64_t>::max())
			{
				for ( uint64_t i = 0; i < B.size(); ++i )
				{
					B[i] = UNIQUE_PTR_MOVE(libmaus::lz::BgzfDeflateBase::unique_ptr_type(
						new libmaus::lz::BgzfDeflateBase()
					));
					// completely empty buffer on flush
					B[i]->flushmode = true;
					B[i]->objectid = i;
					deflatefreelist.enque(i);
				}
					
				curobject = deflatefreelist.deque();
				B[curobject]->blockid = outid++;
				
				for ( uint64_t i = 0; i < T.size(); ++i )
				{
					T[i] = UNIQUE_PTR_MOVE(BgzfDeflateParallelThread::unique_ptr_type(new BgzfDeflateParallelThread(this)));
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
					uint64_t const freespace = B[curobject]->pe - B[curobject]->pc;
					uint64_t const towrite = std::min(n,freespace);
					std::copy(reinterpret_cast<uint8_t const *>(c),reinterpret_cast<uint8_t const *>(c)+towrite,B[curobject]->pc);

					c += towrite;
					B[curobject]->pc += towrite;
					n -= towrite;

					if ( B[curobject]->pc == B[curobject]->pe )
					{
						{
							exlock.lock();

							if ( exceptionid != std::numeric_limits<uint64_t>::max() )
							{
								exlock.unlock();
								
								drain();

								libmaus::parallel::ScopePosixMutex Q(exlock);
								throw (*pse);
							}
							else
							{
								exlock.unlock();
							}
						}
					
						{
							libmaus::parallel::ScopePosixMutex Q(qlock);
							deflatecompqueue.push_back(curobject);
						}

						globlist.enque(curobject);
						
						curobject = deflatefreelist.deque();
						B[curobject]->blockid = outid++;
					}
				}
			}
			
			void drain()
			{
				{
					libmaus::parallel::ScopePosixMutex Q(qlock);
					if ( B[curobject]->pc != B[curobject]->pa )
					{
						deflatecompqueue.push_back(curobject);
						globlist.enque(curobject);
					}
					else
					{
						deflatefreelist.enque(curobject);
					}
				}
					
				// wait until all threads are idle
				while ( deflatefreelist.getFillState() < B.size() )
				{
					// wait for 1/100 s
					// struct timespec waittime = { 0, 10000000 };
					struct timespec waittime = { 1,0 };
					nanosleep(&waittime,0);
				}			
			}
			
			void flush()
			{
				if ( ! outflushed )
				{
					drain();
				
					globlist.terminate();

					{
						exlock.lock();

						if ( exceptionid != std::numeric_limits<uint64_t>::max() )
						{
							exlock.unlock();
								
							libmaus::parallel::ScopePosixMutex Q(exlock);
							throw (*pse);
						}
						else
						{
							exlock.unlock();
						}
					}

					// write default compressed block with size 0 (EOF marker)
					libmaus::lz::BgzfDeflateBase eofBase;
					uint64_t const eofflushsize = eofBase.flush(true /* full flush */);
					out.write(reinterpret_cast<char const *>(eofBase.outbuf.begin()),eofflushsize);
					
					outflushed = true;
				}
			}
		};
	}
}
#endif
