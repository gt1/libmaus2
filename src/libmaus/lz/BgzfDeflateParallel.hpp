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
#include <libmaus/lz/BgzfDeflateBlockIdInfo.hpp>
#include <libmaus/lz/BgzfDeflateBlockIdComparator.hpp>
#include <libmaus/lz/BgzfDeflateParallelContext.hpp>
#include <libmaus/lz/BgzfDeflateParallelThread.hpp>

namespace libmaus
{
	namespace lz
	{
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
