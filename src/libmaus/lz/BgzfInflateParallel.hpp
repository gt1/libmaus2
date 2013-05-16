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
#if ! defined(LIBMAUS_LZ_BGZFINFLATEPARALLEL_HPP)
#define LIBMAUS_LZ_BGZFINFLATEPARALLEL_HPP

#include <libmaus/lz/BgzfInflateBlock.hpp>
#include <libmaus/parallel/TerminatableSynchronousQueue.hpp>
#include <libmaus/parallel/TerminatableSynchronousHeap.hpp>
#include <libmaus/parallel/PosixThread.hpp>

namespace libmaus
{
	namespace lz
	{
		struct BgzfInflateParallel
		{
			struct BgzfInflateBlockBlockIdComparator
			{
				BgzfInflateParallel * parent;
						
				BgzfInflateBlockBlockIdComparator(BgzfInflateParallel * rparent) : parent(rparent)
				{
				
				}

				bool operator()(uint64_t const i, uint64_t const j) const
				{
					return parent->B[i]->blockid > parent->B[j]->blockid;
				}		
			};

			struct BgzfInflateParallelThread : public libmaus::parallel::PosixThread
			{
				typedef BgzfInflateParallelThread this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				
				enum op_type { op_read_block, op_decompress_block, op_none };
			
				BgzfInflateParallel * parent;
				
				BgzfInflateParallelThread(BgzfInflateParallel * rparent) : parent(rparent)
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
						
						{
							libmaus::parallel::ScopePosixMutex S(parent->qlock);
							
							if ( parent->inflatefreelist.size() )
							{
							
								op = op_read_block;
								objectid = parent->inflatefreelist.front();
								parent->inflatefreelist.pop_front();
							}
							else if ( parent->inflatereadlist.size() )
							{
								op = op_decompress_block;
								objectid = parent->inflatereadlist.front();
								parent->inflatereadlist.pop_front();
							}
						}
						
						/*   */
						switch ( op )
						{
							/* read a block */
							case op_read_block:
							{
								libmaus::parallel::ScopePosixMutex I(parent->inlock);
								parent->B[objectid]->blockid = parent->inid++;
								parent->B[objectid]->readBlock(parent->in);

								libmaus::parallel::ScopePosixMutex Q(parent->qlock);
								parent->inflatereadlist.push_back(objectid);
								parent->globlist.enque(objectid);
								break;
							}
							/* decompress block */
							case op_decompress_block:
							{
								parent->B[objectid]->decompressBlock();
								libmaus::parallel::ScopePosixMutex Q(parent->qlock);
								parent->inflatedecompressedlist.enque(objectid);
								break;
							}
							default:
								break;
						}
					}
					
					return 0;		
				}
			};

			std::istream & in;
			uint64_t inid;
			libmaus::parallel::PosixMutex inlock;
			
			libmaus::parallel::PosixMutex qlock;

			libmaus::autoarray::AutoArray<libmaus::lz::BgzfInflateBlock::unique_ptr_type> B;
			
			libmaus::parallel::TerminatableSynchronousQueue<uint64_t> globlist;
			std::deque<uint64_t> inflatefreelist;
			std::deque<uint64_t> inflatereadlist;
			BgzfInflateBlockBlockIdComparator inflateheapcomp;
			libmaus::parallel::SynchronousHeap<uint64_t,BgzfInflateBlockBlockIdComparator> inflatedecompressedlist;
			
			libmaus::autoarray::AutoArray<BgzfInflateParallelThread::unique_ptr_type> T;
			
			uint64_t eb;
			uint64_t gcnt;
			
			BgzfInflateParallel(std::istream & rin, uint64_t const rnumthreads, uint64_t const rnumblocks)
			: 
				in(rin), inid(0), 
				B(rnumblocks), 
				inflateheapcomp(this), 
				inflatedecompressedlist(inflateheapcomp),
				T(rnumthreads),
				eb(0),
				gcnt(0)
			{
				for ( uint64_t i = 0; i < B.size(); ++i )
				{
					B[i] = UNIQUE_PTR_MOVE(libmaus::lz::BgzfInflateBlock::unique_ptr_type(new libmaus::lz::BgzfInflateBlock(i)));
					globlist.enque(i);
					inflatefreelist.push_back(i);
				}

				for ( uint64_t i = 0; i < T.size(); ++i )
				{
					T[i] = UNIQUE_PTR_MOVE(BgzfInflateParallelThread::unique_ptr_type(new BgzfInflateParallelThread(this)));
					T[i]->start();
				}
			}
			
			uint64_t gcount() const
			{
				return gcnt;
			}
			
			uint64_t read(char * const data, uint64_t const n)
			{
				gcnt = 0;
			
				if ( n < libmaus::lz::BgzfInflateBlock::maxblocksize )
				{
					libmaus::exception::LibMausException se;
					se.getStream() << "BgzfInflateParallel::read(): buffer provided is too small: " << n << " < " << libmaus::lz::BgzfInflateBlock::maxblocksize << std::endl;
					se.finish();
					throw se;
				}
			
				std::vector < uint64_t > putback;
				uint64_t objectid = 0;
				
				/* get object ids until we have the next expected one, then put out of order entries back */
				while ( true )
				{
					objectid = inflatedecompressedlist.deque();
					
					/* we have an exception, terminate readers and throw it at caller */
					if ( B[objectid]->failed() )
					{
						libmaus::parallel::ScopePosixMutex Q(qlock);
						globlist.terminate();
						throw B[objectid]->getException();
					}
					/* the obtained id is not the next expected one, register it for requeuing */
					else if ( B[objectid]->blockid != eb )
						putback.push_back(objectid);
					/* we have what we wanted, break loop */
					else
						break;
				}
				
				/* put unwanted ids back */
				for ( uint64_t i = 0; i < putback.size(); ++i )
					inflatedecompressedlist.enque(putback[i]);

				uint64_t const blocksize = B[objectid]->blockinfo.second;
				
				/* empty block (EOF) */
				if ( ! blocksize )
				{
					libmaus::parallel::ScopePosixMutex Q(qlock);
					globlist.terminate();
					
					return 0;
				}
				/* block contains data */
				else
				{
					uint64_t const blockid = B[objectid]->blockid;
					assert ( blockid == eb );
					eb += 1;

					std::copy(B[objectid]->data.begin(), B[objectid]->data.begin()+blocksize, reinterpret_cast<uint8_t *>(data));

					libmaus::parallel::ScopePosixMutex Q(qlock);
					inflatefreelist.push_back(objectid);
					globlist.enque(objectid);
					
					gcnt = blocksize;
					
					return blocksize;
				}
			}
		};
	}
}
#endif
