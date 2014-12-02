/*
    libmaus
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_CONTROL_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_CONTROL_HPP

#include <libmaus/bambam/parallel/BlockCompressPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/AlignmentBlockCompressPackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/AddPendingCompressBufferWriteInterface.hpp>
#include <libmaus/bambam/parallel/ReturnCompressionPendingElementInterface.hpp>
#include <libmaus/bambam/parallel/AlignmentBlockCompressPackage.hpp>
#include <libmaus/bambam/parallel/ReturnRewriteBufferInterface.hpp>
#include <libmaus/bambam/parallel/CompressionPendingElement.hpp>
#include <libmaus/bambam/parallel/CompressBufferAllocator.hpp>
#include <libmaus/bambam/parallel/CompressBufferHeapComparator.hpp>
#include <libmaus/bambam/parallel/CompressBuffer.hpp>
#include <libmaus/bambam/parallel/AlignmentRewritePosSortContext.hpp>
#include <libmaus/bambam/parallel/SortFinishedInterface.hpp>
#include <libmaus/bambam/parallel/MergeSortWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/BaseMergeSortWorkPackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/AlignmentRewritePosMergeSortPackage.hpp>
#include <libmaus/bambam/parallel/BaseSortWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/BaseSortWorkPackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/AlignmentRewritePosSortBaseSortPackage.hpp>
#include <libmaus/bambam/parallel/AlignmentRewritePosSortContextMergePackageFinished.hpp>
#include <libmaus/bambam/parallel/AlignmentRewritePosSortContextBaseBlockSortedInterface.hpp>
#include <libmaus/bambam/parallel/RewriteBlockWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/AlignmentRewriteBufferReinsertForFillingInterface.hpp>
#include <libmaus/bambam/parallel/AlignmentRewriteBufferAddPendingInterface.hpp>
#include <libmaus/bambam/parallel/AlignmentBufferReinsertInterface.hpp>
#include <libmaus/bambam/parallel/AlignmentBufferReturnInterface.hpp>
#include <libmaus/bambam/parallel/RewritePackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/RewriteBlockWorkPackage.hpp>
#include <libmaus/bambam/parallel/ValidateBlockWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/ValidateBlockAddPendingInterface.hpp>
#include <libmaus/bambam/parallel/ValidatePackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/ValidateBlockWorkPackage.hpp>
#include <libmaus/bambam/parallel/ParseBlockWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/ParsePackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/ParsedBlockStallInterface.hpp>
#include <libmaus/bambam/parallel/ParsedBlockAddPendingInterface.hpp>
#include <libmaus/bambam/parallel/DecompressedBlockReturnInterface.hpp>
#include <libmaus/bambam/parallel/AlignmentRewriteBufferAllocator.hpp>
#include <libmaus/bambam/parallel/AlignmentBufferAllocator.hpp>
#include <libmaus/bambam/parallel/DecompressedPendingObjectHeapComparator.hpp>
#include <libmaus/bambam/parallel/DecompressedPendingObject.hpp>
#include <libmaus/bambam/parallel/ParseBlockWorkPackage.hpp>
#include <libmaus/bambam/parallel/ParseInfo.hpp>
#include <libmaus/bambam/parallel/AlignmentRewriteBufferPosComparator.hpp>
#include <libmaus/bambam/parallel/AlignmentRewriteBuffer.hpp>
#include <libmaus/bambam/parallel/AlignmentBuffer.hpp>
#include <libmaus/bambam/parallel/PushBackSpace.hpp>
#include <libmaus/bambam/parallel/DecompressBlockWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/BgzfInflateZStreamBaseReturnInterface.hpp>
#include <libmaus/bambam/parallel/DecompressedBlockAddPendingInterface.hpp>
#include <libmaus/bambam/parallel/InputBlockReturnInterface.hpp>
#include <libmaus/bambam/parallel/DecompressBlockWorkPackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/DecompressBlockWorkPackage.hpp>
#include <libmaus/bambam/parallel/DecompressedBlock.hpp>
#include <libmaus/bambam/parallel/InputBlockWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/InputBlockAddPendingInterface.hpp>
#include <libmaus/bambam/parallel/InputBlockWorkPackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/InputBlockWorkPackage.hpp>
#include <libmaus/bambam/parallel/ControlInputInfo.hpp>
#include <libmaus/bambam/parallel/InputBlock.hpp>

#include <libmaus/aio/PosixFdOutputStream.hpp>
#include <libmaus/parallel/LockedCounter.hpp>
#include <libmaus/parallel/LockedHeap.hpp>
#include <libmaus/parallel/LockedQueue.hpp>
#include <libmaus/parallel/SimpleThreadPool.hpp>
#include <libmaus/parallel/SimpleThreadPoolWorkPackageFreeList.hpp>
#include <libmaus/parallel/SynchronousCounter.hpp>
#include <libmaus/timing/RealTimeClock.hpp>

#include <libmaus/lz/CompressorObjectFreeListAllocatorFactory.hpp>

#include <stack>
#include <csignal>

namespace libmaus
{
	namespace bambam
	{		
		namespace parallel
		{
			template<typename _stream_type>
			struct NamedTempFile
			{
				typedef _stream_type stream_type;
				typedef NamedTempFile<stream_type> this_type;
				typedef typename libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef typename libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
	
				std::string const name;
				uint64_t const id;
				stream_type stream;
				
				NamedTempFile(std::string const & rname, uint64_t const rid)
				: name(rname), id(rid), stream(name) {}
				
				static unique_ptr_type uconstruct(std::string const & rname, uint64_t const rid)
				{
					unique_ptr_type tptr(new this_type(rname,rid));
					return UNIQUE_PTR_MOVE(tptr);
				}
	
				static shared_ptr_type sconstruct(std::string const & rname, uint64_t const rid)
				{
					shared_ptr_type tptr(new this_type(rname,rid));
					return tptr;
				}
				
				uint64_t getId() const
				{
					return id;
				}
				
				std::string const & getName() const
				{
					return name;
				}
				
				stream_type & getStream()
				{
					return stream;
				}
			};
	
			template<typename _stream_type>
			struct TemporaryFileAllocator
			{
				typedef _stream_type stream_type;
				
				std::string prefix;
				libmaus::parallel::SynchronousCounter<uint64_t> * S;
				
				TemporaryFileAllocator() : prefix(), S(0) {}
				TemporaryFileAllocator(
					std::string const & rprefix,
					libmaus::parallel::SynchronousCounter<uint64_t> * const rS
				) : prefix(rprefix), S(rS)
				{
				
				}
				
				NamedTempFile<stream_type> * operator()()
				{
					uint64_t const lid = static_cast<uint64_t>((*S)++);
					std::ostringstream fnostr;
					fnostr << prefix << "_" << std::setw(6) << std::setfill('0') << lid;
					std::string const fn = fnostr.str();
					return new NamedTempFile<stream_type>(fn,lid);
				}
			};
	
			// BamAlignmentRewriteBufferPosComparator
			template<typename _order_type>
			struct Control :
				public InputBlockWorkPackageReturnInterface,
				public InputBlockAddPendingInterface,
				public DecompressBlockWorkPackageReturnInterface,
				public InputBlockReturnInterface,
				public DecompressedBlockAddPendingInterface,
				public BgzfInflateZStreamBaseReturnInterface,
				public DecompressedBlockReturnInterface,
				public ParsedBlockAddPendingInterface,
				public ParsedBlockStallInterface,
				public ParsePackageReturnInterface,
				public ValidatePackageReturnInterface,
				public ValidateBlockAddPendingInterface,
				public RewritePackageReturnInterface,
				public AlignmentBufferReturnInterface,
				public AlignmentBufferReinsertInterface,
				public AlignmentRewriteBufferAddPendingInterface,
				public AlignmentRewriteBufferReinsertForFillingInterface,
				public BaseSortWorkPackageReturnInterface<_order_type>,
				public BaseMergeSortWorkPackageReturnInterface<_order_type>,
				public SortFinishedInterface,
				public ReturnRewriteBufferInterface,
				public AlignmentBlockCompressPackageReturnInterface,
				public AddPendingCompressBufferWriteInterface,
				public ReturnCompressionPendingElementInterface
			{
				typedef _order_type order_type;
			
				typedef Control<order_type> this_type;
				typedef typename libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef typename libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
	
				typedef DecompressedBlock decompressed_block_type;
				
				libmaus::parallel::SimpleThreadPool & STP;
				
				std::string const tempfileprefix;
				libmaus::parallel::SynchronousCounter<uint64_t> tempfilesyncid;
				typedef libmaus::aio::PosixFdOutputStream temp_stream_type;
				typedef NamedTempFile<temp_stream_type> named_temp_file_type;
				libmaus::parallel::LockedFreeList<
					named_temp_file_type, 
					TemporaryFileAllocator<temp_stream_type> 
				> tmpfilefreelist;
				std::map<uint64_t,std::string> tempfileidtoname;
	
				// compressor object free list
				libmaus::parallel::LockedGrowingFreeList<libmaus::lz::CompressorObject, libmaus::lz::CompressorObjectFreeListAllocator> compfreelist;
				
				InputBlockWorkPackageDispatcher readDispatcher;
				uint64_t const readDispatcherId;
				
				DecompressBlockWorkPackageDispatcher decompressDispatcher;
				uint64_t const decompressDispatcherId;
	
				ParseBlockWorkPackageDispatcher parseDispatcher;
				uint64_t const parseDispatcherId;
	
				ValidateBlockWorkPackageDispatcher validateDispatcher;
				uint64_t const validateDispatcherId;
				
				RewriteBlockWorkPackageDispatcher rewriteDispatcher;
				uint64_t const rewriteDispatcherId;
				
				BaseSortWorkPackageDispatcher<order_type> baseSortDispatcher;
				uint64_t const baseSortDispatcherId;
				
				MergeSortWorkPackageDispatcher<order_type> mergeSortDispatcher;
				uint64_t const mergeSortDispatcherId;
	
				BlockCompressPackageDispatcher blockCompressDispatcher;
				uint64_t const blockCompressDispatcherId;
				
				uint64_t const numthreads;
	
				ControlInputInfo inputinfo;
	
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<InputBlockWorkPackage> readWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<DecompressBlockWorkPackage> decompressWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<ParseBlockWorkPackage> parseWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<ValidateBlockWorkPackage> validateWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<RewriteBlockWorkPackage> rewriteWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<AlignmentRewritePosSortBaseSortPackage<order_type> > baseSortPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<AlignmentRewritePosMergeSortPackage<order_type> > mergeSortPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<AlignmentBlockCompressPackage> blockCompressPackages;
	
				// free list for bgzf decompressors
				libmaus::parallel::LockedFreeList<libmaus::lz::BgzfInflateZStreamBase>::unique_ptr_type Pdecoders;
				libmaus::parallel::LockedFreeList<libmaus::lz::BgzfInflateZStreamBase> & decoders;
				
				// free list for decompressed blocks
				libmaus::parallel::LockedFreeList<decompressed_block_type> decompressedBlockFreeList;
				
				// free list for alignment buffers
				libmaus::parallel::LockedFreeList<AlignmentBuffer,AlignmentBufferAllocator> parseBlockFreeList;
				
				// free list for rewrite buffers
				libmaus::parallel::LockedFreeList<AlignmentRewriteBuffer,AlignmentRewriteBufferAllocator> rewriteBlockFreeList;
	
				// input blocks ready to be decompressed
				libmaus::parallel::LockedQueue<ControlInputInfo::input_block_type *> decompressPending;
				
				// decompressed blocks ready to be parsed
				libmaus::parallel::LockedHeap<DecompressedPendingObject,DecompressedPendingObjectHeapComparator> parsePending;
				libmaus::parallel::PosixSpinLock parsePendingLock;
	
				// id of next block to be parsed
				uint64_t volatile nextDecompressedBlockToBeParsed;
				libmaus::parallel::PosixSpinLock nextDecompressedBlockToBeParsedLock;
				
				AlignmentBuffer * volatile parseStallSlot;
				libmaus::parallel::PosixSpinLock parseStallSlotLock;
	
				ParseInfo parseInfo;
				
				libmaus::parallel::LockedBool lastParseBlockSeen;
				libmaus::parallel::SynchronousCounter<uint64_t> readsParsed;
				libmaus::parallel::PosixSpinLock readsParsedLock;
				uint64_t volatile readsParsedLastPrint;
	
				libmaus::parallel::SynchronousCounter<uint64_t> parseBlocksSeen;
				libmaus::parallel::SynchronousCounter<uint64_t> parseBlocksValidated;
	
				libmaus::parallel::LockedBool lastParseBlockValidated;
				
				libmaus::parallel::SynchronousCounter<uint64_t> parseBlocksRewritten;
				
				libmaus::parallel::LockedBool lastParseBlockRewritten;
	
				// list of alignment blocks ready for rewriting			
				libmaus::parallel::LockedQueue<AlignmentBuffer *> rewritePending;
	
				libmaus::parallel::PosixSpinLock readsRewrittenLock;
				libmaus::parallel::SynchronousCounter<uint64_t> readsRewritten;
				
				// list of rewritten blocks to be sorted
				libmaus::parallel::LockedQueue<AlignmentRewriteBuffer *> sortPending;
				
				// blocks currently being sorted
				std::map < AlignmentRewriteBuffer *, typename AlignmentRewritePosSortContext<order_type>::shared_ptr_type > sortActive;
				libmaus::parallel::PosixSpinLock sortActiveLock;
	
				libmaus::parallel::SynchronousCounter<uint64_t> numSortBlocksIn;
				libmaus::parallel::SynchronousCounter<uint64_t> numSortBlocksOut;
	
				libmaus::parallel::LockedBool lastSortBlockFinished;
				libmaus::parallel::SynchronousCounter<uint64_t> readsSorted;
	
				uint64_t const compblocksize;
				libmaus::parallel::LockedFreeList < CompressBuffer, CompressBufferAllocator > compblockfreelist;
				
				//
				libmaus::parallel::LockedGrowingFreeList < CompressionPendingElement > compressObjectPendingElementFreeList;
				libmaus::parallel::LockedQueue < CompressionPendingElement * > compressObjectPending;
				libmaus::parallel::PosixSpinLock compressObjectPendingLock;
				
				uint64_t nextOutputSuperBlock;
				libmaus::parallel::PosixSpinLock nextOutputSuperBlockLock;
				
				libmaus::parallel::LockedGrowingFreeList < libmaus::parallel::LockedHeap<CompressBuffer *, CompressBufferHeapComparator> > writePendingHeapFreeList;
				std::map<uint64_t, libmaus::parallel::LockedHeap<CompressBuffer *, CompressBufferHeapComparator> * > writePending;
				libmaus::parallel::PosixSpinLock writePendingLock;
				
				std::map<uint64_t,uint64_t> writePendingNextOut;
				libmaus::parallel::PosixSpinLock writePendingNextOutLock;
				
				std::map<uint64_t,uint64_t> writePendingUnfinished;
				libmaus::parallel::PosixSpinLock writePendindUnfinishedLock;
							
				virtual void putAlignmentBlockCompressPackagePackage(AlignmentBlockCompressPackage * package)
				{
					blockCompressPackages.returnPackage(package);
				}
				
				virtual void putBlockWritten(CompressBuffer * buffer)
				{
				
				}
				
				void checkPendingWrite()
				{
					std::vector<CompressBuffer *> towrite;
				
					{
						libmaus::parallel::ScopePosixSpinLock lwritePendingLock(writePendingLock);
						libmaus::parallel::ScopePosixSpinLock lwritePendingNextOutLock(writePendingNextOutLock);
						
						for ( std::map<uint64_t, libmaus::parallel::LockedHeap<CompressBuffer *, CompressBufferHeapComparator> * >::iterator ita = writePending.begin();
							ita != writePending.end(); ++ita )
						{
							uint64_t const blockid = ita->first;
							
							assert ( writePendingNextOut.find(blockid) != writePendingNextOut.end() );
							
							uint64_t const nextpending = writePendingNextOut.find(blockid)->second;
							
							if ( ita->second->top()->subid == nextpending )
							{
								CompressBuffer * buffer = ita->second->top();
								ita->second->pop();			
								towrite.push_back(buffer);
							}
						}
					}
					
					// xxx enque writes
				}
				
				// add a compressed block ready for writing
				virtual void putAddPendingCompressBufferWrite(CompressBuffer * buffer)
				{
					{
						libmaus::parallel::ScopePosixSpinLock lwritePendingLock(writePendingLock);
	
						std::map<uint64_t, libmaus::parallel::LockedHeap<CompressBuffer *, CompressBufferHeapComparator> * >::iterator
							ita = writePending.find(buffer->blockid);
						
						if ( ita == writePending.end() )
						{
							writePending[buffer->blockid] = 0;
							ita = writePending.find(buffer->blockid);
							ita->second = writePendingHeapFreeList.get();
							
							{
								libmaus::parallel::ScopePosixSpinLock lwritePendingNextOutLock(writePendingNextOutLock);
								writePendingNextOut[buffer->blockid] = 0;
							}
							{
								libmaus::parallel::ScopePosixSpinLock lwritePendindUnfinishedLock(writePendindUnfinishedLock);
								writePendingUnfinished[buffer->blockid] = buffer->totalsubids;
							}
						}
						
						ita->second->push(buffer);
						
						std::cerr << "added write pending for " << buffer->blockid << " " << buffer->subid << std::endl;
					}
					
					checkPendingWrite();
				}
				
				virtual void putReturnRewriteBuffer(AlignmentRewriteBuffer * buffer)
				{
					// return buffer
					buffer->reset();
					rewriteBlockFreeList.put(buffer);
					// check if we can process another buffer now
					checkRewritePending();			
				}
				
				void checkCompressObjectPending()
				{
					bool running = true;
					while ( running )
					{
						libmaus::parallel::ScopePosixSpinLock lcompressObjectPendingLock(compressObjectPendingLock);
						
						// get compress buffer
						CompressBuffer * compbuf = 0; 
						running = running && ((compbuf = compblockfreelist.getIf()) != 0);
						
						// get pending object
						CompressionPendingElement * pend = 0;
						running = running && compressObjectPending.tryDequeFront(pend);
						
						if ( ! running )
						{
							if ( pend )
								compressObjectPending.push_front(pend);
							if ( compbuf )
								compblockfreelist.put(compbuf);
						}
						else
						{
							// enque work package
							AlignmentBlockCompressPackage * package = blockCompressPackages.getPackage();
	
							*package = AlignmentBlockCompressPackage(
								0, /* priority */
								compbuf,
								pend,
								blockCompressDispatcherId
							);
							
							STP.enque(package);
						}
					}
				}
				
				virtual void putReturnCompressionPendingElement(CompressionPendingElement * pend)
				{
					compressObjectPending.push_back(pend);
				}
	
				virtual void putSortFinished(AlignmentRewriteBuffer * buffer)
				{
					typename AlignmentRewritePosSortContext<order_type>::shared_ptr_type context;
					
					{
						libmaus::parallel::ScopePosixSpinLock slock(sortActiveLock);
						typename std::map < AlignmentRewriteBuffer *, typename AlignmentRewritePosSortContext<order_type>::shared_ptr_type >::iterator ita =
							sortActive.find(buffer);
						assert ( ita != sortActive.end() );
						context = ita->second;
						sortActive.erase(ita);
					}
					
					#if 0
					{
						uint64_t const f = buffer->fill();
						for ( uint64_t i = 1; i < f; ++i )
						{
							uint8_t const * pa = buffer->A.begin() + buffer->pP[i-1] + sizeof(uint32_t) + sizeof(uint64_t);
							uint8_t const * pb = buffer->A.begin() + buffer->pP[  i] + sizeof(uint32_t) + sizeof(uint64_t);
	
							int32_t const refa = ::libmaus::bambam::BamAlignmentDecoderBase::getRefID(pa);
							int32_t const refb = ::libmaus::bambam::BamAlignmentDecoderBase::getRefID(pb);
				
							if ( refa != refb )
							{
								assert ( static_cast<uint32_t>(refa) < static_cast<uint32_t>(refb) );
							}
							else
							{
								int32_t const posa = ::libmaus::bambam::BamAlignmentDecoderBase::getPos(pa);
								int32_t const posb = ::libmaus::bambam::BamAlignmentDecoderBase::getPos(pb);
								
								assert ( posa <= posb );
							}
						}
					}
					#endif
					
					readsSorted += buffer->fill();
	
					// zzzyyy
					
					// divide rewrite buffer into blocks of reads
					uint64_t const f = buffer->fill();
					uint64_t zf = 0;
					
					while ( zf < f )
					{
						uint64_t s = 0;
						uint64_t rzf = zf;
						while ( zf < f && (s + buffer->lengthAt(zf) + sizeof(uint32_t) <= compblocksize) )
							s += buffer->lengthAt(zf++) + sizeof(uint32_t);
							
						buffer->blocksizes.push_back(zf-rzf);
					
						// make sure we have at least one read (otherwise buffer is too small for data)
						assert ( zf != rzf );
					}
	
					// number of blocks produced
					uint64_t const numsubblocks = buffer->blocksizes.size();
	
					// get super block id
					uint64_t superblockid = 0;
					{
						libmaus::parallel::ScopePosixSpinLock SPSL(nextOutputSuperBlockLock);
						superblockid = nextOutputSuperBlock++;
					}
	
					// compute prefix sums over block sizes
					uint64_t blockacc = 0;
					for ( uint64_t i = 0; i < buffer->blocksizes.size(); ++i )
					{
						uint64_t const t = buffer->blocksizes[i];
						buffer->blocksizes[i] = blockacc;
						blockacc += t;
					}
					// push total sum
					buffer->blocksizes.push_back(blockacc);
					
					#if 0
					for ( uint64_t i = 0; i < numsubblocks; ++i )
					{
						libmaus::parallel::ScopePosixSpinLock lcompressObjectPendingLock(compressObjectPendingLock);
						
						CompressionPendingElement * pending = compressObjectPendingElementFreeList.get();
						*pending = CompressionPendingElement(buffer,superblockid,i,numsubblocks);
						compressObjectPending.push_back(pending);
					}
					#endif
	
					putReturnRewriteBuffer(buffer);
			
					numSortBlocksOut++;
					
					if ( 
						lastParseBlockRewritten.get()
						&&
						static_cast<uint64_t>(numSortBlocksIn) == static_cast<uint64_t>(numSortBlocksOut)
					)
					{
						lastSortBlockFinished.set(true);
					}
				}
	
				virtual void putMergeSortWorkPackage(AlignmentRewritePosMergeSortPackage<order_type> * package)
				{
					mergeSortPackages.returnPackage(package);
				}
	
				virtual void putBaseSortWorkPackage(AlignmentRewritePosSortBaseSortPackage<order_type> * package)
				{
					baseSortPackages.returnPackage(package);
				}
	
				virtual void putReinsertAlignmentBuffer(AlignmentBuffer * buffer)
				{
					#if 0
					STP.getGlobalLock().lock();
					std::cerr << "reinserting alignment buffer" << std::endl;
					STP.getGlobalLock().unlock();
					#endif
					
					// reinsert into pending queue
					rewritePending.push_front(buffer);
					// check if we can process this buffer
					checkRewritePending();
				}
	
				/*
				 * add a pending full rewrite buffer
				 */
				virtual void putAlignmentRewriteBufferAddPending(AlignmentRewriteBuffer * buffer)
				{
					// zzz process buffer here
					#if 0
					STP.getGlobalLock().lock();
					std::cerr << "finished rewrite buffer of size " << buffer->fill() << std::endl;
					STP.getGlobalLock().unlock();
					#endif
	
					{
						libmaus::parallel::PosixSpinLock lreadsRewrittenLock(readsRewrittenLock);
						readsRewritten += buffer->fill();
					}
	
					typename AlignmentRewritePosSortContext<order_type>::shared_ptr_type sortContext(
						new AlignmentRewritePosSortContext<order_type>(
							buffer,
							STP.getNumThreads(),
							STP,
							mergeSortPackages,
							mergeSortDispatcherId,
							*this
						)
					);
					
					{
					libmaus::parallel::ScopePosixSpinLock slock(sortActiveLock);
					sortActive[buffer] = sortContext;
					}
					
					numSortBlocksIn++;
					sortContext->enqueBaseSortPackages(baseSortPackages,baseSortDispatcherId);
				}
	
				/*
				 * reinsert a rewrite buffer for more filling
				 */
				virtual void putAlignmentRewriteBufferReinsertForFilling(AlignmentRewriteBuffer * buffer)
				{
					#if 0
					STP.getGlobalLock().lock();
					std::cerr << "reinserting rewrite buffer" << std::endl;
					STP.getGlobalLock().unlock();
					#endif
					
					// return buffer
					rewriteBlockFreeList.put(buffer);
					// check whether we can process another one
					checkRewritePending();
				}
				
				/*
				 * return a fully processed parse buffer
				 */
				virtual void putReturnAlignmentBuffer(AlignmentBuffer * buffer)
				{
					#if 0
					STP.getGlobalLock().lock();
					std::cerr << "parse buffer finished" << std::endl;
					STP.getGlobalLock().unlock();
					#endif
					
					parseBlocksRewritten += 1;
					
					if ( lastParseBlockValidated.get() && static_cast<uint64_t>(parseBlocksRewritten) == static_cast<uint64_t>(parseBlocksValidated) )
					{
						lastParseBlockRewritten.set(true);
						
						// check whether any rewrite blocks are in the free list
						std::vector < AlignmentRewriteBuffer * > reblocks;
						while ( ! rewriteBlockFreeList.empty() )
							reblocks.push_back(rewriteBlockFreeList.get());
	
						// number of blocks queued for sorting
						uint64_t queued = 0;
						
						for ( uint64_t i = 0; i < reblocks.size(); ++i )
							if ( reblocks[i]->fill() )
							{
								reblocks[i]->reorder();
								putAlignmentRewriteBufferAddPending(reblocks[i]);
								queued++;
							}
							else
							{
								rewriteBlockFreeList.put(reblocks[i]);
							}
						
						#if 0
						STP.getGlobalLock().lock();
						std::cerr << "all parse buffers finished" << std::endl;
						STP.getGlobalLock().unlock();
						#endif
						
						// ensure at least one block is passed so lastParseBlockRewritten==true is observed downstream
						if ( ! queued )
						{
							libmaus::parallel::PosixSpinLock lreadsParsed(readsParsedLock);
	
							assert ( reblocks.size() );
							assert ( ! reblocks[0]->fill() );
							reblocks[0]->reorder();
							putAlignmentRewriteBufferAddPending(reblocks[0]);
						}
					}
							
					buffer->reset();
					parseBlockFreeList.put(buffer);
					checkParsePendingList();
				}
	
				virtual void putReturnValidatePackage(ValidateBlockWorkPackage * package)
				{
					validateWorkPackages.returnPackage(package);
				}
	
				virtual void putReturnParsePackage(ParseBlockWorkPackage * package)
				{
					parseWorkPackages.returnPackage(package);
				}
	
				virtual void putReturnRewritePackage(RewriteBlockWorkPackage * package)
				{
					rewriteWorkPackages.returnPackage(package);
				}
	
				virtual void putParsedBlockStall(AlignmentBuffer * algn)
				{
					{
						libmaus::parallel::ScopePosixSpinLock slock(parseStallSlotLock);
						
						#if 0
						{
						std::ostringstream ostr;
						ostr << "setting stall slot to " << algn;
						STP.addLogStringWithThreadId(ostr.str());
						}
						#endif
						
						if ( parseStallSlot )
							STP.printLog(std::cerr );
										
						assert ( parseStallSlot == 0 );
						parseStallSlot = algn;
					}
					
					checkParsePendingList();
				}
	
				// return a read work package to the free list
				void putInputBlockWorkPackage(InputBlockWorkPackage * package)
				{
					readWorkPackages.returnPackage(package);
				}
	
				// return decompress block work package
				virtual void putDecompressBlockWorkPackage(DecompressBlockWorkPackage * package)
				{
					decompressWorkPackages.returnPackage(package);
				}
	
				// check whether we can decompress packages			
				void checkDecompressPendingList()
				{
					bool running = true;
					
					while ( running )
					{
						// XXX ZZZ lock for decompressPending
						ControlInputInfo::input_block_type * inputblock = 0;
						libmaus::lz::BgzfInflateZStreamBase * decoder = 0;
						decompressed_block_type * outputblock = 0;
						
						bool ok = true;
						ok = ok && decompressPending.tryDequeFront(inputblock);
						ok = ok && ((decoder = decoders.getIf()) != 0);
						ok = ok && ((outputblock = decompressedBlockFreeList.getIf()) != 0);
						
						if ( ok )
						{
							DecompressBlockWorkPackage * package = decompressWorkPackages.getPackage();
							*package = DecompressBlockWorkPackage(0,inputblock,outputblock,decoder,decompressDispatcherId);
							STP.enque(package);
						}
						else
						{
							running = false;
							
							if ( outputblock )
								decompressedBlockFreeList.put(outputblock);
							if ( decoder )
								decoders.put(decoder);
							if ( inputblock )
								decompressPending.push_front(inputblock);
						}
					}
				}
	
				// add a compressed bgzf block to the pending list
				void putInputBlockAddPending(ControlInputInfo::input_block_type * block)
				{
					// put package in the pending list
					decompressPending.push_back(block);
					// check whether we have sufficient resources to decompress next package
					checkDecompressPendingList();
				}
	
				void checkParsePendingList()
				{
					libmaus::parallel::ScopePosixSpinLock slock(nextDecompressedBlockToBeParsedLock);
					libmaus::parallel::ScopePosixSpinLock alock(parseStallSlotLock);
					libmaus::parallel::ScopePosixSpinLock plock(parsePendingLock);
	
					AlignmentBuffer * algnbuffer = 0;
	
					#if 0
					{
					std::ostringstream ostr;
					ostr << "checkParsePendingList stall slot " << parseStallSlot;
					STP.addLogStringWithThreadId(ostr.str());
					}
					#endif
									
					if ( 
						parsePending.size() &&
						(parsePending.top().first == nextDecompressedBlockToBeParsed) &&
						(algnbuffer=parseStallSlot)
					)
					{					
						parseStallSlot = 0;
											
						DecompressedPendingObject obj = parsePending.pop();		
	
						#if 0
						STP.addLogStringWithThreadId("erasing stall slot for block id" + libmaus::util::NumberSerialisation::formatNumber(obj.second->blockid,0));
						#endif
						
						ParseBlockWorkPackage * package = parseWorkPackages.getPackage();
						*package = ParseBlockWorkPackage(
							0 /* prio */,
							obj.second,
							algnbuffer,
							&parseInfo,
							parseDispatcherId
						);
						STP.enque(package);
					}
					else if ( 
						parsePending.size() && 
						(parsePending.top().first == nextDecompressedBlockToBeParsed) &&
						(algnbuffer = parseBlockFreeList.getIf())
					)
					{
						DecompressedPendingObject obj = parsePending.pop();		
						
						#if 0
						STP.addLogStringWithThreadId("using free block for block id" + libmaus::util::NumberSerialisation::formatNumber(obj.second->blockid,0));
						#endif
											
						ParseBlockWorkPackage * package = parseWorkPackages.getPackage();
						*package = ParseBlockWorkPackage(
							0 /* prio */,
							obj.second,
							algnbuffer,
							&parseInfo,
							parseDispatcherId
						);
						STP.enque(package);
					}
					#if 0			
					else
					{
						STP.addLogStringWithThreadId("checkParsePendingList no action");			
					}
					#endif
				}
	
				// return a decompressed block after parsing
				virtual void putDecompressedBlockReturn(DecompressedBlock * block)
				{
					#if 0
					STP.addLogStringWithThreadId("returning block " + libmaus::util::NumberSerialisation::formatNumber(block->blockid,0));
					#endif
				
					// add block to the free list
					decompressedBlockFreeList.put(block);
	
					// ready for next block
					{
					libmaus::parallel::ScopePosixSpinLock slock(nextDecompressedBlockToBeParsedLock);
					nextDecompressedBlockToBeParsed += 1;
					}
	
					// check whether we have sufficient resources to decompress next package
					checkDecompressPendingList();			
	
					// see if we can parse the next block now
					checkParsePendingList();
				}
				
				virtual void putParsedBlockAddPending(AlignmentBuffer * algn)
				{
					parseBlocksSeen += 1;
						
					if ( algn->final )
						lastParseBlockSeen.set(true);
				
					ValidateBlockWorkPackage * package = validateWorkPackages.getPackage();
					*package = ValidateBlockWorkPackage(
						0 /* prio */,
						algn,
						validateDispatcherId
					);
					STP.enque(package);
				}
	
				void checkRewritePending()
				{
					AlignmentBuffer * inbuffer = 0;
					bool const pendingok = rewritePending.tryDequeFront(inbuffer);
					AlignmentRewriteBuffer * outbuffer = rewriteBlockFreeList.getIf();
					
					if ( pendingok && (outbuffer != 0) )
					{
						#if 0
						STP.getGlobalLock().lock();
						std::cerr << "checkRewritePending" << std::endl;
						STP.getGlobalLock().unlock();
						#endif
											
						RewriteBlockWorkPackage * package = rewriteWorkPackages.getPackage();
	
						*package = RewriteBlockWorkPackage(
							0 /* prio */,
							inbuffer,
							outbuffer,
							rewriteDispatcherId
						);
	
						STP.enque(package);
					}
					else
					{
						if ( outbuffer )
							rewriteBlockFreeList.put(outbuffer);
						if ( pendingok )
							rewritePending.push_front(inbuffer);
					}
				}
	
				virtual void putValidatedBlockAddPending(AlignmentBuffer * algn, bool const ok)
				{
					if ( ! ok )
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "Input validation failed.\n";
						lme.finish();
						throw lme;
					}
				
					readsParsed += algn->fill();
	
					#if 0
					STP.addLogStringWithThreadId("seen " + libmaus::util::NumberSerialisation::formatNumber(readsParsed,0));	
					#endif
	
					{
						libmaus::parallel::ScopePosixSpinLock slock(readsParsedLock);
						
						if ( static_cast<uint64_t>(readsParsed) / (1024*1024) != readsParsedLastPrint/(1024*1024) )
						{
							STP.getGlobalLock().lock();				
							std::cerr << "seen " << readsParsed << " low " << algn->low << std::endl;
							STP.getGlobalLock().unlock();
						}
						
						readsParsedLastPrint = static_cast<uint64_t>(readsParsed);
					}
					
					parseBlocksValidated += 1;
					if ( lastParseBlockSeen.get() && parseBlocksValidated == parseBlocksSeen )
						lastParseBlockValidated.set(true);
						
					rewritePending.push_back(algn);
					
					checkRewritePending();
				}
	
				// add decompressed block to pending list
				virtual void putDecompressedBlockAddPending(DecompressedBlock * block)
				{
					{
					libmaus::parallel::ScopePosixSpinLock plock(parsePendingLock);
					parsePending.push(DecompressedPendingObject(block->blockid,block));
					}
	
					checkParsePendingList();
				}
	
				// return a bgzf decoder object		
				virtual void putBgzfInflateZStreamBaseReturn(libmaus::lz::BgzfInflateZStreamBase * decoder)
				{
					// add decoder to the free list
					decoders.put(decoder);
					// check whether we have sufficient resources to decompress next package
					checkDecompressPendingList();
				}
	
				// return input block after decompression
				virtual void putInputBlockReturn(ControlInputInfo::input_block_type * block)
				{
					// put package in the free list
					inputinfo.inputBlockFreeList.put(block);
	
					// enque next read
					InputBlockWorkPackage * package = readWorkPackages.getPackage();
					*package = InputBlockWorkPackage(0 /* priority */, &inputinfo,readDispatcherId);
					STP.enque(package);
				}
				
				Control(
					libmaus::parallel::SimpleThreadPool & rSTP, 
					std::istream & ristr, 
					uint64_t const rstreamid,
					uint64_t const rparsebuffersize,
					uint64_t const rrewritebuffersize,
					uint64_t const rnumrewritebuffers,
					libmaus::parallel::LockedGrowingFreeList<libmaus::lz::CompressorObject, libmaus::lz::CompressorObjectFreeListAllocator> & rcompfreelist,
					uint64_t const numcompblocks,
					uint64_t const rcompblocksize,
					std::string const & rtempfileprefix
				)
				: 
					STP(rSTP),
					tempfileprefix(rtempfileprefix),
					tempfilesyncid(0),
					tmpfilefreelist(STP.getNumThreads(),TemporaryFileAllocator<libmaus::aio::PosixFdOutputStream>(tempfileprefix,&tempfilesyncid)),
					compfreelist(rcompfreelist),
					readDispatcher(*this,*this),
					readDispatcherId(STP.getNextDispatcherId()),
					decompressDispatcher(*this,*this,*this,*this),
					decompressDispatcherId(STP.getNextDispatcherId()),
					parseDispatcher(*this,*this,*this,*this,*this),
					parseDispatcherId(STP.getNextDispatcherId()),
					validateDispatcher(*this,*this),
					validateDispatcherId(STP.getNextDispatcherId()),
					rewriteDispatcher(*this,*this,*this,*this,*this),
					rewriteDispatcherId(STP.getNextDispatcherId()),
					baseSortDispatcher(*this),
					baseSortDispatcherId(STP.getNextDispatcherId()),
					mergeSortDispatcher(*this),
					mergeSortDispatcherId(STP.getNextDispatcherId()),
					blockCompressDispatcher(compfreelist,*this,*this,*this),
					blockCompressDispatcherId(STP.getNextDispatcherId()),
					numthreads(STP.getNumThreads()),
					inputinfo(ristr,rstreamid,numthreads),
					readWorkPackages(),
					Pdecoders(new libmaus::parallel::LockedFreeList<libmaus::lz::BgzfInflateZStreamBase>(numthreads)),
					decoders(*Pdecoders),
					decompressedBlockFreeList(numthreads),
					parseBlockFreeList(numthreads,AlignmentBufferAllocator(rparsebuffersize,2)),
					rewriteBlockFreeList(rnumrewritebuffers,AlignmentRewriteBufferAllocator(rrewritebuffersize,2)),
					nextDecompressedBlockToBeParsed(0),
					parseStallSlot(0),
					lastParseBlockSeen(false),
					readsParsed(0),
					readsParsedLastPrint(0),
					parseBlocksSeen(0),
					parseBlocksValidated(0),
					lastParseBlockValidated(false),
					parseBlocksRewritten(0),
					lastParseBlockRewritten(false),
					numSortBlocksIn(0),
					numSortBlocksOut(0),
					lastSortBlockFinished(false),
					compblocksize(rcompblocksize),
					compblockfreelist(numcompblocks,CompressBufferAllocator(compblocksize))
				{
					STP.registerDispatcher(readDispatcherId,&readDispatcher);
					STP.registerDispatcher(decompressDispatcherId,&decompressDispatcher);
					STP.registerDispatcher(parseDispatcherId,&parseDispatcher);
					STP.registerDispatcher(validateDispatcherId,&validateDispatcher);
					STP.registerDispatcher(rewriteDispatcherId,&rewriteDispatcher);
					STP.registerDispatcher(baseSortDispatcherId,&baseSortDispatcher);
					STP.registerDispatcher(mergeSortDispatcherId,&mergeSortDispatcher);
					STP.registerDispatcher(blockCompressDispatcherId,&blockCompressDispatcher);
				
					// get temp file names	
					std::vector<named_temp_file_type *> tempvec;
					while (! tmpfilefreelist.empty() )
						tempvec.push_back(tmpfilefreelist.get());
					for ( uint64_t i = 0; i < tempvec.size(); ++i )
					{
						tmpfilefreelist.put(tempvec[i]);
						tempfileidtoname[tempvec[i]->getId()] = tempvec[i]->getName();
					}
				}
				
				void serialTestDecode(std::ostream & out)
				{
					libmaus::timing::RealTimeClock rtc; rtc.start();
				
					bool running = true;
					ParseInfo BPDP;
					// AlignmentBuffer BPDAB(2*1024ull*1024ull*1024ull,2);
					// AlignmentBuffer BPDAB(1024ull*1024ull,2);
					AlignmentBuffer BPDAB(1024ull*1024ull,2);
					uint64_t pcnt = 0;
					uint64_t cnt = 0;
					
					// read blocks until no more input available
					while ( running )
					{
						ControlInputInfo::input_block_type * inblock = inputinfo.inputBlockFreeList.get();	
						inblock->readBlock(inputinfo.istr);
	
						decompressed_block_type * outblock = decompressedBlockFreeList.get();
											
						outblock->decompressBlock(decoders,*inblock);
	
						running = running && (!(outblock->final));
	
						while ( ! BPDP.parseBlock(*outblock,BPDAB) )
						{
							//BPDAB.checkValidUnpacked();
							BPDAB.reorder();
							// BPDAB.checkValidPacked();
							
							BPDP.putBackLastName(BPDAB);
	
							cnt += BPDAB.fill();
									
							BPDAB.reset();
							
							if ( cnt / (1024*1024) != pcnt / (1024*1024) )
							{
								std::cerr << "cnt=" << cnt << " als/sec=" << (static_cast<double>(cnt) / rtc.getElapsedSeconds()) << std::endl;
								pcnt = cnt;			
							}
						}
											
						// out.write(outblock->P,outblock->uncompdatasize);
						
						decompressedBlockFreeList.put(outblock);
						inputinfo.inputBlockFreeList.put(inblock);	
					}
	
					while ( ! BPDP.putBackBufferEmpty() )
					{				
						decompressed_block_type * outblock = decompressedBlockFreeList.get();
						assert ( ! outblock->uncompdatasize );
						
						while ( ! BPDP.parseBlock(*outblock,BPDAB) )
						{
							//BPDAB.checkValidUnpacked();
							BPDAB.reorder();
							// BPDAB.checkValidPacked();
							
							BPDP.putBackLastName(BPDAB);
	
							cnt += BPDAB.fill();
							
							BPDAB.reset();
	
							if ( cnt / (1024*1024) != pcnt / (1024*1024) )
							{
								std::cerr << "cnt=" << cnt << " als/sec=" << (static_cast<double>(cnt) / rtc.getElapsedSeconds()) << std::endl;
								pcnt = cnt;			
							}
						}
												
						decompressedBlockFreeList.put(outblock);
					}
	
					BPDAB.reorder();
					cnt += BPDAB.fill();
					BPDAB.reset();
					
					std::cerr << "cnt=" << cnt << " als/sec=" << (static_cast<double>(cnt) / rtc.getElapsedSeconds()) << std::endl;
				}
				
				static void serialTestDecode1(std::istream & in, std::ostream & out)
				{
					libmaus::parallel::SimpleThreadPool STP(1);
					Control BPDC(STP,in,0/*streamid*/,1024*1024 /* parse buffer size */,1024*1024*1024 /* rewrite buffer size */,4);
					BPDC.serialTestDecode(out);
				}
				
				void enqueReadPackage()
				{
					InputBlockWorkPackage * package = readWorkPackages.getPackage();
					*package = InputBlockWorkPackage(0 /* priority */, &inputinfo,readDispatcherId);
					STP.enque(package);
				}
	
				static Control * sigobj;
	
				static void sigusr1(int)
				{
					std::cerr << "decompressPending.size()=" << sigobj->decompressPending.size() << "\n";
					std::cerr << "parsePending.size()=" << sigobj->parsePending.size() << "\n";
					std::cerr << "rewritePending.size()=" << sigobj->rewritePending.size() << "\n";
					std::cerr << "decoders.empty()=" << sigobj->decoders.empty() << "\n";
					std::cerr << "decompressedBlockFreeList.empty()=" << sigobj->decompressedBlockFreeList.empty() << "\n";
					std::cerr << "parseBlockFreeList.empty()=" << sigobj->parseBlockFreeList.empty() << "\n";
					std::cerr << "rewriteBlockFreeList.empty()=" << sigobj->rewriteBlockFreeList.empty() << "\n";
					sigobj->STP.printStateHistogram(std::cerr);
				}
	
				static void serialParallelDecode1(std::istream & in)
				{
					try
					{
						// libmaus::parallel::SimpleThreadPool STP(12);
						std::string const tempfileprefix = "temp";
						uint64_t const numthreads = 24;
						libmaus::parallel::SimpleThreadPool STP(numthreads);
						libmaus::lz::CompressorObjectFreeListAllocator::unique_ptr_type compalloc(
							libmaus::lz::CompressorObjectFreeListAllocatorFactory::construct("snappy")
						);
						libmaus::parallel::LockedGrowingFreeList<libmaus::lz::CompressorObject, libmaus::lz::CompressorObjectFreeListAllocator> compfreelist(
							*compalloc
						);
						Control BPDC(
							STP,in,0/*streamid*/,1024*1024 /* parse buffer size */, 1024*1024*1024 /* rewrite buffer size */,4,compfreelist, 2*numthreads /* num comp blocks */,64*1024, /* comp block size */
							tempfileprefix
						);
						
						sigobj = & BPDC;
						signal(SIGUSR1,sigusr1);
						
						BPDC.enqueReadPackage();
					
						while ( (!STP.isInPanicMode()) && (! BPDC.lastSortBlockFinished.get()) )
						{
							sleep(1);
						}
					
						STP.terminate();
					
						std::cerr << "number of reads parsed    " << BPDC.readsParsed << std::endl;
						std::cerr << "number of reads rewritten " << BPDC.readsRewritten << std::endl;
						std::cerr << "number of reads sorted " << BPDC.readsSorted << std::endl;
					}
					catch(std::exception const & ex)
					{
						std::cerr << ex.what() << std::endl;
					}
				}
			};	
	
			template<>		
			Control<AlignmentRewriteBufferPosComparator> * Control<AlignmentRewriteBufferPosComparator>::sigobj = 0;
		}
	}
}
#endif
