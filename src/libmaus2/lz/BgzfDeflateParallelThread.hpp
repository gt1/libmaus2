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
#if ! defined(LIBMAUS_LZ_BGZFDEFLATEPARALLELTHREAD_HPP)
#define LIBMAUS_LZ_BGZFDEFLATEPARALLELTHREAD_HPP

#include <libmaus2/lz/BgzfDeflateBase.hpp>
#include <libmaus2/parallel/TerminatableSynchronousQueue.hpp>
#include <libmaus2/parallel/TerminatableSynchronousHeap.hpp>
#include <libmaus2/parallel/PosixThread.hpp>
#include <libmaus2/parallel/OMPNumThreadsScope.hpp>
#include <libmaus2/lz/BgzfDeflateBlockIdInfo.hpp>
#include <libmaus2/lz/BgzfDeflateBlockIdComparator.hpp>
#include <libmaus2/lz/BgzfDeflateParallelContext.hpp>
#include <libmaus2/lz/BgzfThreadOpBase.hpp>
#include <libmaus2/util/utf8.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct BgzfDeflateParallelThread : public libmaus2::parallel::PosixThread, public libmaus2::lz::BgzfThreadOpBase
		{
			typedef BgzfDeflateParallelThread this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			
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
					BgzfThreadQueueElement defglob;
					try
					{
						defglob = deflatecontext.deflategloblist.deque();
					}
					catch(std::exception const & ex)
					{
						/* queue is terminated, break loop */
						break;
					}

					libmaus2_lz_bgzf_op_type op = defglob.op;
					uint64_t objectid = 0;
					
					{
						libmaus2::parallel::ScopePosixMutex S(deflatecontext.deflateqlock);
						
						switch ( op )
						{
							case libmaus2_lz_bgzf_op_compress_block:
								objectid = deflatecontext.deflatecompqueue.front();
								deflatecontext.deflatecompqueue.pop_front();
								break;
							case libmaus2_lz_bgzf_op_write_block:
							{
								BgzfThreadQueueElement const btqe = deflatecontext.deflatewritequeue.deque();
								objectid = btqe.objectid;
								libmaus2::parallel::ScopePosixMutex O(deflatecontext.deflateoutlock);
								assert ( deflatecontext.deflateB[objectid]->blockid == deflatecontext.deflatenextwriteid );
								break;
							}
							default:
								break;
						}

						assert ( op != libmaus2_lz_bgzf_op_none );
					}					
					
					switch ( op )
					{
						case libmaus2_lz_bgzf_op_compress_block:
						{
							try
							{
								BgzfDeflateZStreamBaseFlushInfo const BDZSBFI = deflatecontext.deflateB[objectid]->flush(true /* full flush */);
								assert ( !BDZSBFI.movesize );
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

									libmaus2::exception::LibMausException::unique_ptr_type tex(se.uclone());
									deflatecontext.deflatepse = UNIQUE_PTR_MOVE(tex);
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
							break;
						}
						case libmaus2_lz_bgzf_op_write_block:
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
								
								#if 0
								std::cerr << "compsize " 
									<< deflatecontext.deflateB[objectid]->compsize 
									<< " outbytes "
									<< deflatecontext.deflateoutbytes
									<< std::endl;
								#endif
								
								#if 0 // this test is done in streamWrite()
								if ( ! deflatecontext.deflateout )
								{
									libmaus2::exception::LibMausException se;
									se.getStream() << "BgzfDeflateParallel: output error on output stream." << std::endl;
									se.finish();
									throw se;
								}
								#endif
								
								if ( deflatecontext.deflateindexstr )
								{
									if ( deflatecontext.deflateB[objectid]->flushinfo.blocks == 0 )
									{
									
									}
									else if ( deflatecontext.deflateB[objectid]->flushinfo.blocks == 1 )
									{
										libmaus2::util::UTF8::encodeUTF8(deflatecontext.deflateB[objectid]->flushinfo.block_a_u,*(deflatecontext.deflateindexstr));
										libmaus2::util::UTF8::encodeUTF8(deflatecontext.deflateB[objectid]->flushinfo.block_a_c,*(deflatecontext.deflateindexstr));
									}
									else if ( deflatecontext.deflateB[objectid]->flushinfo.blocks == 2 )
									{
										libmaus2::util::UTF8::encodeUTF8(deflatecontext.deflateB[objectid]->flushinfo.block_a_u,*(deflatecontext.deflateindexstr));
										libmaus2::util::UTF8::encodeUTF8(deflatecontext.deflateB[objectid]->flushinfo.block_a_c,*(deflatecontext.deflateindexstr));
										libmaus2::util::UTF8::encodeUTF8(deflatecontext.deflateB[objectid]->flushinfo.block_b_u,*(deflatecontext.deflateindexstr));
										libmaus2::util::UTF8::encodeUTF8(deflatecontext.deflateB[objectid]->flushinfo.block_b_c,*(deflatecontext.deflateindexstr));
									}
								}
							}
							catch(libmaus2::exception::LibMausException const & ex)
							{								
								libmaus2::parallel::ScopePosixMutex S(deflatecontext.deflateexlock);

								if ( deflatecontext.deflateB[objectid]->blockid * 2 + 1 < deflatecontext.deflateexceptionid )
								{
									deflatecontext.deflateexceptionid = deflatecontext.deflateB[objectid]->blockid * 2 + 1;
									libmaus2::exception::LibMausException::unique_ptr_type tex(ex.uclone());
									deflatecontext.deflatepse = UNIQUE_PTR_MOVE(tex);									
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

									libmaus2::exception::LibMausException::unique_ptr_type tex(se.uclone());

									deflatecontext.deflatepse = UNIQUE_PTR_MOVE(tex);
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
	}
}
#endif
