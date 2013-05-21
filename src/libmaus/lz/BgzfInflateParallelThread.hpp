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
#if ! defined(LIBMAUS_LZ_BGZFINFLATEPARALLELTHREAD_HPP)
#define LIBMAUS_LZ_BGZFINFLATEPARALLELTHREAD_HPP

#include <libmaus/lz/BgzfInflateBlock.hpp>
#include <libmaus/parallel/TerminatableSynchronousQueue.hpp>
#include <libmaus/parallel/TerminatableSynchronousHeap.hpp>
#include <libmaus/parallel/PosixThread.hpp>
#include <libmaus/parallel/OMPNumThreadsScope.hpp>
#include <libmaus/lz/BgzfInflateBlockIdComparator.hpp>
#include <libmaus/lz/BgzfInflateBlockIdInfo.hpp>
#include <libmaus/lz/BgzfInflateParallelContext.hpp>

namespace libmaus
{
	namespace lz
	{
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
	}
}
#endif
