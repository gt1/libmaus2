/*
    libmaus2
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

#include <libmaus2/bambam/parallel/FragmentAlignmentBufferRewriteUpdateInterval.hpp>
#include <libmaus2/bambam/parallel/FragmentAlignmentBufferRewriteWorkPackageDispatcher.hpp>
#include <libmaus2/bambam/parallel/FragmentAlignmentBufferRewriteFragmentCompleteInterface.hpp>
#include <libmaus2/bambam/parallel/FragmentAlignmentBufferRewriteWorkPackageReturnInterface.hpp>
#include <libmaus2/bambam/parallel/FragmentAlignmentBufferRewriteWorkPackage.hpp>
#include <libmaus2/bambam/parallel/WriteBlockWorkPackageDispatcher.hpp>
#include <libmaus2/bambam/parallel/BgzfOutputBlockWrittenInterface.hpp>
#include <libmaus2/bambam/parallel/ReturnBgzfOutputBufferInterface.hpp>
#include <libmaus2/bambam/parallel/WriteBlockWorkPackageReturnInterface.hpp>
#include <libmaus2/bambam/parallel/WritePendingObjectHeapComparator.hpp>
#include <libmaus2/bambam/parallel/WritePendingObject.hpp>
#include <libmaus2/bambam/parallel/BgzfLinearMemCompressWorkPackageDispatcher.hpp>
#include <libmaus2/bambam/parallel/SmallLinearBlockCompressionPendingObjectHeapComparator.hpp>

#include <libmaus2/bambam/parallel/ControlInputInfo.hpp>
#include <libmaus2/bambam/parallel/InputBlockWorkPackageReturnInterface.hpp>
#include <libmaus2/bambam/parallel/InputBlockAddPendingInterface.hpp>
#include <libmaus2/bambam/parallel/InputBlockWorkPackage.hpp>
#include <libmaus2/parallel/NumCpus.hpp>
#include <libmaus2/parallel/SimpleThreadPool.hpp>
#include <libmaus2/parallel/SimpleThreadPoolWorkPackageFreeList.hpp>
#include <libmaus2/bambam/parallel/InputBlockWorkPackageDispatcher.hpp>

#include <libmaus2/bambam/parallel/RequeReadInterface.hpp>
#include <libmaus2/bambam/parallel/DecompressBlocksWorkPackage.hpp>
#include <libmaus2/bambam/parallel/DecompressBlocksWorkPackageDispatcher.hpp>

#include <libmaus2/bambam/parallel/DecompressBlockWorkPackageReturnInterface.hpp>
#include <libmaus2/bambam/parallel/InputBlockReturnInterface.hpp>
#include <libmaus2/bambam/parallel/DecompressedBlockAddPendingInterface.hpp>
#include <libmaus2/bambam/parallel/BgzfInflateZStreamBaseReturnInterface.hpp>
#include <libmaus2/bambam/parallel/DecompressBlockWorkPackageDispatcher.hpp>
#include <libmaus2/parallel/LockedQueue.hpp>
#include <libmaus2/parallel/LockedCounter.hpp>
#include <libmaus2/bambam/parallel/BgzfInflateZStreamBaseGetInterface.hpp>
#include <libmaus2/bambam/parallel/ParseInfo.hpp>
#include <libmaus2/bambam/parallel/DecompressedPendingObject.hpp>
#include <libmaus2/bambam/parallel/DecompressedPendingObjectHeapComparator.hpp>
#include <libmaus2/parallel/LockedHeap.hpp>
#include <libmaus2/bambam/parallel/ParseBlockWorkPackage.hpp>
#include <libmaus2/bambam/parallel/AlignmentBufferAllocator.hpp>
#include <libmaus2/bambam/parallel/AlignmentBufferTypeInfo.hpp>
#include <libmaus2/bambam/parallel/ParseBlockWorkPackageDispatcher.hpp>

#include <libmaus2/bambam/parallel/DecompressedBlockReturnInterface.hpp>
#include <libmaus2/bambam/parallel/GetBgzfDeflateZStreamBaseInterface.hpp>
#include <libmaus2/bambam/parallel/ParsedBlockAddPendingInterface.hpp>
#include <libmaus2/bambam/parallel/ParsedBlockStallInterface.hpp>
#include <libmaus2/bambam/parallel/ParsePackageReturnInterface.hpp>
#include <libmaus2/bambam/parallel/PutBgzfDeflateZStreamBaseInterface.hpp>
#include <libmaus2/bambam/parallel/ValidateBlockFragmentWorkPackage.hpp>
#include <libmaus2/bambam/parallel/ValidationFragment.hpp>
#include <libmaus2/bambam/parallel/ValidateBlockFragmentPackageReturnInterface.hpp>
#include <libmaus2/bambam/parallel/ValidateBlockFragmentAddPendingInterface.hpp>
#include <libmaus2/bambam/parallel/ValidateBlockFragmentWorkPackageDispatcher.hpp>

#include <libmaus2/lz/BgzfDeflateZStreamBase.hpp>
#include <libmaus2/lz/BgzfDeflateOutputBufferBaseTypeInfo.hpp>
#include <libmaus2/lz/BgzfDeflateOutputBufferBaseAllocator.hpp>
#include <libmaus2/lz/BgzfDeflateZStreamBaseTypeInfo.hpp>
#include <libmaus2/lz/BgzfDeflateZStreamBaseAllocator.hpp>
#include <libmaus2/parallel/LockedGrowingFreeList.hpp>
#include <libmaus2/bambam/parallel/AlignmentBufferHeapComparator.hpp>
#include <libmaus2/bambam/parallel/AddWritePendingBgzfBlockInterface.hpp>

#include <libmaus2/bambam/parallel/WriteBlockWorkPackage.hpp>

#include <libmaus2/lz/BgzfInflateZStreamBaseTypeInfo.hpp>
#include <libmaus2/lz/BgzfInflateZStreamBaseAllocator.hpp>

#include <libmaus2/bambam/parallel/DecompressedBlockAllocator.hpp>
#include <libmaus2/bambam/parallel/DecompressedBlockTypeInfo.hpp>

#include <libmaus2/bambam/parallel/FragmentAlignmentBuffer.hpp>
#include <libmaus2/bambam/parallel/FragmentAlignmentBufferAllocator.hpp>
#include <libmaus2/bambam/parallel/FragmentAlignmentBufferTypeInfo.hpp>

#include <libmaus2/bambam/parallel/FragmentAlignmentBufferHeapComparator.hpp>
#include <libmaus2/lz/BgzfDeflate.hpp>

#include <libmaus2/bambam/parallel/ChecksumsInterfaceGetInterface.hpp>
#include <libmaus2/bambam/parallel/ChecksumsInterfacePutInterface.hpp>
			
namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			struct RewriteControl :
				public InputBlockWorkPackageReturnInterface,
				public InputBlockAddPendingInterface,
				public DecompressBlockWorkPackageReturnInterface,
				public InputBlockReturnInterface,
				public DecompressedBlockAddPendingInterface,
				public BgzfInflateZStreamBaseReturnInterface,
				public RequeReadInterface,
				public DecompressBlocksWorkPackageReturnInterface,
				public BgzfInflateZStreamBaseGetInterface,
				public DecompressedBlockReturnInterface,
				public ParsedBlockAddPendingInterface,
				public ParsedBlockStallInterface,
				public ParsePackageReturnInterface,
				public ValidateBlockFragmentPackageReturnInterface,
				public ValidateBlockFragmentAddPendingInterface,
				public GetBgzfDeflateZStreamBaseInterface,
				public SmallLinearBlockCompressionPendingObjectFinishedInterface,
				public PutBgzfDeflateZStreamBaseInterface,
				public AddWritePendingBgzfBlockInterface,
				public BgzfLinearMemCompressWorkPackageReturnInterface,
				public ReturnBgzfOutputBufferInterface,
				public BgzfOutputBlockWrittenInterface,
				public WriteBlockWorkPackageReturnInterface,
				public ParseInfoHeaderCompleteCallback,
				public FragmentAlignmentBufferRewriteWorkPackageReturnInterface,
				public FragmentAlignmentBufferRewriteFragmentCompleteInterface,
				public FragmentAlignmentBufferRewriteUpdateInterval,
				public ChecksumsInterfaceGetInterface,
				public ChecksumsInterfacePutInterface				
			{
				static unsigned int getInputBlockCountShift()
				{
					return 8;
				}
				
				static unsigned int getInputBlockCount()
				{
					return 1ull << getInputBlockCountShift();
				}
				
				std::ostream & out;
							
				libmaus2::parallel::LockedBool decodingFinished;
				
				libmaus2::parallel::SimpleThreadPool & STP;
				libmaus2::parallel::LockedFreeList<
					libmaus2::lz::BgzfInflateZStreamBase,
					libmaus2::lz::BgzfInflateZStreamBaseAllocator,
					libmaus2::lz::BgzfInflateZStreamBaseTypeInfo
				> zstreambases;

				InputBlockWorkPackageDispatcher IBWPD;
                                uint64_t const IBWPDid;
                                DecompressBlocksWorkPackageDispatcher DBWPD;
                                uint64_t const  DBWPDid;
				ParseBlockWorkPackageDispatcher PBWPD;
				uint64_t const PBWPDid;
				ValidateBlockFragmentWorkPackageDispatcher VBFWPD;
				uint64_t const VBFWPDid;				
				BgzfLinearMemCompressWorkPackageDispatcher BLMCWPD;
				uint64_t const BLMCWPDid;
				WriteBlockWorkPackageDispatcher WBWPD;
				uint64_t const WBWPDid;
				FragmentAlignmentBufferRewriteWorkPackageDispatcher FABRWPD;
				uint64_t const FABRWPDid;

				ControlInputInfo controlInputInfo;

				libmaus2::parallel::SimpleThreadPoolWorkPackageFreeList<InputBlockWorkPackage> readWorkPackages;
				libmaus2::parallel::SimpleThreadPoolWorkPackageFreeList<DecompressBlockWorkPackage> decompressBlockWorkPackages;
				libmaus2::parallel::SimpleThreadPoolWorkPackageFreeList<DecompressBlocksWorkPackage> decompressBlocksWorkPackages;
				libmaus2::parallel::SimpleThreadPoolWorkPackageFreeList<ParseBlockWorkPackage> parseBlockWorkPackages;
				libmaus2::parallel::SimpleThreadPoolWorkPackageFreeList<ValidateBlockFragmentWorkPackage> validateBlockFragmentWorkPackages;
				libmaus2::parallel::SimpleThreadPoolWorkPackageFreeList<BgzfLinearMemCompressWorkPackage> bgzfWorkPackages;
				libmaus2::parallel::SimpleThreadPoolWorkPackageFreeList<WriteBlockWorkPackage> writeWorkPackages;
				libmaus2::parallel::SimpleThreadPoolWorkPackageFreeList<FragmentAlignmentBufferRewriteWorkPackage> fragmentAlignmentBufferRewriteWorkPackages;
				
				libmaus2::parallel::LockedQueue<ControlInputInfo::input_block_type::shared_ptr_type> decompressPendingQueue;
				
				libmaus2::parallel::PosixSpinLock decompressLock;
				libmaus2::parallel::LockedFreeList<
					DecompressedBlock,
					DecompressedBlockAllocator,
					DecompressedBlockTypeInfo
				> decompressBlockFreeList;
				
				libmaus2::parallel::LockedCounter bgzfin;
				libmaus2::parallel::LockedCounter bgzfde;
				libmaus2::parallel::LockedBool decompressFinished;
				
				libmaus2::parallel::SynchronousCounter<uint64_t> inputBlockReturnCount;
				libmaus2::parallel::PosixSpinLock requeReadPendingLock;
				libmaus2::parallel::LockedBool requeReadPending;
				
				libmaus2::bambam::parallel::ParseInfo parseInfo;
				libmaus2::autoarray::AutoArray<char> bamHeader;

				// decompressed blocks ready to be parsed
				libmaus2::parallel::LockedHeap<DecompressedPendingObject,DecompressedPendingObjectHeapComparator> parsePending;
				libmaus2::parallel::PosixSpinLock parsePendingLock;

				// id of next block to be parsed
				uint64_t volatile nextDecompressedBlockToBeParsed;
				libmaus2::parallel::PosixSpinLock nextDecompressedBlockToBeParsedLock;

				// stall slot for parsing BAM records
				AlignmentBuffer::shared_ptr_type parseStallSlot;
				libmaus2::parallel::PosixSpinLock parseStallSlotLock;

				// free list for alignment buffers
				libmaus2::parallel::LockedFreeList<AlignmentBuffer,AlignmentBufferAllocator,AlignmentBufferTypeInfo> parseBlockFreeList;

				libmaus2::parallel::LockedBool lastParseBlockSeen;
				libmaus2::parallel::LockedCounter readsParsed;
				libmaus2::parallel::LockedCounter blocksParsed;
				uint64_t volatile readsParsedLastPrint;
				libmaus2::parallel::PosixSpinLock readsParsedLastPrintLock;
				libmaus2::parallel::SynchronousCounter<uint64_t> nextParseBufferId;

				std::map<uint64_t,AlignmentBuffer::shared_ptr_type> validationActive;
				std::map<uint64_t,uint64_t> validationFragmentsPerId;
				std::map<uint64_t,bool> validationOk;
				libmaus2::parallel::PosixSpinLock validationActiveLock;
				libmaus2::parallel::LockedCounter readsValidated;
				libmaus2::parallel::LockedCounter blocksValidated;
				libmaus2::parallel::LockedCounter buffersCompressed;
				libmaus2::parallel::LockedCounter buffersWritten;
				libmaus2::parallel::LockedBool lastParseBlockValidated;

				// finite size free list for output buffers
				libmaus2::parallel::LockedFreeList<
					libmaus2::lz::BgzfDeflateOutputBufferBase,
					libmaus2::lz::BgzfDeflateOutputBufferBaseAllocator,
					libmaus2::lz::BgzfDeflateOutputBufferBaseTypeInfo
					> bgzfDeflateOutputBufferFreeList;
					
				// infinite size free list for bgzf compressor objects
				libmaus2::parallel::LockedGrowingFreeList<
					libmaus2::lz::BgzfDeflateZStreamBase,
					libmaus2::lz::BgzfDeflateZStreamBaseAllocator,
					libmaus2::lz::BgzfDeflateZStreamBaseTypeInfo
				> bgzfDeflateZStreamBaseFreeList;

				libmaus2::parallel::PosixSpinLock rewriteLargeBlockLock;
				std::priority_queue<FragmentAlignmentBuffer::shared_ptr_type, std::vector<FragmentAlignmentBuffer::shared_ptr_type>, 
					FragmentAlignmentBufferHeapComparator > rewriteLargeBlockQueue;
				uint64_t volatile rewriteLargeBlockNext;
				libmaus2::parallel::LockedBool lastParseBlockCompressed;

				libmaus2::parallel::PosixSpinLock compressionActiveBlocksLock;
				std::map<uint64_t, FragmentAlignmentBuffer::shared_ptr_type> compressionActive;
				std::map< int64_t, uint64_t> compressionUnfinished;
				std::priority_queue<SmallLinearBlockCompressionPendingObject,std::vector<SmallLinearBlockCompressionPendingObject>,
					SmallLinearBlockCompressionPendingObjectHeapComparator> compressionUnqueuedPending;

				libmaus2::parallel::PosixSpinLock writePendingCountLock;
				std::map<int64_t,uint64_t> writePendingCount;
				libmaus2::parallel::PosixSpinLock writeNextLock;				
				std::pair<int64_t,uint64_t> writeNext;
				libmaus2::parallel::LockedBool lastParseBlockWritten;
				
				libmaus2::parallel::LockedFreeList<
					FragmentAlignmentBuffer,
					FragmentAlignmentBufferAllocator,
					FragmentAlignmentBufferTypeInfo
				> fragmentBufferFreeList;

				static uint64_t getParseBufferSize()
				{
					return (1ull<<23);
				}

				ChecksumsInterface::shared_ptr_type getSeqChecksumsObject()
				{
					return ChecksumsInterface::shared_ptr_type();
				}
				
				void returnSeqChecksumsObject(ChecksumsInterface::shared_ptr_type)
				{
				}

				RewriteControl(
					libmaus2::parallel::SimpleThreadPool & rSTP,
					std::istream & in,
					int const level,
					std::ostream & rout
				)
				: 
					out(rout),
					decodingFinished(false),
					STP(rSTP), 
					zstreambases(STP.getNumThreads()),
					IBWPD(*this,*this,*this),
					IBWPDid(STP.getNextDispatcherId()),
					DBWPD(*this,*this,*this,*this,*this),
					DBWPDid(STP.getNextDispatcherId()),
					PBWPD(*this,*this,*this,*this,*this),
					PBWPDid(STP.getNextDispatcherId()),
					VBFWPD(*this,*this,*this,*this),
					VBFWPDid(STP.getNextDispatcherId()),
					//
					BLMCWPD(*this,*this,*this,*this,*this),
					BLMCWPDid(STP.getNextDispatcherId()),
					WBWPD(*this,*this,*this),
					WBWPDid(STP.getNextDispatcherId()),
					FABRWPD(*this,*this,*this),
					FABRWPDid(STP.getNextDispatcherId()),
					controlInputInfo(in,0,getInputBlockCount()),
					decompressBlockFreeList(getInputBlockCount() * STP.getNumThreads() * 2),
					bgzfin(0),
					bgzfde(0),
					decompressFinished(false),
					requeReadPending(false),
					parseInfo(this),
					nextDecompressedBlockToBeParsed(0),
					parseStallSlot(),
					parseBlockFreeList(STP.getNumThreads(),AlignmentBufferAllocator(getParseBufferSize(),2 /* pointer multiplier */)),
					lastParseBlockSeen(false),
					readsParsedLastPrint(0),
					lastParseBlockValidated(false),
					bgzfDeflateOutputBufferFreeList(2*STP.getNumThreads(),libmaus2::lz::BgzfDeflateOutputBufferBaseAllocator(level)),
					bgzfDeflateZStreamBaseFreeList(libmaus2::lz::BgzfDeflateZStreamBaseAllocator(level)),
					rewriteLargeBlockNext(0),
					lastParseBlockCompressed(false),
					writeNext(-1,0),
					lastParseBlockWritten(false),
					fragmentBufferFreeList(STP.getNumThreads(),FragmentAlignmentBufferAllocator(STP.getNumThreads(), 2 /* pointer multiplier */))
				{
					STP.registerDispatcher(IBWPDid,&IBWPD);
					STP.registerDispatcher(DBWPDid,&DBWPD);
					STP.registerDispatcher(PBWPDid,&PBWPD);
					STP.registerDispatcher(VBFWPDid,&VBFWPD);
					STP.registerDispatcher(BLMCWPDid,&BLMCWPD);
					STP.registerDispatcher(WBWPDid,&WBWPD);
					STP.registerDispatcher(FABRWPDid,&FABRWPD);
				}

				void enqueReadPackage()
				{
					InputBlockWorkPackage * package = readWorkPackages.getPackage();
					*package = InputBlockWorkPackage(10 /* priority */, &controlInputInfo,IBWPDid);
					STP.enque(package);
				}

				void checkEnqueReadPackage()
				{
					if ( ! controlInputInfo.getEOF() )
						enqueReadPackage();
				}
				
				void requeRead()
				{
					libmaus2::parallel::ScopePosixSpinLock llock(requeReadPendingLock);
					requeReadPending.set(true);
				}
				
				void waitDecompressFinished()
				{
					while ( ! decompressFinished.get() )
						sleep(1);
				}

				void waitDecodingFinished()
				{
					while ( ( ! decodingFinished.get() ) && (!STP.isInPanicMode()) )
					{
						sleep(1);
						// STP.printStateHistogram(std::cerr);
					}
				}
				
				void bamHeaderComplete(libmaus2::bambam::BamHeaderParserState const & BHPS)
				{
					bamHeader = BHPS.getSerialised();
					uint64_t const maxblocksize = libmaus2::lz::BgzfConstants::getBgzfMaxBlockSize();
					uint64_t const headersize = bamHeader.size();
					uint64_t const tnumblocks = (headersize + maxblocksize - 1)/maxblocksize;
					uint64_t const blocksize = (headersize+tnumblocks-1)/tnumblocks;
					uint64_t const numblocks = (headersize+blocksize-1)/blocksize;
					
					{
						libmaus2::parallel::ScopePosixSpinLock lcompressionActiveBlocksLock(compressionActiveBlocksLock);
						compressionUnfinished[-1] = numblocks;
					}
							
					{
						libmaus2::parallel::ScopePosixSpinLock lwritePendingCountLock(writePendingCountLock);
						writePendingCount[-1] = numblocks;
					}
						
					// enque compression requests
					for ( uint64_t i = 0; i < numblocks; ++i )
					{
						uint64_t const ilow = i*blocksize;
						uint64_t const ihigh = std::min(ilow+blocksize,headersize);
						uint8_t * plow  = reinterpret_cast<uint8_t *>(bamHeader.begin()) + ilow;
						uint8_t * phigh = reinterpret_cast<uint8_t *>(bamHeader.begin()) + ihigh;
						
						libmaus2::parallel::ScopePosixSpinLock lcompressionActiveBlocksLock(compressionActiveBlocksLock);
						compressionUnqueuedPending.push(SmallLinearBlockCompressionPendingObject(-1,i,plow,phigh));
					}
				}

				void putBgzfInflateZStreamBaseReturn(libmaus2::lz::BgzfInflateZStreamBase::shared_ptr_type decoder)
				{
					zstreambases.put(decoder);
				}
				
				libmaus2::lz::BgzfInflateZStreamBase::shared_ptr_type getBgzfInflateZStreamBase()
				{
					return zstreambases.get();
				}

				// get bgzf compressor object
				virtual libmaus2::lz::BgzfDeflateZStreamBase::shared_ptr_type getBgzfDeflateZStreamBase()
				{
					libmaus2::lz::BgzfDeflateZStreamBase::shared_ptr_type tptr(bgzfDeflateZStreamBaseFreeList.get());
					return tptr;
				}

				// return bgzf compressor object
				virtual void putBgzfDeflateZStreamBase(libmaus2::lz::BgzfDeflateZStreamBase::shared_ptr_type & ptr)
				{
					bgzfDeflateZStreamBaseFreeList.put(ptr);
				}

				// work package return routines				
				void putInputBlockWorkPackage(InputBlockWorkPackage * package) { readWorkPackages.returnPackage(package); }
				void putDecompressBlockWorkPackage(DecompressBlockWorkPackage * package) { decompressBlockWorkPackages.returnPackage(package); }				
				void putDecompressBlocksWorkPackage(DecompressBlocksWorkPackage * package) { decompressBlocksWorkPackages.returnPackage(package); }
				void putReturnParsePackage(ParseBlockWorkPackage * package) { parseBlockWorkPackages.returnPackage(package); }
				void putReturnValidateBlockFragmentPackage(ValidateBlockFragmentWorkPackage * package) { validateBlockFragmentWorkPackages.returnPackage(package); }
				void returnBgzfLinearMemCompressWorkPackage(BgzfLinearMemCompressWorkPackage * package) { bgzfWorkPackages.returnPackage(package); }
				void returnWriteBlockWorkPackage(WriteBlockWorkPackage * package) { writeWorkPackages.returnPackage(package); }
				void returnFragmentAlignmentBufferRewriteWorkPackage(FragmentAlignmentBufferRewriteWorkPackage * package) { fragmentAlignmentBufferRewriteWorkPackages.returnPackage(package); }
				
				// return input block after decompression
				void putInputBlockReturn(ControlInputInfo::input_block_type::shared_ptr_type block) 
				{ 					
					uint64_t const freecnt = controlInputInfo.inputBlockFreeList.putAndCount(block);
					uint64_t const capacity = controlInputInfo.inputBlockFreeList.capacity();
					
					bool req = false;
					
					{
						libmaus2::parallel::ScopePosixSpinLock llock(requeReadPendingLock);
						bool const pending = requeReadPending.get();
						
						if ( (pending && freecnt >= (capacity>>2)) )
						{
							req = true;
							requeReadPending.set(false);
						}
					}

					if ( req )
						checkEnqueReadPackage();
				}
				
				static uint64_t getDecompressBatchSize()
				{
					return 4;
				}

				void checkEnqueDecompress()
				{
					std::vector<ControlInputInfo::input_block_type::shared_ptr_type> ginblocks;
					std::vector<DecompressedBlock::shared_ptr_type> goutblocks;
					
					{
						libmaus2::parallel::ScopePosixSpinLock plock(decompressPendingQueue.getLock());

						uint64_t const psize = decompressPendingQueue.sizeUnlocked();
							
						if ( ! psize )
							return;
								
						bool const forcerun = decompressPendingQueue.backUnlocked()->final;
						if ( (! forcerun) && (psize < getDecompressBatchSize()) )
							return;
							
						libmaus2::parallel::ScopePosixSpinLock flock(decompressBlockFreeList.getLock());
						uint64_t const fsize = decompressBlockFreeList.freeUnlocked();
						if ( (! forcerun) && (fsize < getDecompressBatchSize()) )
							return;
							
						uint64_t const csize = std::min(psize,fsize);
						for ( uint64_t i = 0; i < csize; ++i )
						{
							ginblocks.push_back(decompressPendingQueue.dequeFrontUnlocked());
							goutblocks.push_back(decompressBlockFreeList.getUnlocked());					
						}
					}
				
					if ( ginblocks.size() )
					{
						std::vector<ControlInputInfo::input_block_type::shared_ptr_type> inblocks;
						std::vector<DecompressedBlock::shared_ptr_type> outblocks;

						uint64_t const loops = (ginblocks.size() + getDecompressBatchSize() - 1)/getDecompressBatchSize();
						
						for ( uint64_t l = 0; l < loops; ++l )
						{
							uint64_t const low = l * getDecompressBatchSize();
							uint64_t const high = std::min(low + getDecompressBatchSize(),static_cast<uint64_t>(ginblocks.size()));
							
							std::vector<ControlInputInfo::input_block_type::shared_ptr_type> inblocks(ginblocks.begin()+low,ginblocks.begin()+high);
							std::vector<DecompressedBlock::shared_ptr_type> outblocks(goutblocks.begin()+low,goutblocks.begin()+high);

							DecompressBlocksWorkPackage * package = decompressBlocksWorkPackages.getPackage();

							package->setData(0 /* prio */,inblocks,outblocks,DBWPDid);

							STP.enque(package);
						}
					}
				}

				void putInputBlockAddPending(
					std::deque<ControlInputInfo::input_block_type::shared_ptr_type>::iterator ita,
					std::deque<ControlInputInfo::input_block_type::shared_ptr_type>::iterator ite)
				{
					bgzfin += ite-ita;
					{
						libmaus2::parallel::ScopePosixSpinLock llock(decompressPendingQueue.getLock());
						for ( 
							std::deque<ControlInputInfo::input_block_type::shared_ptr_type>::iterator itc = ita;
							itc != ite; ++itc )
							decompressPendingQueue.push_backUnlocked(*itc);
					}

					checkEnqueDecompress();
				}

				void putInputBlockAddPending(ControlInputInfo::input_block_type::shared_ptr_type package) 
				{
					bgzfin.increment();
					decompressPendingQueue.push_back(package);
					checkEnqueDecompress();
				}

				void checkParsePendingList()
				{
					libmaus2::parallel::ScopePosixSpinLock slock(nextDecompressedBlockToBeParsedLock);
					libmaus2::parallel::ScopePosixSpinLock alock(parseStallSlotLock);
					libmaus2::parallel::ScopePosixSpinLock plock(parsePendingLock);

					#if 0
					STP.getGlobalLock().lock();
					std::cerr << "check pending, parsePending.size()=" << parsePending.size() << " stall slot " 
						<< parseStallSlot 
						<< " next id "
						<< (parsePending.size() ? parsePending.top().first : -1)
						<< std::endl;
					STP.getGlobalLock().unlock();
					#endif
	
					AlignmentBuffer::shared_ptr_type algnbuffer = AlignmentBuffer::shared_ptr_type();
	
					#if 0
					{
					std::ostringstream ostr;
					ostr << "checkParsePendingList stall slot " << parseStallSlot;
					STP.addLogStringWithThreadId(ostr.str());
					}
					#endif
				
					// is there a buffer in the stall slot?					
					if ( 
						parsePending.size() &&
						(parsePending.top().first == nextDecompressedBlockToBeParsed) &&
						(algnbuffer=parseStallSlot)
					)
					{					
						parseStallSlot = AlignmentBuffer::shared_ptr_type();
											
						DecompressedPendingObject obj = parsePending.pop();		
	
						#if 0
						STP.addLogStringWithThreadId("erasing stall slot for block id" + libmaus2::util::NumberSerialisation::formatNumber(obj.second->blockid,0));
						#endif
						
						ParseBlockWorkPackage * package = parseBlockWorkPackages.getPackage();
						*package = ParseBlockWorkPackage(
							0 /* prio */,
							obj.second,
							algnbuffer,
							&parseInfo,
							PBWPDid
						);
						STP.enque(package);
					}
					// do we have a buffer in the free list
					else if ( 
						parsePending.size() && 
						(parsePending.top().first == nextDecompressedBlockToBeParsed) &&
						(algnbuffer = parseBlockFreeList.getIf())
					)
					{
						algnbuffer->reset();
						algnbuffer->id = nextParseBufferId++;
					
						DecompressedPendingObject obj = parsePending.pop();		
						
						#if 0
						STP.addLogStringWithThreadId("using free block for block id" + libmaus2::util::NumberSerialisation::formatNumber(obj.second->blockid,0));
						#endif
											
						ParseBlockWorkPackage * package = parseBlockWorkPackages.getPackage();
						*package = ParseBlockWorkPackage(
							0 /* prio */,
							obj.second,
							algnbuffer,
							&parseInfo,
							PBWPDid
						);
						STP.enque(package);
					}
					#if 1
					else
					{
						STP.addLogStringWithThreadId("checkParsePendingList no action");			
					}
					#endif
				}

				void putDecompressedBlockReturn(libmaus2::bambam::parallel::DecompressedBlock::shared_ptr_type block)
				{
					decompressBlockFreeList.put(block);
					checkEnqueDecompress();				

					{
					libmaus2::parallel::ScopePosixSpinLock slock(nextDecompressedBlockToBeParsedLock);
					nextDecompressedBlockToBeParsed += 1;
					}
					
					checkParsePendingList();
				}

				void putParsedBlockStall(AlignmentBuffer::shared_ptr_type algn)
				{
					{
						libmaus2::parallel::ScopePosixSpinLock slock(parseStallSlotLock);
						
						// parse stall slot should be empty
						if ( parseStallSlot )
							STP.printLog(std::cerr);
										
						assert ( parseStallSlot == 0 );

						parseStallSlot = algn;
					}
					
					checkParsePendingList();
				}

				void putParsedBlockAddPending(AlignmentBuffer::shared_ptr_type algn)
				{
					readsParsed += algn->fill();
					blocksParsed += 1;

					{
						libmaus2::parallel::ScopePosixSpinLock sreadsParsedLastPrintLock(readsParsedLastPrintLock);
						
						if ( ((readsParsed >> 20) != (readsParsedLastPrint >> 20)) || algn->final )
						{
							libmaus2::parallel::ScopePosixSpinLock SPSL(STP.getGlobalLock());
							std::cerr << readsParsed << std::endl;
							readsParsedLastPrint = readsParsed;
						}				
					}
					
					if ( algn->final )
						lastParseBlockSeen.set(true);
					
					uint64_t const f = algn->fill();
					uint64_t const readsPerPackage = std::max((f + STP.getNumThreads() - 1)/STP.getNumThreads(),static_cast<uint64_t>(1));
					uint64_t const validationPackages = std::max((f + readsPerPackage - 1)/readsPerPackage, static_cast<uint64_t>(1));
					
					{
						libmaus2::parallel::ScopePosixSpinLock llock(validationActiveLock);
						validationActive[algn->id] = algn;
						validationFragmentsPerId[algn->id] = validationPackages;
						validationOk[algn->id] = true;
					}

					for ( uint64_t p = 0; p < validationPackages; ++p )
					{
						uint64_t const low = std::min(p*readsPerPackage,f);
						uint64_t const high = std::min((p+1)*readsPerPackage,f);
						ValidateBlockFragmentWorkPackage * package = validateBlockFragmentWorkPackages.getPackage();
						*package = ValidateBlockFragmentWorkPackage(0/*prio*/,ValidationFragment(low,high,algn),VBFWPDid);
						STP.enque(package);
					}						
				}

				void putDecompressedBlockAddPending(DecompressedBlock::shared_ptr_type block)
				{
					uint64_t cnt = bgzfde.increment();
					
					if ( controlInputInfo.getEOF() && (static_cast<uint64_t>(bgzfin) == cnt) )
						decompressFinished.set(true);

					{
						// put block in pending list
						libmaus2::parallel::ScopePosixSpinLock slock(parsePendingLock);
						parsePending.push(DecompressedPendingObject(block->blockid,block));
					}
					
					checkParsePendingList();					
				}
					
				void smallLinearBlockCompressionPendingObjectFinished(SmallLinearBlockCompressionPendingObject const & obj)
				{
					bool blockfinished = false;
					{
						libmaus2::parallel::ScopePosixSpinLock lcompressionActiveBlocksLock(compressionActiveBlocksLock);

						if ( --compressionUnfinished[obj.blockid] == 0 )
							blockfinished = true;
					}
	
					if ( obj.blockid >= 0 && blockfinished )
					{
						FragmentAlignmentBuffer::shared_ptr_type algn = FragmentAlignmentBuffer::shared_ptr_type();
						{
						libmaus2::parallel::ScopePosixSpinLock lcompressionActiveBlocksLock(compressionActiveBlocksLock);
						algn = compressionActive.find(obj.blockid)->second;
						compressionUnfinished.erase(compressionUnfinished.find(obj.blockid));
						compressionActive.erase(compressionActive.find(obj.blockid));
						}
						
						#if 0
						std::cerr << "block finished " << algn->id << std::endl;
						#endif
						
						buffersCompressed += 1;

						if ( lastParseBlockValidated.get() && blocksValidated == buffersCompressed )
							lastParseBlockCompressed.set(true);

						// ZZZ return FragmentAlignmentBuffer
						fragmentBufferFreeList.put(algn);
						checkValidatedRewritePending();
					}
				
				}
				
				
				std::priority_queue<WritePendingObject,std::vector<WritePendingObject>,WritePendingObjectHeapComparator> writePendingQueue;
				libmaus2::parallel::PosixSpinLock writePendingQueueLock;

				virtual void returnBgzfOutputBufferInterface(libmaus2::lz::BgzfDeflateOutputBufferBase::shared_ptr_type & obuf)
				{
					bgzfDeflateOutputBufferFreeList.put(obuf);
					checkSmallBlockCompressionPending();				
				}
				virtual void bgzfOutputBlockWritten(uint64_t const /* streamid */, int64_t const blockid, uint64_t const subid, uint64_t const /* n */)
				{
					{
						libmaus2::parallel::ScopePosixSpinLock lwritePendingQueueLock(writePendingQueueLock);
						libmaus2::parallel::ScopePosixSpinLock lwriteNextLock(writeNextLock);
						libmaus2::parallel::ScopePosixSpinLock lwritePendingCountLock(writePendingCountLock);
						
						#if 0
						std::cerr << "block written " << blockid << ":" << subid << std::endl;
						#endif
						
						if ( ! -- writePendingCount[blockid] )
						{
							writeNext.first++;
							writeNext.second = 0;
							writePendingCount.erase(writePendingCount.find(blockid));
							
							if ( blockid >= 0 )
							{
								#if 0
								std::cerr << "finished writing block " << blockid << std::endl;
								#endif
							
								buffersWritten++;
							
								if ( lastParseBlockCompressed.get() && buffersWritten == buffersCompressed )
								{
									lastParseBlockWritten.set(true);
									
									// add EOF
									libmaus2::lz::BgzfDeflate<std::ostream> defl(out);
									defl.addEOFBlock();
								}

								if ( lastParseBlockWritten.get() )
									decodingFinished.set(true);		
							}
						}
						else
						{
							writeNext.second++;
						}
					}
					
					checkWritePendingQueue();				
				}
				
				void checkWritePendingQueue()
				{
					libmaus2::parallel::ScopePosixSpinLock lwritePendingQueueLock(writePendingQueueLock);
					libmaus2::parallel::ScopePosixSpinLock lwriteNextLock(writeNextLock);
					if ( 
						writePendingQueue.size() &&
						writePendingQueue.top().blockid == writeNext.first &&
						writePendingQueue.top().subid == writeNext.second
					)
					{
						WriteBlockWorkPackage * package = writeWorkPackages.getPackage();
						*package = WriteBlockWorkPackage(0,writePendingQueue.top(),WBWPDid);
						writePendingQueue.pop();
						STP.enque(package);
						// enque work package
					}					
				}
				
				void addWritePendingBgzfBlock(
					int64_t const blockid,
					int64_t const subid,
					libmaus2::lz::BgzfDeflateOutputBufferBase::shared_ptr_type obuf,
					libmaus2::lz::BgzfDeflateZStreamBaseFlushInfo const & flushinfo
				)
				{
					{
						libmaus2::parallel::ScopePosixSpinLock lwritePendingQueueLock(writePendingQueueLock);
						writePendingQueue.push(WritePendingObject(0 /* streamid */,&out,blockid,subid,obuf,flushinfo));
					}
					
					checkWritePendingQueue();
				}

				void checkSmallBlockCompressionPending()
				{
					bool running = true;
					while ( running )
					{
						libmaus2::lz::BgzfDeflateOutputBufferBase::shared_ptr_type obuf = 
							bgzfDeflateOutputBufferFreeList.getIf();
						
						if ( obuf )
						{
							SmallLinearBlockCompressionPendingObject obj;
							bool haveObject = false;
							
							{
								libmaus2::parallel::ScopePosixSpinLock lcompressionActiveBlocksLock(compressionActiveBlocksLock);
								if ( compressionUnqueuedPending.size() )
								{
									obj = compressionUnqueuedPending.top();
									compressionUnqueuedPending.pop();
									haveObject = true;
								}
							}
							
							if ( haveObject )
							{
								BgzfLinearMemCompressWorkPackage * package = bgzfWorkPackages.getPackage();
								*package = BgzfLinearMemCompressWorkPackage(0,obj,obuf,BLMCWPDid);
								STP.enque(package);								
							}
							else
							{
								bgzfDeflateOutputBufferFreeList.put(obuf);
								running = false;
							}
						}
						else
						{
							running = false;
						}
					}
				}
				
				void checkLargeBlockCompressionPending()
				{
					bool running = true;
					
					while ( running )
					{
						FragmentAlignmentBuffer::shared_ptr_type algn = FragmentAlignmentBuffer::shared_ptr_type();
						
						{
							libmaus2::parallel::ScopePosixSpinLock llock(rewriteLargeBlockLock);
							if ( rewriteLargeBlockQueue.size() && rewriteLargeBlockQueue.top()->id == rewriteLargeBlockNext )
							{
								algn = rewriteLargeBlockQueue.top();
								rewriteLargeBlockQueue.pop();
							}
						}

						if ( algn )
						{	
							{
								libmaus2::parallel::ScopePosixSpinLock lcompressionActiveBlocksLock(compressionActiveBlocksLock);
								compressionActive[algn->id] = algn;
							}
						
							// compute fragments for compression
							std::vector<std::pair<uint8_t *,uint8_t *> > V;
							algn->getLinearOutputFragments(libmaus2::lz::BgzfConstants::getBgzfMaxBlockSize(),V);
							
							{
								libmaus2::parallel::ScopePosixSpinLock lcompressionActiveBlocksLock(compressionActiveBlocksLock);
								compressionUnfinished[algn->id] = V.size();
							}
							
							{
								libmaus2::parallel::ScopePosixSpinLock lwritePendingCountLock(writePendingCountLock);
								writePendingCount[algn->id] = V.size();
							}
						
							// enque compression requests
							for ( uint64_t i = 0; i < V.size(); ++i )
							{
								libmaus2::parallel::ScopePosixSpinLock lcompressionActiveBlocksLock(compressionActiveBlocksLock);
								compressionUnqueuedPending.push(
									SmallLinearBlockCompressionPendingObject(algn->id,i,V[i].first,V[i].second)
								);
							}
							
							libmaus2::parallel::ScopePosixSpinLock llock(rewriteLargeBlockLock);
							++rewriteLargeBlockNext;
						}					
						else
						{
							running = false;
						}
					}
					
					checkSmallBlockCompressionPending();
				}
				
				libmaus2::parallel::LockedCounter nextValidatedBlockToBeRewritten;
				libmaus2::parallel::PosixSpinLock validatedBlocksToBeRewrittenQueueLock;
				std::priority_queue<
					AlignmentBuffer::shared_ptr_type,
					std::vector<AlignmentBuffer::shared_ptr_type>,
					AlignmentBufferHeapComparator
				> validatedBlocksToBeRewrittenQueue;
				
				libmaus2::parallel::PosixSpinLock rewriteActiveLock;
				std::map<uint64_t,AlignmentBuffer::shared_ptr_type> rewriteActiveAlignmentBuffers;
				std::map<uint64_t,FragmentAlignmentBuffer::shared_ptr_type> rewriteActiveFragmentAlignmentBuffers;
				std::map<uint64_t,uint64_t> rewriteActiveCnt;

				void fragmentAlignmentBufferRewriteFragmentComplete(
					AlignmentBuffer::shared_ptr_type & algn,
					FragmentAlignmentBuffer::shared_ptr_type & FAB,
					uint64_t const j
				)
				{
					bool blockFinished = false;
					
					{					
						libmaus2::parallel::ScopePosixSpinLock slock(rewriteActiveLock);
						
						if ( -- rewriteActiveCnt[algn->id] == 0 )
						{
							blockFinished = true;
							rewriteActiveAlignmentBuffers.erase(rewriteActiveAlignmentBuffers.find(algn->id));
							rewriteActiveFragmentAlignmentBuffers.erase(rewriteActiveFragmentAlignmentBuffers.find(algn->id));
							rewriteActiveCnt.erase(rewriteActiveCnt.find(algn->id));
						}
					}
					
					if ( blockFinished )
					{
						{
							libmaus2::parallel::ScopePosixSpinLock llock(rewriteLargeBlockLock);
							rewriteLargeBlockQueue.push(FAB);
						}
							
						checkLargeBlockCompressionPending();

						// return alignment buffer
						parseBlockFreeList.put(algn);
						// check for more parsing
						checkParsePendingList();
					}
				}

				void checkValidatedRewritePending()
				{
					bool running = true;
					
					while ( running )
					{
						AlignmentBuffer::shared_ptr_type algn;

						{
							libmaus2::parallel::ScopePosixSpinLock slock(validatedBlocksToBeRewrittenQueueLock);
							if ( validatedBlocksToBeRewrittenQueue.size() && 
								validatedBlocksToBeRewrittenQueue.top()->id ==
								static_cast<uint64_t>(nextValidatedBlockToBeRewritten) )
							{
								algn = validatedBlocksToBeRewrittenQueue.top();
								validatedBlocksToBeRewrittenQueue.pop();
							}
						}
						
						if ( algn )
						{
							FragmentAlignmentBuffer::shared_ptr_type FAB = fragmentBufferFreeList.getIf();
							
							if ( FAB )
							{
								nextValidatedBlockToBeRewritten += 1;

								uint64_t const f = algn->fill();
								FAB->reset();
								FAB->id = algn->id;
								FAB->subid = algn->subid;
								FAB->checkPointerSpace(f);
								algn->computeSplitPoints(FAB->getOffsetStartVector());
								
								{
									libmaus2::parallel::ScopePosixSpinLock slock(rewriteActiveLock);
									rewriteActiveAlignmentBuffers[algn->id] = algn;
									rewriteActiveFragmentAlignmentBuffers[FAB->id] = FAB;
									rewriteActiveCnt[FAB->id] = FAB->size();
								}
								
								for ( uint64_t j = 0; j < FAB->size(); ++j )
								{
									FragmentAlignmentBufferRewriteWorkPackage * pack = fragmentAlignmentBufferRewriteWorkPackages.getPackage();
									
									*pack = FragmentAlignmentBufferRewriteWorkPackage(
										0 /* prio */,
										algn,
										FAB,
										j,
										FABRWPDid
									);
									
									STP.enque(pack);
								}
							}
							else
							{
								libmaus2::parallel::ScopePosixSpinLock slock(validatedBlocksToBeRewrittenQueueLock);
								validatedBlocksToBeRewrittenQueue.push(algn);
								running = false;
							}
						}
						else
						{
							running = false;
						}
					}
				}
				
				void validateBlockFragmentFinished(ValidationFragment & V, bool const ok)
				{
					readsValidated += (V.high-V.low);
				
					AlignmentBuffer::shared_ptr_type algn = V.buffer;
					bool returnbuffer = false;
					
					// check whether this is the last fragment for this buffer
					{
						libmaus2::parallel::ScopePosixSpinLock llock(validationActiveLock);
						validationOk[algn->id] = validationOk[algn->id] && ok;
						
						if ( -- validationFragmentsPerId[algn->id] == 0 )
						{
							bool const gok = validationOk.find(algn->id)->second;
							if ( ! gok )
							{
								libmaus2::exception::LibMausException lme;
								lme.getStream() << "Validation failed." << std::endl;
								lme.finish();
								throw lme;
							}
						
							validationFragmentsPerId.erase(validationFragmentsPerId.find(algn->id));
							validationActive.erase(validationActive.find(algn->id));
							validationOk.erase(validationOk.find(algn->id));
							returnbuffer = true;
						}
					}
					
					if ( returnbuffer )
					{
						blocksValidated += 1;

						if ( lastParseBlockSeen.get() && blocksValidated == blocksParsed )
							lastParseBlockValidated.set(true);				
					}
					
					if ( returnbuffer )
					{
						{
							libmaus2::parallel::ScopePosixSpinLock slock(validatedBlocksToBeRewrittenQueueLock);
							validatedBlocksToBeRewrittenQueue.push(algn);
						}
						
						checkValidatedRewritePending();
					}					
				}
				
				void fragmentAlignmentBufferRewriteUpdateInterval(int64_t /* maxleftoff */, int64_t /* maxrightoff */)
				{
				
				}
			};
		}
	}
}

#include <libmaus2/aio/PosixFdInputStream.hpp>

int main()
{
	try
	{
		uint64_t const numlogcpus = libmaus2::parallel::NumCpus::getNumLogicalProcessors();
		libmaus2::parallel::SimpleThreadPool STP(numlogcpus);
		libmaus2::aio::PosixFdInputStream in(STDIN_FILENO);
		libmaus2::bambam::parallel::RewriteControl VC(STP,in,Z_DEFAULT_COMPRESSION,std::cout);
		VC.checkEnqueReadPackage();
		VC.waitDecodingFinished();
		STP.terminate();
		STP.join();
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
