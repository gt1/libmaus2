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
#if ! defined(LIBMAUS_LZ_BGZFDEFLATEPARALLELTHREAD_HPP)
#define LIBMAUS_LZ_BGZFDEFLATEPARALLELTHREAD_HPP

#include <libmaus/lz/BgzfDeflateBase.hpp>
#include <libmaus/parallel/TerminatableSynchronousQueue.hpp>
#include <libmaus/parallel/TerminatableSynchronousHeap.hpp>
#include <libmaus/parallel/PosixThread.hpp>
#include <libmaus/parallel/OMPNumThreadsScope.hpp>
#include <libmaus/lz/BgzfDeflateBlockIdInfo.hpp>
#include <libmaus/lz/BgzfDeflateBlockIdComparator.hpp>
#include <libmaus/lz/BgzfDeflateParallelContext.hpp>
#include <libmaus/lz/BgzfThreadOpBase.hpp>
#include <libmaus/util/utf8.hpp>

namespace libmaus
{
	namespace lz
	{
		struct BgzfDeflateParallelThread : public libmaus::parallel::PosixThread, public libmaus::lz::BgzfThreadOpBase
		{
			typedef BgzfDeflateParallelThread this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			
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

					libmaus_lz_bgzf_op_type op = defglob.op;
					uint64_t objectid = 0;
					
					{
						libmaus::parallel::ScopePosixMutex S(deflatecontext.deflateqlock);
						
						switch ( op )
						{
							case libmaus_lz_bgzf_op_compress_block:
								objectid = deflatecontext.deflatecompqueue.front();
								deflatecontext.deflatecompqueue.pop_front();
								break;
							case libmaus_lz_bgzf_op_write_block:
							{
								BgzfThreadQueueElement const btqe = deflatecontext.deflatewritequeue.deque();
								objectid = btqe.objectid;
								libmaus::parallel::ScopePosixMutex O(deflatecontext.deflateoutlock);
								assert ( deflatecontext.deflateB[objectid]->blockid == deflatecontext.deflatenextwriteid );
								break;
							}
							default:
								break;
						}

						assert ( op != libmaus_lz_bgzf_op_none );
					}					
					
					switch ( op )
					{
						case libmaus_lz_bgzf_op_compress_block:
						{
							try
							{
								BgzfDeflateZStreamBaseFlushInfo const BDZSBFI = deflatecontext.deflateB[objectid]->flush(true /* full flush */);
								deflatecontext.deflateB[objectid]->compsize = BDZSBFI.getCompressedSize();
							}
							catch(libmaus::exception::LibMausException const & ex)
							{								
								libmaus::parallel::ScopePosixMutex S(deflatecontext.deflateexlock);
					
								if ( deflatecontext.deflateB[objectid]->blockid * 2 + 0 < deflatecontext.deflateexceptionid )
								{
									deflatecontext.deflateexceptionid = deflatecontext.deflateB[objectid]->blockid * 2 + 0;
									libmaus::exception::LibMausException::unique_ptr_type tex(ex.uclone());
									deflatecontext.deflatepse = UNIQUE_PTR_MOVE(tex);
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

									libmaus::exception::LibMausException::unique_ptr_type tex(se.uclone());
									deflatecontext.deflatepse = UNIQUE_PTR_MOVE(tex);
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
							break;
						}
						case libmaus_lz_bgzf_op_write_block:
						{
							libmaus::parallel::ScopePosixMutex O(deflatecontext.deflateoutlock);
							try
							{
								deflatecontext.deflateout.write(
									reinterpret_cast<char const *>(deflatecontext.deflateB[objectid]->outbuf.begin()),
									deflatecontext.deflateB[objectid]->compsize
								);
								
								deflatecontext.deflateoutbytes += deflatecontext.deflateB[objectid]->compsize;
								
								#if 0
								std::cerr << "compsize " 
									<< deflatecontext.deflateB[objectid]->compsize 
									<< " outbytes "
									<< deflatecontext.deflateoutbytes
									<< std::endl;
								#endif
								
								if ( ! deflatecontext.deflateout )
								{
									libmaus::exception::LibMausException se;
									se.getStream() << "BgzfDeflateParallel: output error on output stream." << std::endl;
									se.finish();
									throw se;
								}
								
								if ( deflatecontext.deflateindexstr )
								{
									libmaus::util::UTF8::encodeUTF8(deflatecontext.deflateB[objectid]->uncompsize,*(deflatecontext.deflateindexstr));
									libmaus::util::UTF8::encodeUTF8(deflatecontext.deflateB[objectid]->compsize,*(deflatecontext.deflateindexstr));
								}
							}
							catch(libmaus::exception::LibMausException const & ex)
							{								
								libmaus::parallel::ScopePosixMutex S(deflatecontext.deflateexlock);

								if ( deflatecontext.deflateB[objectid]->blockid * 2 + 1 < deflatecontext.deflateexceptionid )
								{
									deflatecontext.deflateexceptionid = deflatecontext.deflateB[objectid]->blockid * 2 + 1;
									libmaus::exception::LibMausException::unique_ptr_type tex(ex.uclone());
									deflatecontext.deflatepse = UNIQUE_PTR_MOVE(tex);									
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

									libmaus::exception::LibMausException::unique_ptr_type tex(se.uclone());

									deflatecontext.deflatepse = UNIQUE_PTR_MOVE(tex);
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
