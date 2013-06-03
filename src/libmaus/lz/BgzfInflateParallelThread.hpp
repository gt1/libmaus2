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
#include <libmaus/lz/BgzfThreadOpBase.hpp>

namespace libmaus
{
	namespace lz
	{
		struct BgzfInflateParallelThread : public libmaus::parallel::PosixThread, public ::libmaus::lz::BgzfThreadOpBase
		{
			typedef BgzfInflateParallelThread this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
					
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
					libmaus_lz_bgzf_op_type op = globbtqe.op;
					uint64_t objectid = 0;
				
					switch ( op )
					{
						case libmaus_lz_bgzf_op_read_block:
						{
							libmaus::parallel::ScopePosixMutex S(inflatecontext.inflateqlock);
							objectid = inflatecontext.inflatefreelist.front();
							inflatecontext.inflatefreelist.pop_front();
							break;
						}
						case libmaus_lz_bgzf_op_decompress_block:
						{
							libmaus::parallel::ScopePosixMutex S(inflatecontext.inflateqlock);
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
						case libmaus_lz_bgzf_op_read_block:
						{
							libmaus::parallel::ScopePosixMutex I(inflatecontext.inflateinlock);
							inflatecontext.inflateB[objectid]->blockid = inflatecontext.inflateinid++;
							inflatecontext.inflateB[objectid]->readBlock(inflatecontext.inflatein);

							/* copy compressed block to copy stream if there is one */
							if ( (! (inflatecontext.inflateB[objectid]->failed())) && inflatecontext.copyostr )
							{
								// copy data
								if ( inflatecontext.inflateB[objectid]->blockinfo.first )
								{
									// bgzf header
									inflatecontext.copyostr->write(
										reinterpret_cast<char const *>(inflatecontext.inflateB[objectid]->header.begin()),
										inflatecontext.inflateB[objectid]->headersize
									);
									// payload and footer
									inflatecontext.copyostr->write(
										reinterpret_cast<char const *>(inflatecontext.inflateB[objectid]->block.begin()),
										inflatecontext.inflateB[objectid]->blockinfo.first + inflatecontext.inflateB[objectid]->footersize
									);
								}
								// flush output stream if there is no more data
								else
									inflatecontext.copyostr->flush();

								// check state of stream
								if ( ! (*(inflatecontext.copyostr)) )
								{
									inflatecontext.inflateB[objectid]->ex = UNIQUE_PTR_MOVE(libmaus::exception::LibMausException::unique_ptr_type(new libmaus::exception::LibMausException));
									inflatecontext.inflateB[objectid]->ex->getStream() << "Failed to write Bgzf data to copy stream.";
									inflatecontext.inflateB[objectid]->ex->finish();
								}
							}
							
							libmaus::parallel::ScopePosixMutex Q(inflatecontext.inflateqlock);
							inflatecontext.inflatereadlist.push_back(objectid);
							inflatecontext.inflategloblist.enque(
								BgzfThreadQueueElement(
									libmaus_lz_bgzf_op_decompress_block,
									objectid,
									inflatecontext.inflateB[objectid]->blockid
								)
							);
							break;
						}
						/* decompress block */
						case libmaus_lz_bgzf_op_decompress_block:
						{
							inflatecontext.inflateB[objectid]->decompressBlock();
							libmaus::parallel::ScopePosixMutex Q(inflatecontext.inflateqlock);
							inflatecontext.inflatedecompressedlist.enque(
								BgzfThreadQueueElement(
									libmaus_lz_bgzf_op_none,
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
