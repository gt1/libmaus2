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
#if ! defined(LIBMAUS2_LZ_BGZFDEFLATEPARALLEL_HPP)
#define LIBMAUS2_LZ_BGZFDEFLATEPARALLEL_HPP

#include <libmaus2/lz/BgzfDeflateBase.hpp>
#include <libmaus2/parallel/TerminatableSynchronousQueue.hpp>
#include <libmaus2/parallel/TerminatableSynchronousHeap.hpp>
#include <libmaus2/parallel/PosixThread.hpp>
#include <libmaus2/parallel/OMPNumThreadsScope.hpp>
#include <libmaus2/lz/BgzfDeflateBlockIdInfo.hpp>
#include <libmaus2/lz/BgzfDeflateBlockIdComparator.hpp>
#include <libmaus2/lz/BgzfDeflateParallelContext.hpp>
#include <libmaus2/lz/BgzfDeflateParallelThread.hpp>
#include <libmaus2/lz/BgzfThreadOpBase.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct BgzfDeflateParallel
		{
			typedef BgzfDeflateParallel this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
		
			private:
			libmaus2::parallel::TerminatableSynchronousHeap<BgzfThreadQueueElement,BgzfThreadQueueElementHeapComparator>
				deflategloblist;
			BgzfDeflateParallelContext deflatecontext;
			libmaus2::autoarray::AutoArray < BgzfDeflateParallelThread::unique_ptr_type > T;

			void drain()
			{
				// handle last block
				{
					libmaus2::parallel::ScopePosixMutex Q(deflatecontext.deflateqlock);
					if ( deflatecontext.deflateB[deflatecontext.deflatecurobject]->pc != deflatecontext.deflateB[deflatecontext.deflatecurobject]->pa )
					{
						deflatecontext.deflatecompqueue.push_back(deflatecontext.deflatecurobject);
						deflatecontext.deflategloblist.enque(
							BgzfThreadQueueElement(
								BgzfThreadOpBase::libmaus2_lz_bgzf_op_compress_block,
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

			public:
			BgzfDeflateParallel(std::ostream & rdeflateout, uint64_t const rnumthreads, uint64_t const rnumbuffers, int const level, std::ostream * rdeflateindexostr = 0)
			: deflategloblist(), deflatecontext(deflategloblist,rdeflateout,rnumbuffers,level,BgzfDeflateParallelContext::getDefaultDeflateGetCur(),rdeflateindexostr), T(rnumthreads)
			{
				for ( uint64_t i = 0; i < T.size(); ++i )
				{
					BgzfDeflateParallelThread::unique_ptr_type tTi(new BgzfDeflateParallelThread(deflatecontext));
					T[i] = UNIQUE_PTR_MOVE(tTi);
					T[i]->start();
				}
			}
			~BgzfDeflateParallel()
			{
				flush();
			}

			void registerBlockOutputCallback(::libmaus2::lz::BgzfDeflateOutputCallback * cb)
			{
				deflatecontext.blockoutputcallbacks.push_back(cb);
			}
			
			void flushInternal()
			{
				{
					deflatecontext.deflateexlock.lock();

					if ( deflatecontext.deflateexceptionid != std::numeric_limits<uint64_t>::max() )
					{
						deflatecontext.deflateexlock.unlock();
						
						drain();

						libmaus2::parallel::ScopePosixMutex Q(deflatecontext.deflateexlock);
						throw (*(deflatecontext.deflatepse));
					}
					else
					{
						deflatecontext.deflateexlock.unlock();
					}
				}
			
				{
					libmaus2::parallel::ScopePosixMutex Q(deflatecontext.deflateqlock);
					deflatecontext.deflatecompqueue.push_back(deflatecontext.deflatecurobject);
				}

				deflatecontext.deflategloblist.enque(
					BgzfThreadQueueElement(
						BgzfThreadOpBase::libmaus2_lz_bgzf_op_compress_block,
						deflatecontext.deflatecurobject,
						0 /* block id */
					)
				);
				
				deflatecontext.deflatecurobject = deflatecontext.deflatefreelist.deque();
				deflatecontext.deflateB[deflatecontext.deflatecurobject]->blockid = deflatecontext.deflateoutid++;
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
						flushInternal();
				}
			}

			void writeSynced(char const * c, uint64_t n)
			{
				// flush if necessary
				if ( deflatecontext.deflateB[deflatecontext.deflatecurobject]->pc != deflatecontext.deflateB[deflatecontext.deflatecurobject]->pa )
					flushInternal();

				// enque data for compression
				while ( n )
				{
					uint64_t const freespace = deflatecontext.deflateB[deflatecontext.deflatecurobject]->pe - deflatecontext.deflateB[deflatecontext.deflatecurobject]->pc;
					uint64_t const towrite = std::min(n,freespace);
					std::copy(reinterpret_cast<uint8_t const *>(c),reinterpret_cast<uint8_t const *>(c)+towrite,deflatecontext.deflateB[deflatecontext.deflatecurobject]->pc);

					c += towrite;
					deflatecontext.deflateB[deflatecontext.deflatecurobject]->pc += towrite;
					n -= towrite;

					if ( deflatecontext.deflateB[deflatecontext.deflatecurobject]->pc == deflatecontext.deflateB[deflatecontext.deflatecurobject]->pe )
						flushInternal();
				}

				// flush if necessary
				if ( deflatecontext.deflateB[deflatecontext.deflatecurobject]->pc != deflatecontext.deflateB[deflatecontext.deflatecurobject]->pa )
					flushInternal();
			}

			/**
			 * write block c of length n. stream needs to be synced
			 * before using this function.
			 *
			 * @param c block data
			 * @param n number of bytes in c
			 * @return number of bgzf blocks written
			 **/
			uint64_t writeSyncedCount(char const * c, uint64_t n)
			{
				// check whether buffer is empty
				if ( deflatecontext.deflateB[deflatecontext.deflatecurobject]->pc != deflatecontext.deflateB[deflatecontext.deflatecurobject]->pa )
				{
					libmaus2::exception::LibMausException se;
					se.getStream() << "Call to BgzfDeflateParallel::writeSyncedCount() but stream is not synced." << std::endl;
					se.finish();
					throw se;
				}
				
				uint64_t bcnt = 0;

				// enque data for compression
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
						flushInternal();
						bcnt += 1;
					}
				}

				// flush if necessary
				if ( deflatecontext.deflateB[deflatecontext.deflatecurobject]->pc != deflatecontext.deflateB[deflatecontext.deflatecurobject]->pa )
				{
					flushInternal();
					bcnt += 1;
				}
				
				return bcnt;
			}

			void put(uint8_t const c)
			{
				assert ( deflatecontext.deflateB[deflatecontext.deflatecurobject]->pc != deflatecontext.deflateB[deflatecontext.deflatecurobject]->pe );

				*((deflatecontext.deflateB[deflatecontext.deflatecurobject]->pc)++) = c;

				if ( deflatecontext.deflateB[deflatecontext.deflatecurobject]->pc == deflatecontext.deflateB[deflatecontext.deflatecurobject]->pe )
					flushInternal();
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
								
							libmaus2::parallel::ScopePosixMutex Q(deflatecontext.deflateexlock);
							throw (*(deflatecontext.deflatepse));
						}
						else
						{
							deflatecontext.deflateexlock.unlock();
						}
					}

					// write default compressed block with size 0 (EOF marker)
					libmaus2::lz::BgzfDeflateBase eofBase;
					BgzfDeflateZStreamBaseFlushInfo const BDZSBFI = eofBase.flush(true /* full flush */);
					assert ( ! BDZSBFI.movesize );
					// uint64_t const eofflushsize = BDZSBFI.getCompressedSize();
					// deflatecontext.deflateout.write(reinterpret_cast<char const *>(eofBase.outbuf.begin()),eofflushsize);
					deflatecontext.streamWrite(eofBase.inbuf.begin(),eofBase.outbuf.begin(),BDZSBFI);
					
					// std::cerr << "Writing " << eofflushsize << " bytes for flush" << std::endl;
					
					deflatecontext.deflateoutflushed = true;
				}
			}
		};
	}
}
#endif
