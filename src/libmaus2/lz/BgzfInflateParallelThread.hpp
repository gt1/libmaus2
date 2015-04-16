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
#if ! defined(LIBMAUS2_LZ_BGZFINFLATEPARALLELTHREAD_HPP)
#define LIBMAUS2_LZ_BGZFINFLATEPARALLELTHREAD_HPP

#include <libmaus2/lz/BgzfInflateBlock.hpp>
#include <libmaus2/parallel/TerminatableSynchronousQueue.hpp>
#include <libmaus2/parallel/TerminatableSynchronousHeap.hpp>
#include <libmaus2/parallel/PosixThread.hpp>
#include <libmaus2/parallel/OMPNumThreadsScope.hpp>
#include <libmaus2/lz/BgzfInflateBlockIdComparator.hpp>
#include <libmaus2/lz/BgzfInflateBlockIdInfo.hpp>
#include <libmaus2/lz/BgzfInflateParallelContext.hpp>
#include <libmaus2/lz/BgzfThreadOpBase.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct BgzfInflateParallelThread : public libmaus2::parallel::PosixThread, public ::libmaus2::lz::BgzfThreadOpBase
		{
			typedef BgzfInflateParallelThread this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
					
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
					BgzfThreadQueueElement globbtqe;
					
					/* get any id from global list */
					try
					{
						globbtqe = inflatecontext.inflategloblist.deque();
					}
					catch(std::exception const & ex)
					{
						/* queue is terminated, break loop */
						break;
					}
					
					/* check which operation we are to perform */
					libmaus2_lz_bgzf_op_type op = globbtqe.op;
					uint64_t objectid = 0;
				
					switch ( op )
					{
						case libmaus2_lz_bgzf_op_read_block:
						{
							libmaus2::parallel::ScopePosixMutex S(inflatecontext.inflateqlock);
							objectid = inflatecontext.inflatefreelist.front();
							inflatecontext.inflatefreelist.pop_front();
							break;
						}
						case libmaus2_lz_bgzf_op_decompress_block:
						{
							libmaus2::parallel::ScopePosixMutex S(inflatecontext.inflateqlock);
							objectid = inflatecontext.inflatereadlist.front();
							inflatecontext.inflatereadlist.pop_front();
							
						}
						default:
							break;
					}						
					
					/*   */
					switch ( op )
					{
						/* read a block */
						case libmaus2_lz_bgzf_op_read_block:
						{
							libmaus2::parallel::ScopePosixMutex I(inflatecontext.inflateinlock);
							inflatecontext.inflateB[objectid]->blockid = inflatecontext.inflateinid++;
							inflatecontext.inflateB[objectid]->readBlock(inflatecontext.inflatein);

							/* copy compressed block to copy stream if there is one */
							if ( (! (inflatecontext.inflateB[objectid]->failed())) && inflatecontext.copyostr )
							{
								// copy data
								
								// bgzf header
								inflatecontext.copyostr->write(
									reinterpret_cast<char const *>(inflatecontext.inflateB[objectid]->header.begin()),
									inflatecontext.inflateB[objectid]->getBgzfHeaderSize()
								);
								// payload and footer
								inflatecontext.copyostr->write(
									reinterpret_cast<char const *>(inflatecontext.inflateB[objectid]->block.begin()),
									inflatecontext.inflateB[objectid]->blockinfo.compressed + inflatecontext.inflateB[objectid]->getBgzfFooterSize()
								);

								// flush output stream if block is empty (might be EOF)
								if ( ! inflatecontext.inflateB[objectid]->blockinfo.uncompressed )
									inflatecontext.copyostr->flush();

								// check state of stream
								if ( ! (*(inflatecontext.copyostr)) )
								{
									libmaus2::exception::LibMausException::unique_ptr_type tex(new libmaus2::exception::LibMausException);
									inflatecontext.inflateB[objectid]->ex = UNIQUE_PTR_MOVE(tex);
									inflatecontext.inflateB[objectid]->ex->getStream() << "Failed to write Bgzf data to copy stream.";
									inflatecontext.inflateB[objectid]->ex->finish();
								}
							}
							
							libmaus2::parallel::ScopePosixMutex Q(inflatecontext.inflateqlock);
							inflatecontext.inflatereadlist.push_back(objectid);
							inflatecontext.inflategloblist.enque(
								BgzfThreadQueueElement(
									libmaus2_lz_bgzf_op_decompress_block,
									objectid,
									inflatecontext.inflateB[objectid]->blockid
								)
							);
							break;
						}
						/* decompress block */
						case libmaus2_lz_bgzf_op_decompress_block:
						{
							inflatecontext.inflateB[objectid]->decompressBlock();
							libmaus2::parallel::ScopePosixMutex Q(inflatecontext.inflateqlock);
							inflatecontext.inflatedecompressedlist.enque(
								BgzfThreadQueueElement(
									libmaus2_lz_bgzf_op_none,
									objectid,
									inflatecontext.inflateB[objectid]->blockid
								)
							);
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
