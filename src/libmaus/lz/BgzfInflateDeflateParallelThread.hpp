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
#if ! defined(LIBMAUS_LZ_BGZFINFLATEDEFLATEPARALLELTHREAD_HPP)
#define LIBMAUS_LZ_BGZFINFLATEDEFLATEPARALLELTHREAD_HPP

#include <libmaus/lz/BgzfInflateBlock.hpp>
#include <libmaus/parallel/TerminatableSynchronousQueue.hpp>
#include <libmaus/parallel/TerminatableSynchronousHeap.hpp>
#include <libmaus/parallel/PosixThread.hpp>
#include <libmaus/parallel/OMPNumThreadsScope.hpp>
#include <libmaus/lz/BgzfInflateBlockIdComparator.hpp>
#include <libmaus/lz/BgzfInflateBlockIdInfo.hpp>
#include <libmaus/lz/BgzfInflateParallelContext.hpp>
#include <libmaus/lz/BgzfDeflateBlockIdComparator.hpp>
#include <libmaus/lz/BgzfDeflateBlockIdInfo.hpp>
#include <libmaus/lz/BgzfDeflateParallelContext.hpp>
#include <libmaus/lz/BgzfThreadOpBase.hpp>

namespace libmaus
{
	namespace lz
	{
		struct BgzfDispatchReadBlock
		{
			static void readBlock(BgzfInflateParallelContext & inflatecontext)
			{
				uint64_t objectid = 0;
				
				{
					libmaus::parallel::ScopePosixMutex S(inflatecontext.inflateqlock);
					objectid = inflatecontext.inflatefreelist.front();
					inflatecontext.inflatefreelist.pop_front();
				}
				
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
								inflatecontext.inflateB[objectid]->getBgzfHeaderSize()
							);
							// payload and footer
							inflatecontext.copyostr->write(
								reinterpret_cast<char const *>(inflatecontext.inflateB[objectid]->block.begin()),
								inflatecontext.inflateB[objectid]->blockinfo.first + inflatecontext.inflateB[objectid]->getBgzfFooterSize()
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
							libmaus::lz::BgzfThreadOpBase::libmaus_lz_bgzf_op_decompress_block,
							objectid,
							inflatecontext.inflateB[objectid]->blockid
						)
					);
				}
			}
		};
		
		struct BgzfDispatchDecompressBlock
		{
			static void decompressBlock(BgzfInflateParallelContext & inflatecontext)
			{
				uint64_t objectid = 0;
				
				{
					libmaus::parallel::ScopePosixMutex S(inflatecontext.inflateqlock);
					objectid = inflatecontext.inflatereadlist.front();
					inflatecontext.inflatereadlist.pop_front();
				}
				{
					inflatecontext.inflateB[objectid]->decompressBlock();
					libmaus::parallel::ScopePosixMutex Q(inflatecontext.inflateqlock);
					inflatecontext.inflatedecompressedlist.enque(
						BgzfThreadQueueElement(
							libmaus::lz::BgzfThreadOpBase::libmaus_lz_bgzf_op_none,
							objectid,
							inflatecontext.inflateB[objectid]->blockid
						)
					);
				}			
			}		
		};
		
		struct BgzfDispatchCompressBlock
		{
			static void compressBlock(BgzfDeflateParallelContext & deflatecontext)
			{
				uint64_t objectid = 0;
				
				{
					libmaus::parallel::ScopePosixMutex S(deflatecontext.deflateqlock);
					objectid = deflatecontext.deflatecompqueue.front();
					deflatecontext.deflatecompqueue.pop_front();
				}
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
					deflatecontext.deflatewritequeue.enque(
						BgzfThreadQueueElement(
							libmaus::lz::BgzfThreadOpBase::libmaus_lz_bgzf_op_write_block,
							objectid,
							deflatecontext.deflateB[objectid]->blockid
						),
						&(deflatecontext.deflategloblist)
					);
				}			
			}	
		};
		
		struct BgzfDispatchWriteBlock
		{
			static void writeBlock(BgzfDeflateParallelContext & deflatecontext)
			{
				uint64_t objectid = 0;
				
				{
					libmaus::parallel::ScopePosixMutex S(deflatecontext.deflateqlock);
					BgzfThreadQueueElement const btqe = deflatecontext.deflatewritequeue.deque();
					objectid = btqe.objectid;
					libmaus::parallel::ScopePosixMutex O(deflatecontext.deflateoutlock);
					assert ( deflatecontext.deflateB[objectid]->blockid == deflatecontext.deflatenextwriteid );
				}
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
					deflatecontext.deflatewritequeue.setReadyFor(
						BgzfThreadQueueElement(
							libmaus::lz::BgzfThreadOpBase::libmaus_lz_bgzf_op_write_block,
							objectid,
							deflatecontext.deflatenextwriteid
							),
						&(deflatecontext.deflategloblist)
					);
					deflatecontext.deflatefreelist.enque(objectid);
				}
			}
		};
	
		struct BgzfInflateDeflateParallelThread 
			: 
				public libmaus::parallel::PosixThread, 
				public libmaus::lz::BgzfDispatchReadBlock,
				public libmaus::lz::BgzfDispatchDecompressBlock,
				public libmaus::lz::BgzfDispatchCompressBlock,
				public libmaus::lz::BgzfDispatchWriteBlock
		{
			typedef BgzfInflateDeflateParallelThread this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
		
			private:			
			BgzfInflateParallelContext & inflatecontext;
			BgzfDeflateParallelContext & deflatecontext;

			protected:
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
					switch ( globbtqe.op )
					{
						case libmaus::lz::BgzfThreadOpBase::libmaus_lz_bgzf_op_read_block:
						{
							readBlock(inflatecontext);
							break;
						}
						case libmaus::lz::BgzfThreadOpBase::libmaus_lz_bgzf_op_decompress_block:
						{
							decompressBlock(inflatecontext);
							break;
						}
						case libmaus::lz::BgzfThreadOpBase::libmaus_lz_bgzf_op_compress_block:
						{
							compressBlock(deflatecontext);
							break;
						}
						case libmaus::lz::BgzfThreadOpBase::libmaus_lz_bgzf_op_write_block:
						{
							writeBlock(deflatecontext);
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

			public:
			BgzfInflateDeflateParallelThread(
				BgzfInflateParallelContext & rinflatecontext,
				BgzfDeflateParallelContext & rdeflatecontext
			)
			: inflatecontext(rinflatecontext), deflatecontext(rdeflatecontext)
			{
			}
			~BgzfInflateDeflateParallelThread()
			{
			}

		};
	}
}
#endif
