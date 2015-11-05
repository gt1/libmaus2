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
#if ! defined(LIBMAUS2_LZ_BGZFINFLATEDEFLATEPARALLELTHREAD_HPP)
#define LIBMAUS2_LZ_BGZFINFLATEDEFLATEPARALLELTHREAD_HPP

#include <libmaus2/lz/BgzfInflateBlock.hpp>
#include <libmaus2/parallel/TerminatableSynchronousQueue.hpp>
#include <libmaus2/parallel/TerminatableSynchronousHeap.hpp>
#include <libmaus2/parallel/PosixThread.hpp>
#include <libmaus2/parallel/OMPNumThreadsScope.hpp>
#include <libmaus2/lz/BgzfInflateBlockIdComparator.hpp>
#include <libmaus2/lz/BgzfInflateBlockIdInfo.hpp>
#include <libmaus2/lz/BgzfInflateParallelContext.hpp>
#include <libmaus2/lz/BgzfDeflateBlockIdComparator.hpp>
#include <libmaus2/lz/BgzfDeflateBlockIdInfo.hpp>
#include <libmaus2/lz/BgzfDeflateParallelContext.hpp>
#include <libmaus2/lz/BgzfThreadOpBase.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct BgzfDispatchReadBlock
		{
			static void readBlock(BgzfInflateParallelContext & inflatecontext)
			{
				uint64_t objectid = 0;

				{
					libmaus2::parallel::ScopePosixMutex S(inflatecontext.inflateqlock);
					objectid = inflatecontext.inflatefreelist.front();
					inflatecontext.inflatefreelist.pop_front();
				}

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

						// flush output stream if block is empty (might be eof)
						if ( inflatecontext.inflateB[objectid]->blockinfo.uncompressed )
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
							libmaus2::lz::BgzfThreadOpBase::libmaus2_lz_bgzf_op_decompress_block,
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
					libmaus2::parallel::ScopePosixMutex S(inflatecontext.inflateqlock);
					objectid = inflatecontext.inflatereadlist.front();
					inflatecontext.inflatereadlist.pop_front();
				}
				{
					inflatecontext.inflateB[objectid]->decompressBlock();
					libmaus2::parallel::ScopePosixMutex Q(inflatecontext.inflateqlock);
					inflatecontext.inflatedecompressedlist.enque(
						BgzfThreadQueueElement(
							libmaus2::lz::BgzfThreadOpBase::libmaus2_lz_bgzf_op_none,
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
					libmaus2::parallel::ScopePosixMutex S(deflatecontext.deflateqlock);
					objectid = deflatecontext.deflatecompqueue.front();
					deflatecontext.deflatecompqueue.pop_front();
				}
				{
					try
					{
						BgzfDeflateZStreamBaseFlushInfo const BDZSBFI = deflatecontext.deflateB[objectid]->flush(true /* full flush */);
						assert ( ! BDZSBFI.movesize );
						deflatecontext.deflateB[objectid]->compsize = BDZSBFI.getCompressedSize();
						deflatecontext.deflateB[objectid]->flushinfo = BDZSBFI;
					}
					catch(libmaus2::exception::LibMausException const & ex)
					{
						libmaus2::parallel::ScopePosixMutex S(deflatecontext.deflateexlock);

						if ( deflatecontext.deflateB[objectid]->blockid * 2 + 0 < deflatecontext.deflateexceptionid )
						{
							deflatecontext.deflateexceptionid = deflatecontext.deflateB[objectid]->blockid * 2 + 0;
							libmaus2::exception::LibMausException::unique_ptr_type tex(ex.uclone());
							deflatecontext.deflatepse = UNIQUE_PTR_MOVE(tex);
						}

						deflatecontext.deflateB[objectid]->compsize = 0;
						deflatecontext.deflateB[objectid]->flushinfo = BgzfDeflateZStreamBaseFlushInfo();
					}
					catch(std::exception const & ex)
					{
						libmaus2::parallel::ScopePosixMutex S(deflatecontext.deflateexlock);

						if ( deflatecontext.deflateB[objectid]->blockid * 2 + 0 < deflatecontext.deflateexceptionid )
						{
							deflatecontext.deflateexceptionid = deflatecontext.deflateB[objectid]->blockid * 2 + 0;

							libmaus2::exception::LibMausException se;
							se.getStream() << ex.what() << std::endl;
							se.finish();

							libmaus2::exception::LibMausException::unique_ptr_type ptr(se.uclone());
							deflatecontext.deflatepse = UNIQUE_PTR_MOVE(ptr);
						}

						deflatecontext.deflateB[objectid]->compsize = 0;
						deflatecontext.deflateB[objectid]->flushinfo = BgzfDeflateZStreamBaseFlushInfo();
					}

					libmaus2::parallel::ScopePosixMutex S(deflatecontext.deflateqlock);
					deflatecontext.deflatewritequeue.enque(
						BgzfThreadQueueElement(
							libmaus2::lz::BgzfThreadOpBase::libmaus2_lz_bgzf_op_write_block,
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
					libmaus2::parallel::ScopePosixMutex S(deflatecontext.deflateqlock);
					BgzfThreadQueueElement const btqe = deflatecontext.deflatewritequeue.deque();
					objectid = btqe.objectid;
					libmaus2::parallel::ScopePosixMutex O(deflatecontext.deflateoutlock);
					assert ( deflatecontext.deflateB[objectid]->blockid == deflatecontext.deflatenextwriteid );
				}
				{
					libmaus2::parallel::ScopePosixMutex O(deflatecontext.deflateoutlock);
					try
					{
						#if 0
						deflatecontext.deflateout.write(
							reinterpret_cast<char const *>(deflatecontext.deflateB[objectid]->outbuf.begin()),
							deflatecontext.deflateB[objectid]->compsize
						);
						#endif
						deflatecontext.streamWrite(
							deflatecontext.deflateB[objectid]->inbuf.begin(),
							deflatecontext.deflateB[objectid]->outbuf.begin(),
							deflatecontext.deflateB[objectid]->flushinfo
						);

						deflatecontext.deflateoutbytes += deflatecontext.deflateB[objectid]->compsize;

						#if 0 // this check is done in streamWrite
						if ( ! deflatecontext.deflateout )
						{
							libmaus2::exception::LibMausException se;
							se.getStream() << "BgzfDeflateParallel: output error on output stream." << std::endl;
							se.finish();
							throw se;
						}
						#endif
					}
					catch(libmaus2::exception::LibMausException const & ex)
					{
						libmaus2::parallel::ScopePosixMutex S(deflatecontext.deflateexlock);

						if ( deflatecontext.deflateB[objectid]->blockid * 2 + 1 < deflatecontext.deflateexceptionid )
						{
							deflatecontext.deflateexceptionid = deflatecontext.deflateB[objectid]->blockid * 2 + 1;
							libmaus2::exception::LibMausException::unique_ptr_type ptr(ex.uclone());
							deflatecontext.deflatepse = UNIQUE_PTR_MOVE(ptr);
						}
					}
					catch(std::exception const & ex)
					{
						libmaus2::parallel::ScopePosixMutex S(deflatecontext.deflateexlock);

						if ( deflatecontext.deflateB[objectid]->blockid * 2 + 1 < deflatecontext.deflateexceptionid )
						{
							deflatecontext.deflateexceptionid = deflatecontext.deflateB[objectid]->blockid * 2 + 1;

							libmaus2::exception::LibMausException se;
							se.getStream() << ex.what() << std::endl;
							se.finish();

							libmaus2::exception::LibMausException::unique_ptr_type ptr(se.uclone());
							deflatecontext.deflatepse = UNIQUE_PTR_MOVE(ptr);
						}
					}

					deflatecontext.deflatenextwriteid += 1;
					deflatecontext.deflatewritequeue.setReadyFor(
						BgzfThreadQueueElement(
							libmaus2::lz::BgzfThreadOpBase::libmaus2_lz_bgzf_op_write_block,
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
				public libmaus2::parallel::PosixThread,
				public libmaus2::lz::BgzfDispatchReadBlock,
				public libmaus2::lz::BgzfDispatchDecompressBlock,
				public libmaus2::lz::BgzfDispatchCompressBlock,
				public libmaus2::lz::BgzfDispatchWriteBlock
		{
			typedef BgzfInflateDeflateParallelThread this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

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
						case libmaus2::lz::BgzfThreadOpBase::libmaus2_lz_bgzf_op_read_block:
						{
							readBlock(inflatecontext);
							break;
						}
						case libmaus2::lz::BgzfThreadOpBase::libmaus2_lz_bgzf_op_decompress_block:
						{
							decompressBlock(inflatecontext);
							break;
						}
						case libmaus2::lz::BgzfThreadOpBase::libmaus2_lz_bgzf_op_compress_block:
						{
							compressBlock(deflatecontext);
							break;
						}
						case libmaus2::lz::BgzfThreadOpBase::libmaus2_lz_bgzf_op_write_block:
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
