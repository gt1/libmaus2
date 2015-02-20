/*
    libmaus
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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

#include <libmaus/bambam/parallel/PairReadEndsMergeWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/FragReadEndsMergeWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/PairReadEndsContainerFlushWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/FragReadEndsContainerFlushWorkPackageDispatcher.hpp>

#include <libmaus/bambam/parallel/FragmentAlignmentBufferRewriteUpdateInterval.hpp>

#include <libmaus/bambam/parallel/FragmentAlignmentBufferReorderWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferReorderWorkPackage.hpp>

#include <libmaus/bambam/parallel/FragmentAlignmentBufferPosComparator.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferNameComparator.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferBaseSortWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferSortContext.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferMergeSortWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferSortFinishedInterface.hpp>

#include <libmaus/bambam/parallel/FragmentAlignmentBufferRewriteWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferRewriteFragmentCompleteInterface.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferRewriteWorkPackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferRewriteWorkPackage.hpp>
#include <libmaus/bambam/parallel/WriteBlockWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/BgzfOutputBlockWrittenInterface.hpp>
#include <libmaus/bambam/parallel/ReturnBgzfOutputBufferInterface.hpp>
#include <libmaus/bambam/parallel/WriteBlockWorkPackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/WritePendingObjectHeapComparator.hpp>
#include <libmaus/bambam/parallel/WritePendingObject.hpp>
#include <libmaus/bambam/parallel/BgzfLinearMemCompressWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/SmallLinearBlockCompressionPendingObjectHeapComparator.hpp>

#include <libmaus/bambam/parallel/ControlInputInfo.hpp>
#include <libmaus/bambam/parallel/InputBlockWorkPackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/InputBlockAddPendingInterface.hpp>
#include <libmaus/bambam/parallel/InputBlockWorkPackage.hpp>
#include <libmaus/parallel/NumCpus.hpp>
#include <libmaus/parallel/SimpleThreadPool.hpp>
#include <libmaus/parallel/SimpleThreadPoolWorkPackageFreeList.hpp>
#include <libmaus/bambam/parallel/InputBlockWorkPackageDispatcher.hpp>

#include <libmaus/bambam/parallel/RequeReadInterface.hpp>
#include <libmaus/bambam/parallel/DecompressBlocksWorkPackage.hpp>
#include <libmaus/bambam/parallel/DecompressBlocksWorkPackageDispatcher.hpp>

#include <libmaus/bambam/parallel/DecompressBlockWorkPackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/InputBlockReturnInterface.hpp>
#include <libmaus/bambam/parallel/DecompressedBlockAddPendingInterface.hpp>
#include <libmaus/bambam/parallel/BgzfInflateZStreamBaseReturnInterface.hpp>
#include <libmaus/bambam/parallel/DecompressBlockWorkPackageDispatcher.hpp>
#include <libmaus/parallel/LockedQueue.hpp>
#include <libmaus/parallel/LockedCounter.hpp>
#include <libmaus/bambam/parallel/BgzfInflateZStreamBaseGetInterface.hpp>
#include <libmaus/bambam/parallel/ParseInfo.hpp>
#include <libmaus/bambam/parallel/DecompressedPendingObject.hpp>
#include <libmaus/bambam/parallel/DecompressedPendingObjectHeapComparator.hpp>
#include <libmaus/parallel/LockedHeap.hpp>
#include <libmaus/bambam/parallel/ParseBlockWorkPackage.hpp>
#include <libmaus/bambam/parallel/AlignmentBufferAllocator.hpp>
#include <libmaus/bambam/parallel/AlignmentBufferTypeInfo.hpp>
#include <libmaus/bambam/parallel/ParseBlockWorkPackageDispatcher.hpp>

#include <libmaus/bambam/parallel/DecompressedBlockReturnInterface.hpp>
#include <libmaus/bambam/parallel/GetBgzfDeflateZStreamBaseInterface.hpp>
#include <libmaus/bambam/parallel/ParsedBlockAddPendingInterface.hpp>
#include <libmaus/bambam/parallel/ParsedBlockStallInterface.hpp>
#include <libmaus/bambam/parallel/ParsePackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/PutBgzfDeflateZStreamBaseInterface.hpp>
#include <libmaus/bambam/parallel/ValidateBlockFragmentWorkPackage.hpp>
#include <libmaus/bambam/parallel/ValidationFragment.hpp>
#include <libmaus/bambam/parallel/ValidateBlockFragmentPackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/ValidateBlockFragmentAddPendingInterface.hpp>
#include <libmaus/bambam/parallel/ValidateBlockFragmentWorkPackageDispatcher.hpp>

#include <libmaus/lz/BgzfDeflateZStreamBase.hpp>
#include <libmaus/lz/BgzfDeflateOutputBufferBaseTypeInfo.hpp>
#include <libmaus/lz/BgzfDeflateOutputBufferBaseAllocator.hpp>
#include <libmaus/lz/BgzfDeflateZStreamBaseTypeInfo.hpp>
#include <libmaus/lz/BgzfDeflateZStreamBaseAllocator.hpp>
#include <libmaus/parallel/LockedGrowingFreeList.hpp>
#include <libmaus/bambam/parallel/AlignmentBufferHeapComparator.hpp>
#include <libmaus/bambam/parallel/AddWritePendingBgzfBlockInterface.hpp>

#include <libmaus/bambam/parallel/WriteBlockWorkPackage.hpp>

#include <libmaus/lz/BgzfInflateZStreamBaseTypeInfo.hpp>
#include <libmaus/lz/BgzfInflateZStreamBaseAllocator.hpp>

#include <libmaus/bambam/parallel/DecompressedBlockAllocator.hpp>
#include <libmaus/bambam/parallel/DecompressedBlockTypeInfo.hpp>

#include <libmaus/bambam/parallel/FragmentAlignmentBuffer.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferAllocator.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferTypeInfo.hpp>

#include <libmaus/bambam/parallel/FragmentAlignmentBufferHeapComparator.hpp>
#include <libmaus/lz/BgzfDeflate.hpp>

#include <libmaus/bambam/parallel/FragmentAlignmentBufferRewriteReadEndsWorkPackageDispatcher.hpp>

#include <libmaus/bambam/parallel/ReadEndsContainerTypeInfo.hpp>
#include <libmaus/bambam/parallel/ReadEndsContainerAllocator.hpp>

#include <libmaus/bambam/ReadEndsBlockIndexSet.hpp>
#include <libmaus/bambam/DupMarkBase.hpp>
#include <libmaus/bambam/DupSetCallbackVector.hpp>
#include <libmaus/bambam/DuplicationMetrics.hpp>
#include <libmaus/bambam/DupSetCallbackSharedVector.hpp>

#include <libmaus/bambam/parallel/AddDuplicationMetricsInterface.hpp>


namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			template<typename _order_type>
			struct BlockSortControl :
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
				public FragmentAlignmentBufferRewriteReadEndsWorkPackageReturnInterface,
				public FragmentAlignmentBufferRewriteFragmentCompleteInterface,
				public FragmentAlignmentBufferSortFinishedInterface,
				public FragmentAlignmentBufferBaseSortWorkPackageReturnInterface<_order_type /* order_type */>,
				public FragmentAlignmentBufferMergeSortWorkPackageReturnInterface<_order_type /* order_type */>,
				public FragmentAlignmentBufferReorderWorkPackageReturnInterface,
				public FragmentAlignmentBufferReorderWorkPackageFinishedInterface,
				public FragmentAlignmentBufferRewriteUpdateInterval,
				public ReadEndsContainerFreeListInterface,
				public FragReadEndsContainerFlushFinishedInterface,
				public PairReadEndsContainerFlushFinishedInterface,
				public FragReadEndsContainerFlushWorkPackageReturnInterface,
				public PairReadEndsContainerFlushWorkPackageReturnInterface,
				public FragReadEndsMergeWorkPackageReturnInterface,
				public PairReadEndsMergeWorkPackageReturnInterface,
				public FragReadEndsMergeWorkPackageFinishedInterface,
				public PairReadEndsMergeWorkPackageFinishedInterface,
				public AddDuplicationMetricsInterface
			{
				typedef _order_type order_type;
				typedef BlockSortControl<order_type> this_type;
				
				static unsigned int getInputBlockCountShift()
				{
					return 8;
				}
				
				static unsigned int getInputBlockCount()
				{
					return 1ull << getInputBlockCountShift();
				}
				
				std::ostream & out;
				std::string const tempfileprefix;
							
				libmaus::parallel::LockedBool decodingFinished;
				
				libmaus::parallel::SimpleThreadPool & STP;
				libmaus::parallel::LockedFreeList<
					libmaus::lz::BgzfInflateZStreamBase,
					libmaus::lz::BgzfInflateZStreamBaseAllocator,
					libmaus::lz::BgzfInflateZStreamBaseTypeInfo
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
				FragmentAlignmentBufferRewriteReadEndsWorkPackageDispatcher FABRWPD;
				uint64_t const FABRWPDid;
				FragmentAlignmentBufferReorderWorkPackageDispatcher FABROWPD;
				uint64_t const FABROWPDid;
				libmaus::bambam::parallel::FragmentAlignmentBufferBaseSortWorkPackageDispatcher<order_type> FABBSWPD;
				uint64_t const FABBSWPDid;
				FragmentAlignmentBufferMergeSortWorkPackageDispatcher<order_type> FABMSWPD;
				uint64_t const FABMSWPDid;
				FragReadEndsContainerFlushWorkPackageDispatcher FRECFWPD;
				uint64_t const FRECFWPDid;
				PairReadEndsContainerFlushWorkPackageDispatcher PRECFWPD;
				uint64_t const PRECFWPDid;
				FragReadEndsMergeWorkPackageDispatcher FREMWPD;
				uint64_t const FREMWPDid;
				PairReadEndsMergeWorkPackageDispatcher PREMWPD;
				uint64_t const PREMWPDid;

				ControlInputInfo controlInputInfo;

				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<InputBlockWorkPackage> readWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<DecompressBlockWorkPackage> decompressBlockWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<DecompressBlocksWorkPackage> decompressBlocksWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<ParseBlockWorkPackage> parseBlockWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<ValidateBlockFragmentWorkPackage> validateBlockFragmentWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<BgzfLinearMemCompressWorkPackage> bgzfWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<WriteBlockWorkPackage> writeWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<FragmentAlignmentBufferRewriteReadEndsWorkPackage> fragmentAlignmentBufferRewriteWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<FragmentAlignmentBufferBaseSortPackage<order_type> > baseSortPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<FragmentAlignmentBufferMergeSortWorkPackage<order_type> > mergeSortPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<FragmentAlignmentBufferReorderWorkPackage> reorderPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<FragReadEndsContainerFlushWorkPackage> fragReadContainerFlushPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<PairReadEndsContainerFlushWorkPackage> pairReadContainerFlushPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<FragReadEndsMergeWorkPackage> fragReadEndsMergeWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<PairReadEndsMergeWorkPackage> pairReadEndsMergeWorkPackages;

				libmaus::parallel::LockedQueue<ControlInputInfo::input_block_type::shared_ptr_type> decompressPendingQueue;
				
				libmaus::parallel::PosixSpinLock decompressLock;
				libmaus::parallel::LockedFreeList<
					DecompressedBlock,
					DecompressedBlockAllocator,
					DecompressedBlockTypeInfo
				> decompressBlockFreeList;
				
				libmaus::parallel::LockedCounter bgzfin;
				libmaus::parallel::LockedCounter bgzfde;
				libmaus::parallel::LockedBool decompressFinished;
				
				libmaus::parallel::SynchronousCounter<uint64_t> inputBlockReturnCount;
				libmaus::parallel::PosixSpinLock requeReadPendingLock;
				libmaus::parallel::LockedBool requeReadPending;
				
				libmaus::bambam::parallel::ParseInfo parseInfo;
				libmaus::autoarray::AutoArray<char> bamHeader;

				// decompressed blocks ready to be parsed
				libmaus::parallel::LockedHeap<DecompressedPendingObject,DecompressedPendingObjectHeapComparator> parsePending;
				libmaus::parallel::PosixSpinLock parsePendingLock;

				// id of next block to be parsed
				uint64_t volatile nextDecompressedBlockToBeParsed;
				libmaus::parallel::PosixSpinLock nextDecompressedBlockToBeParsedLock;

				// stall slot for parsing BAM records
				AlignmentBuffer::shared_ptr_type parseStallSlot;
				libmaus::parallel::PosixSpinLock parseStallSlotLock;

				// free list for alignment buffers
				libmaus::parallel::LockedFreeList<AlignmentBuffer,AlignmentBufferAllocator,AlignmentBufferTypeInfo> parseBlockFreeList;

				libmaus::parallel::LockedBool lastParseBlockSeen;
				libmaus::parallel::LockedCounter readsParsed;
				libmaus::parallel::LockedCounter blocksParsed;
				uint64_t volatile readsParsedLastPrint;
				libmaus::parallel::PosixSpinLock readsParsedLastPrintLock;
				libmaus::parallel::SynchronousCounter<uint64_t> nextParseBufferId;

				std::map<uint64_t,AlignmentBuffer::shared_ptr_type> validationActive;
				std::map<uint64_t,uint64_t> validationFragmentsPerId;
				std::map<uint64_t,bool> validationOk;
				libmaus::parallel::PosixSpinLock validationActiveLock;
				libmaus::parallel::LockedCounter readsValidated;
				libmaus::parallel::LockedCounter blocksValidated;
				libmaus::parallel::LockedCounter buffersCompressed;
				libmaus::parallel::LockedCounter buffersWritten;
				libmaus::parallel::LockedBool lastParseBlockValidated;

				// finite size free list for output buffers
				libmaus::parallel::LockedFreeList<
					libmaus::lz::BgzfDeflateOutputBufferBase,
					libmaus::lz::BgzfDeflateOutputBufferBaseAllocator,
					libmaus::lz::BgzfDeflateOutputBufferBaseTypeInfo
					> bgzfDeflateOutputBufferFreeList;
					
				// infinite size free list for bgzf compressor objects
				libmaus::parallel::LockedGrowingFreeList<
					libmaus::lz::BgzfDeflateZStreamBase,
					libmaus::lz::BgzfDeflateZStreamBaseAllocator,
					libmaus::lz::BgzfDeflateZStreamBaseTypeInfo
				> bgzfDeflateZStreamBaseFreeList;

				libmaus::parallel::PosixSpinLock rewriteLargeBlockLock;
				std::priority_queue<FragmentAlignmentBuffer::shared_ptr_type, std::vector<FragmentAlignmentBuffer::shared_ptr_type>, 
					FragmentAlignmentBufferHeapComparator > rewriteLargeBlockQueue;
				uint64_t volatile rewriteLargeBlockNext;
				libmaus::parallel::LockedBool lastParseBlockCompressed;

				libmaus::parallel::PosixSpinLock compressionActiveBlocksLock;
				std::map<uint64_t, FragmentAlignmentBuffer::shared_ptr_type> compressionActive;
				std::map< int64_t, uint64_t> compressionUnfinished;
				std::priority_queue<SmallLinearBlockCompressionPendingObject,std::vector<SmallLinearBlockCompressionPendingObject>,
					SmallLinearBlockCompressionPendingObjectHeapComparator> compressionUnqueuedPending;

				libmaus::parallel::PosixSpinLock writePendingCountLock;
				std::map<int64_t,uint64_t> writePendingCount;
				libmaus::parallel::PosixSpinLock writeNextLock;				
				std::pair<int64_t,uint64_t> writeNext;
				libmaus::parallel::LockedBool lastParseBlockWritten;
				
				libmaus::parallel::LockedFreeList<
					FragmentAlignmentBuffer,
					FragmentAlignmentBufferAllocator,
					FragmentAlignmentBufferTypeInfo
				> fragmentBufferFreeListPreSort;

				libmaus::parallel::LockedFreeList<
					FragmentAlignmentBuffer,
					FragmentAlignmentBufferAllocator,
					FragmentAlignmentBufferTypeInfo
				> fragmentBufferFreeListPostSort;

				// post sort info
				libmaus::parallel::PosixSpinLock postSortRewriteLock;
				std::priority_queue<FragmentAlignmentBuffer::shared_ptr_type, std::vector<FragmentAlignmentBuffer::shared_ptr_type>, 
					FragmentAlignmentBufferHeapComparator > postSortPendingQueue;
				uint64_t volatile postSortNext;
				
				int64_t volatile maxleftoff;
				int64_t volatile maxrightoff;
				libmaus::parallel::PosixSpinLock maxofflock;
				
				libmaus::bambam::parallel::ReadEndsContainerAllocator const readEndsFragContainerAllocator;
				libmaus::bambam::parallel::ReadEndsContainerAllocator const readEndsPairContainerAllocator;
				libmaus::parallel::LockedGrowingFreeList<
					libmaus::bambam::ReadEndsContainer,
					libmaus::bambam::parallel::ReadEndsContainerAllocator,
					libmaus::bambam::parallel::ReadEndsContainerTypeInfo>
					readEndsFragContainerFreeList;
				libmaus::parallel::LockedGrowingFreeList<
					libmaus::bambam::ReadEndsContainer,
					libmaus::bambam::parallel::ReadEndsContainerAllocator,
					libmaus::bambam::parallel::ReadEndsContainerTypeInfo>
					readEndsPairContainerFreeList;

				libmaus::parallel::LockedCounter unflushedFragReadEndsContainers;
				libmaus::parallel::LockedCounter unflushedPairReadEndsContainers;
				
				libmaus::parallel::LockedCounter unmergeFragReadEndsRegions;
				libmaus::parallel::LockedCounter unmergePairReadEndsRegions;

				static uint64_t getParseBufferSize()
				{
					return (1ull<<28);
				}
				
				static uint64_t getReadEndsContainerSize()
				{
					return 16*1024*1024;
				}

				virtual void fragReadEndsContainerFlushFinished(libmaus::bambam::ReadEndsContainer::shared_ptr_type REC)
				{
					readEndsFragContainerFreeList.put(REC);
					unflushedFragReadEndsContainers -= 1;
				}
				
				virtual void pairReadEndsContainerFlushFinished(libmaus::bambam::ReadEndsContainer::shared_ptr_type REC)
				{

					readEndsPairContainerFreeList.put(REC);
					unflushedPairReadEndsContainers -= 1;
				}

				void enqueFlushFragReadEndsLists()
				{
					std::vector <libmaus::bambam::ReadEndsContainer::shared_ptr_type> V = readEndsFragContainerFreeList.getAll();
					unflushedFragReadEndsContainers += V.size();
					
					for ( uint64_t i = 0; i < V.size(); ++i )
					{
						FragReadEndsContainerFlushWorkPackage * pack = fragReadContainerFlushPackages.getPackage();
						*pack = FragReadEndsContainerFlushWorkPackage(V[i],0,FRECFWPDid);
						STP.enque(pack);
					}
				}

				void enqueFlushPairReadEndsLists()
				{
					std::vector <libmaus::bambam::ReadEndsContainer::shared_ptr_type> V = readEndsPairContainerFreeList.getAll();
					unflushedPairReadEndsContainers += V.size();
					
					for ( uint64_t i = 0; i < V.size(); ++i )
					{
						PairReadEndsContainerFlushWorkPackage * pack = pairReadContainerFlushPackages.getPackage();
						*pack = PairReadEndsContainerFlushWorkPackage(V[i],0,PRECFWPDid);
						STP.enque(pack);
					}
				}
				
				libmaus::util::shared_ptr<
					std::vector< ::libmaus::bambam::ReadEndsBlockDecoderBaseCollectionInfoBase > 
				>::type
					getFragMergeInfo()
				{	
					libmaus::util::shared_ptr<
						std::vector< ::libmaus::bambam::ReadEndsBlockDecoderBaseCollectionInfoBase > 
					>::type MI(new std::vector< ::libmaus::bambam::ReadEndsBlockDecoderBaseCollectionInfoBase >);

					std::vector <libmaus::bambam::ReadEndsContainer::shared_ptr_type> V = readEndsFragContainerFreeList.getAll();
					for ( uint64_t i = 0; i < V.size(); ++i )
						MI->push_back(V[i]->getMergeInfo());
					readEndsFragContainerFreeList.put(V);
					return MI;
				}

				libmaus::util::shared_ptr<
					std::vector< ::libmaus::bambam::ReadEndsBlockDecoderBaseCollectionInfoBase > 
				>::type
					getPairMergeInfo()
				{	
					libmaus::util::shared_ptr<
						std::vector< ::libmaus::bambam::ReadEndsBlockDecoderBaseCollectionInfoBase > 
					>::type MI(new std::vector< ::libmaus::bambam::ReadEndsBlockDecoderBaseCollectionInfoBase >);

					std::vector <libmaus::bambam::ReadEndsContainer::shared_ptr_type> V = readEndsPairContainerFreeList.getAll();
					for ( uint64_t i = 0; i < V.size(); ++i )
						MI->push_back(V[i]->getMergeInfo());
					readEndsPairContainerFreeList.put(V);
					return MI;
				}

				void fragReadEndsMergeWorkPackageFinished(FragReadEndsMergeWorkPackage *)
				{
					unmergeFragReadEndsRegions--;
				}
				
				void pairReadEndsMergeWorkPackageFinished(PairReadEndsMergeWorkPackage *)
				{
					unmergePairReadEndsRegions--;
				}
				
				void enqueMergeFragReadEndsLists()
				{	
					libmaus::util::shared_ptr< std::vector< ::libmaus::bambam::ReadEndsBlockDecoderBaseCollectionInfoBase > >::type MI =
						getFragMergeInfo();
					std::vector < std::vector< std::pair<uint64_t,uint64_t> > > SMI = 
						libmaus::bambam::ReadEndsBlockDecoderBaseCollection<true>::getShortMergeIntervals(*MI,STP.getNumThreads(),false /* check */);

					unmergeFragReadEndsRegions += SMI.size();
					
					for ( uint64_t i = 0; i < SMI.size(); ++i )
					{
						ReadEndsMergeRequest req(Pdupbitvec.get(),MI,SMI[i]);
						FragReadEndsMergeWorkPackage * package = fragReadEndsMergeWorkPackages.getPackage();
						*package = FragReadEndsMergeWorkPackage(req,0/*prio*/,FREMWPDid);
						STP.enque(package);
					}
				}

				void enqueMergePairReadEndsLists()
				{				
					libmaus::util::shared_ptr< std::vector< ::libmaus::bambam::ReadEndsBlockDecoderBaseCollectionInfoBase > >::type MI =
						getPairMergeInfo();
					std::vector < std::vector< std::pair<uint64_t,uint64_t> > > SMI = 
						libmaus::bambam::ReadEndsBlockDecoderBaseCollection<true>::getLongMergeIntervals(*MI,STP.getNumThreads(),false /* check */);
					
					unmergePairReadEndsRegions += SMI.size();
					
					for ( uint64_t i = 0; i < SMI.size(); ++i )
					{
						ReadEndsMergeRequest req(Pdupbitvec.get(),MI,SMI[i]);
						PairReadEndsMergeWorkPackage * package = pairReadEndsMergeWorkPackages.getPackage();
						*package = PairReadEndsMergeWorkPackage(req,0/*prio*/,PREMWPDid);
						STP.enque(package);
					}
				}
				
				std::map<uint64_t,libmaus::bambam::DuplicationMetrics> metrics;
				libmaus::parallel::PosixSpinLock metricslock;
				libmaus::bitio::BitVector::unique_ptr_type Pdupbitvec;
				
				libmaus::bitio::BitVector const & getDupBitVector()
				{
					return *Pdupbitvec;
				}
				
				void addDuplicationMetrics(std::map<uint64_t,libmaus::bambam::DuplicationMetrics> const & O)
				{
					libmaus::parallel::ScopePosixSpinLock smetricslock(metricslock);
					metrics = libmaus::bambam::DuplicationMetrics::add(metrics,O);
				}
				
				void flushReadEndsLists()
				{
					// set up duplicate data structure
					uint64_t const ureadsParsed = static_cast<uint64_t>(readsParsed);
					libmaus::bitio::BitVector::unique_ptr_type Tdupbitvec(new libmaus::bitio::BitVector(ureadsParsed));
					Pdupbitvec = UNIQUE_PTR_MOVE(Tdupbitvec);

					// enque ReadEndsContainer flush requests
					std::cerr << "[V] flushing read ends lists...";
					enqueFlushFragReadEndsLists();
					enqueFlushPairReadEndsLists();
					
					// wait for flush requests to finish
					while ( 
						(
							static_cast<uint64_t>(unflushedFragReadEndsContainers)
							||
							static_cast<uint64_t>(unflushedPairReadEndsContainers)
						)
						&& (!STP.isInPanicMode())
					)
					{
						sleep(1);
					}					
					std::cerr << "done." << std::endl;
					
					if ( STP.isInPanicMode() )
						return;

					// enque ReadEnds lists merge requests
					std::cerr << "[V] merging read ends lists/computing duplicates...";
					libmaus::timing::RealTimeClock mergertc; mergertc.start();
					enqueMergeFragReadEndsLists();
					enqueMergePairReadEndsLists();

					// wait for merge requests to finish
					while ( 
						(
							static_cast<uint64_t>(unmergeFragReadEndsRegions)
							||
							static_cast<uint64_t>(unmergePairReadEndsRegions)
						)
						&& (!STP.isInPanicMode())
					)
					{
						sleep(1);
					}
					std::cerr << "done, time " << mergertc.formatTime(mergertc.getElapsedSeconds()) << std::endl;

					if ( STP.isInPanicMode() )
						return;

					libmaus::bambam::DupSetCallbackSharedVector dvec(*Pdupbitvec);
					std::cerr << "[V] num dups " << dvec.getNumDups() << std::endl;

					// print computed metrics
					std::ostream & metricsstr = std::cerr;
					::libmaus::bambam::DuplicationMetrics::printFormatHeader("testparallelbamblocksort",metricsstr);
					for ( std::map<uint64_t,::libmaus::bambam::DuplicationMetrics>::const_iterator ita = metrics.begin(); ita != metrics.end();
						++ita )
						ita->second.format(metricsstr, parseInfo.Pheader->getLibraryName(ita->first));

					if ( metrics.size() == 1 )
					{
						metricsstr << std::endl;
						metricsstr << "## HISTOGRAM\nBIN\tVALUE" << std::endl;
						metrics.begin()->second.printHistogram(metricsstr);
					}
				}
				
				BlockSortControl(
					libmaus::parallel::SimpleThreadPool & rSTP,
					std::istream & in,
					int const level,
					std::ostream & rout,
					std::string const & rtempfileprefix
				)
				: 
					out(rout),
					tempfileprefix(rtempfileprefix),
					decodingFinished(false),
					STP(rSTP), 
					zstreambases(STP.getNumThreads()),
					IBWPD(*this,*this,*this),
					IBWPDid(STP.getNextDispatcherId()),
					DBWPD(*this,*this,*this,*this,*this),
					DBWPDid(STP.getNextDispatcherId()),
					PBWPD(*this,*this,*this,*this,*this),
					PBWPDid(STP.getNextDispatcherId()),
					VBFWPD(*this,*this),
					VBFWPDid(STP.getNextDispatcherId()),
					//
					BLMCWPD(*this,*this,*this,*this,*this),
					BLMCWPDid(STP.getNextDispatcherId()),
					WBWPD(*this,*this,*this),
					WBWPDid(STP.getNextDispatcherId()),
					FABRWPD(*this,*this,*this,*this,*this),
					FABRWPDid(STP.getNextDispatcherId()),
					FABROWPD(*this,*this),
					FABROWPDid(STP.getNextDispatcherId()),
					FABBSWPD(*this),
					FABBSWPDid(STP.getNextDispatcherId()),
					FABMSWPD(*this),
					FABMSWPDid(STP.getNextDispatcherId()),
					FRECFWPD(*this,*this),
					FRECFWPDid(STP.getNextDispatcherId()),
					PRECFWPD(*this,*this),
					PRECFWPDid(STP.getNextDispatcherId()),
					FREMWPD(*this,*this,*this),
					FREMWPDid(STP.getNextDispatcherId()),
					PREMWPD(*this,*this,*this),
					PREMWPDid(STP.getNextDispatcherId()),
					controlInputInfo(in,0,getInputBlockCount()),
					decompressBlockFreeList(getInputBlockCount() * STP.getNumThreads() * 2),
					bgzfin(0),
					bgzfde(0),
					decompressFinished(false),
					requeReadPending(false),
					parseInfo(this),
					nextDecompressedBlockToBeParsed(0),
					parseStallSlot(),
					parseBlockFreeList(
						std::min(STP.getNumThreads(),static_cast<uint64_t>(8)),
						AlignmentBufferAllocator(
							getParseBufferSize(),
							1 /* pointer multiplier */
						)
					),
					lastParseBlockSeen(false),
					readsParsedLastPrint(0),
					lastParseBlockValidated(false),
					bgzfDeflateOutputBufferFreeList(2*STP.getNumThreads(),libmaus::lz::BgzfDeflateOutputBufferBaseAllocator(level)),
					bgzfDeflateZStreamBaseFreeList(libmaus::lz::BgzfDeflateZStreamBaseAllocator(level)),
					rewriteLargeBlockNext(0),
					lastParseBlockCompressed(false),
					writeNext(-1,0),
					lastParseBlockWritten(false),
					fragmentBufferFreeListPreSort(STP.getNumThreads(),FragmentAlignmentBufferAllocator(STP.getNumThreads(), 2 /* pointer multiplier */)),
					fragmentBufferFreeListPostSort(STP.getNumThreads(),FragmentAlignmentBufferAllocator(STP.getNumThreads(), 1 /* pointer multiplier */)),
					postSortNext(0),
					maxleftoff(0),
					maxrightoff(0),
					readEndsFragContainerAllocator(getReadEndsContainerSize(),tempfileprefix+"_readends_frag_"),
					readEndsPairContainerAllocator(getReadEndsContainerSize(),tempfileprefix+"_readends_pair_"),
					readEndsFragContainerFreeList(readEndsFragContainerAllocator),
					readEndsPairContainerFreeList(readEndsPairContainerAllocator),
					unflushedFragReadEndsContainers(0),
					unflushedPairReadEndsContainers(0),
					unmergeFragReadEndsRegions(0),
					unmergePairReadEndsRegions(0)
				{
					STP.registerDispatcher(IBWPDid,&IBWPD);
					STP.registerDispatcher(DBWPDid,&DBWPD);
					STP.registerDispatcher(PBWPDid,&PBWPD);
					STP.registerDispatcher(VBFWPDid,&VBFWPD);
					STP.registerDispatcher(BLMCWPDid,&BLMCWPD);
					STP.registerDispatcher(WBWPDid,&WBWPD);
					STP.registerDispatcher(FABRWPDid,&FABRWPD);
					STP.registerDispatcher(FABROWPDid,&FABROWPD);
					STP.registerDispatcher(FABBSWPDid,&FABBSWPD);
					STP.registerDispatcher(FABMSWPDid,&FABMSWPD);
					STP.registerDispatcher(FRECFWPDid,&FRECFWPD);
					STP.registerDispatcher(PRECFWPDid,&PRECFWPD);
					STP.registerDispatcher(FREMWPDid,&FREMWPD);
					STP.registerDispatcher(PREMWPDid,&PREMWPD);
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
					libmaus::parallel::ScopePosixSpinLock llock(requeReadPendingLock);
					requeReadPending.set(true);
				}
				
				void waitDecompressFinished()
				{
					struct timespec ts, tsrem;
					
					while ( ! decompressFinished.get() )
					{
						ts.tv_sec = 0;
						ts.tv_nsec = 10000000; // 10 milli
						nanosleep(&ts,&tsrem);
						
						// sleep(1);
					}
				}

				void waitDecodingFinished()
				{
					while ( ( ! decodingFinished.get() ) && (!STP.isInPanicMode()) )
					{
						sleep(1);
						// STP.printStateHistogram(std::cerr);
					}
				}
				
				void bamHeaderComplete(libmaus::bambam::BamHeaderParserState const & BHPS)
				{
					bamHeader = BHPS.getSerialised();
					uint64_t const maxblocksize = libmaus::lz::BgzfConstants::getBgzfMaxBlockSize();
					uint64_t const headersize = bamHeader.size();
					uint64_t const tnumblocks = (headersize + maxblocksize - 1)/maxblocksize;
					uint64_t const blocksize = (headersize+tnumblocks-1)/tnumblocks;
					uint64_t const numblocks = (headersize+blocksize-1)/blocksize;
					
					{
						libmaus::parallel::ScopePosixSpinLock lcompressionActiveBlocksLock(compressionActiveBlocksLock);
						compressionUnfinished[-1] = numblocks;
					}
							
					{
						libmaus::parallel::ScopePosixSpinLock lwritePendingCountLock(writePendingCountLock);
						writePendingCount[-1] = numblocks;
					}
						
					// enque compression requests
					for ( uint64_t i = 0; i < numblocks; ++i )
					{
						uint64_t const ilow = i*blocksize;
						uint64_t const ihigh = std::min(ilow+blocksize,headersize);
						uint8_t * plow  = reinterpret_cast<uint8_t *>(bamHeader.begin()) + ilow;
						uint8_t * phigh = reinterpret_cast<uint8_t *>(bamHeader.begin()) + ihigh;
						
						libmaus::parallel::ScopePosixSpinLock lcompressionActiveBlocksLock(compressionActiveBlocksLock);
						compressionUnqueuedPending.push(SmallLinearBlockCompressionPendingObject(-1,i,plow,phigh));
					}
				}

				void putBgzfInflateZStreamBaseReturn(libmaus::lz::BgzfInflateZStreamBase::shared_ptr_type decoder)
				{
					zstreambases.put(decoder);
				}
				
				libmaus::lz::BgzfInflateZStreamBase::shared_ptr_type getBgzfInflateZStreamBase()
				{
					return zstreambases.get();
				}

				// get bgzf compressor object
				virtual libmaus::lz::BgzfDeflateZStreamBase::shared_ptr_type getBgzfDeflateZStreamBase()
				{
					libmaus::lz::BgzfDeflateZStreamBase::shared_ptr_type tptr(bgzfDeflateZStreamBaseFreeList.get());
					return tptr;
				}

				// return bgzf compressor object
				virtual void putBgzfDeflateZStreamBase(libmaus::lz::BgzfDeflateZStreamBase::shared_ptr_type & ptr)
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
				void returnFragmentAlignmentBufferRewriteReadEndsWorkPackage(FragmentAlignmentBufferRewriteReadEndsWorkPackage * package) { fragmentAlignmentBufferRewriteWorkPackages.returnPackage(package); }
				void putBaseSortWorkPackage(FragmentAlignmentBufferBaseSortPackage<order_type> * package) { baseSortPackages.returnPackage(package); }
				void putMergeSortWorkPackage(FragmentAlignmentBufferMergeSortWorkPackage<order_type> * package) { mergeSortPackages.returnPackage(package); }
				void returnFragmentAlignmentBufferReorderWorkPackage(FragmentAlignmentBufferReorderWorkPackage * package) { reorderPackages.returnPackage(package); }
				void fragReadEndsContainerFlushWorkPackageReturn(FragReadEndsContainerFlushWorkPackage * package) { fragReadContainerFlushPackages.returnPackage(package); }
				void pairReadEndsContainerFlushWorkPackageReturn(PairReadEndsContainerFlushWorkPackage * package) { pairReadContainerFlushPackages.returnPackage(package); }
				void fragReadEndsMergeWorkPackageReturn(FragReadEndsMergeWorkPackage * package) { fragReadEndsMergeWorkPackages.returnPackage(package); }
				void pairReadEndsMergeWorkPackageReturn(PairReadEndsMergeWorkPackage * package) { pairReadEndsMergeWorkPackages.returnPackage(package); }

				// return input block after decompression
				void putInputBlockReturn(ControlInputInfo::input_block_type::shared_ptr_type block) 
				{ 					
					uint64_t const freecnt = controlInputInfo.inputBlockFreeList.putAndCount(block);
					uint64_t const capacity = controlInputInfo.inputBlockFreeList.capacity();
					
					bool req = false;
					
					{
						libmaus::parallel::ScopePosixSpinLock llock(requeReadPendingLock);
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
						libmaus::parallel::ScopePosixSpinLock plock(decompressPendingQueue.getLock());

						uint64_t const psize = decompressPendingQueue.sizeUnlocked();
							
						if ( ! psize )
							return;
								
						bool const forcerun = decompressPendingQueue.backUnlocked()->final;
						if ( (! forcerun) && (psize < getDecompressBatchSize()) )
							return;
							
						libmaus::parallel::ScopePosixSpinLock flock(decompressBlockFreeList.getLock());
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
						libmaus::parallel::ScopePosixSpinLock llock(decompressPendingQueue.getLock());
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
					libmaus::parallel::ScopePosixSpinLock slock(nextDecompressedBlockToBeParsedLock);
					libmaus::parallel::ScopePosixSpinLock alock(parseStallSlotLock);
					libmaus::parallel::ScopePosixSpinLock plock(parsePendingLock);

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
						STP.addLogStringWithThreadId("erasing stall slot for block id" + libmaus::util::NumberSerialisation::formatNumber(obj.second->blockid,0));
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
						STP.addLogStringWithThreadId("using free block for block id" + libmaus::util::NumberSerialisation::formatNumber(obj.second->blockid,0));
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

				void putDecompressedBlockReturn(libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type block)
				{
					decompressBlockFreeList.put(block);
					checkEnqueDecompress();				

					{
					libmaus::parallel::ScopePosixSpinLock slock(nextDecompressedBlockToBeParsedLock);
					nextDecompressedBlockToBeParsed += 1;
					}
					
					checkParsePendingList();
				}

				void putParsedBlockStall(AlignmentBuffer::shared_ptr_type algn)
				{
					{
						libmaus::parallel::ScopePosixSpinLock slock(parseStallSlotLock);
						
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
						libmaus::parallel::ScopePosixSpinLock sreadsParsedLastPrintLock(readsParsedLastPrintLock);
						
						if ( ((readsParsed >> 20) != (readsParsedLastPrint >> 20)) || algn->final )
						{
							libmaus::parallel::ScopePosixSpinLock SPSL(STP.getGlobalLock());
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
						libmaus::parallel::ScopePosixSpinLock llock(validationActiveLock);
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
						libmaus::parallel::ScopePosixSpinLock slock(parsePendingLock);
						parsePending.push(DecompressedPendingObject(block->blockid,block));
					}
					
					checkParsePendingList();					
				}
					
				void smallLinearBlockCompressionPendingObjectFinished(SmallLinearBlockCompressionPendingObject const & obj)
				{
					bool blockfinished = false;
					{
						libmaus::parallel::ScopePosixSpinLock lcompressionActiveBlocksLock(compressionActiveBlocksLock);

						if ( --compressionUnfinished[obj.blockid] == 0 )
							blockfinished = true;
					}
	
					if ( obj.blockid >= 0 && blockfinished )
					{
						FragmentAlignmentBuffer::shared_ptr_type algn = FragmentAlignmentBuffer::shared_ptr_type();
						{
						libmaus::parallel::ScopePosixSpinLock lcompressionActiveBlocksLock(compressionActiveBlocksLock);
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

						#if 0
						// return FragmentAlignmentBuffer
						fragmentBufferFreeListPreSort.put(algn);
						checkValidatedRewritePending();
						#endif

						// return buffer
						fragmentBufferFreeListPostSort.put(algn);
						checkPostSortPendingQueue();
					}
				
				}
				
				std::priority_queue<WritePendingObject,std::vector<WritePendingObject>,WritePendingObjectHeapComparator> writePendingQueue;
				libmaus::parallel::PosixSpinLock writePendingQueueLock;

				virtual void returnBgzfOutputBufferInterface(libmaus::lz::BgzfDeflateOutputBufferBase::shared_ptr_type & obuf)
				{
					bgzfDeflateOutputBufferFreeList.put(obuf);
					checkSmallBlockCompressionPending();				
				}
				
				std::vector<uint64_t> streamBytesWritten;
				libmaus::parallel::PosixSpinLock streamBytesWrittenLock;
				std::vector<uint64_t> blockStarts;
				libmaus::parallel::PosixSpinLock blockStartsLock;
				
				virtual void bgzfOutputBlockWritten(uint64_t const streamid, int64_t const blockid, uint64_t const subid, uint64_t const n)
				{
					{
						// make sure stream is known
						{
							libmaus::parallel::ScopePosixSpinLock slock(streamBytesWrittenLock);
							while ( ! (streamid < streamBytesWritten.size()) )
								streamBytesWritten.push_back(0);
						}
						
						if ( subid == 0 && blockid >= 0 )
						{
							libmaus::parallel::ScopePosixSpinLock slock(streamBytesWrittenLock);
							uint64_t const offset = streamBytesWritten[streamid];
							
							libmaus::parallel::ScopePosixSpinLock sblock(blockStartsLock);
							while ( ! (static_cast<uint64_t>(blockid) < blockStarts.size()) )
								blockStarts.push_back(0);
							blockStarts[blockid] = offset;							
						}
						
						// update number of bytes written
						{
							libmaus::parallel::ScopePosixSpinLock slock(streamBytesWrittenLock);
							streamBytesWritten[streamid] += n;						
						}
					
						libmaus::parallel::ScopePosixSpinLock lwritePendingQueueLock(writePendingQueueLock);
						libmaus::parallel::ScopePosixSpinLock lwriteNextLock(writeNextLock);
						libmaus::parallel::ScopePosixSpinLock lwritePendingCountLock(writePendingCountLock);
						
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
									libmaus::lz::BgzfDeflate<std::ostream> defl(out);
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
					libmaus::parallel::ScopePosixSpinLock lwritePendingQueueLock(writePendingQueueLock);
					libmaus::parallel::ScopePosixSpinLock lwriteNextLock(writeNextLock);
					if ( 
						writePendingQueue.size() &&
						writePendingQueue.top().blockid == writeNext.first &&
						static_cast<int64_t>(writePendingQueue.top().subid) == static_cast<int64_t>(writeNext.second)
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
					libmaus::lz::BgzfDeflateOutputBufferBase::shared_ptr_type obuf,
					libmaus::lz::BgzfDeflateZStreamBaseFlushInfo const & flushinfo
				)
				{
					{
						libmaus::parallel::ScopePosixSpinLock lwritePendingQueueLock(writePendingQueueLock);
						writePendingQueue.push(WritePendingObject(0 /* stream id */,&out,blockid,subid,obuf,flushinfo));
					}
					
					checkWritePendingQueue();
				}

				void checkSmallBlockCompressionPending()
				{
					bool running = true;
					while ( running )
					{
						libmaus::lz::BgzfDeflateOutputBufferBase::shared_ptr_type obuf = 
							bgzfDeflateOutputBufferFreeList.getIf();
						
						if ( obuf )
						{
							SmallLinearBlockCompressionPendingObject obj;
							bool haveObject = false;
							
							{
								libmaus::parallel::ScopePosixSpinLock lcompressionActiveBlocksLock(compressionActiveBlocksLock);
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
							libmaus::parallel::ScopePosixSpinLock llock(rewriteLargeBlockLock);
							if ( rewriteLargeBlockQueue.size() && rewriteLargeBlockQueue.top()->id == rewriteLargeBlockNext )
							{
								algn = rewriteLargeBlockQueue.top();
								rewriteLargeBlockQueue.pop();
							}
						}

						if ( algn )
						{	
							{
								libmaus::parallel::ScopePosixSpinLock lcompressionActiveBlocksLock(compressionActiveBlocksLock);
								compressionActive[algn->id] = algn;
							}
						
							// compute fragments for compression
							std::vector<std::pair<uint8_t *,uint8_t *> > V;
							algn->getLinearOutputFragments(libmaus::lz::BgzfConstants::getBgzfMaxBlockSize(),V);
							
							{
								libmaus::parallel::ScopePosixSpinLock lcompressionActiveBlocksLock(compressionActiveBlocksLock);
								compressionUnfinished[algn->id] = V.size();
							}
							
							{
								libmaus::parallel::ScopePosixSpinLock lwritePendingCountLock(writePendingCountLock);
								writePendingCount[algn->id] = V.size();
							}
						
							// enque compression requests
							for ( uint64_t i = 0; i < V.size(); ++i )
							{
								libmaus::parallel::ScopePosixSpinLock lcompressionActiveBlocksLock(compressionActiveBlocksLock);
								compressionUnqueuedPending.push(
									SmallLinearBlockCompressionPendingObject(algn->id,i,V[i].first,V[i].second)
								);
							}
							
							libmaus::parallel::ScopePosixSpinLock llock(rewriteLargeBlockLock);
							++rewriteLargeBlockNext;
						}					
						else
						{
							running = false;
						}
					}
					
					checkSmallBlockCompressionPending();
				}
				
				libmaus::parallel::LockedCounter nextValidatedBlockToBeRewritten;
				libmaus::parallel::PosixSpinLock validatedBlocksToBeRewrittenQueueLock;
				std::priority_queue<
					AlignmentBuffer::shared_ptr_type,
					std::vector<AlignmentBuffer::shared_ptr_type>,
					AlignmentBufferHeapComparator
				> validatedBlocksToBeRewrittenQueue;
				
				// pre sort info
				libmaus::parallel::PosixSpinLock rewriteActiveLock;
				std::map<uint64_t,AlignmentBuffer::shared_ptr_type> rewriteActiveAlignmentBuffers;
				std::map<uint64_t,FragmentAlignmentBuffer::shared_ptr_type> rewriteActiveFragmentAlignmentBuffers;
				std::map<uint64_t,uint64_t> rewriteActiveCnt;
				
				// alignments per output block
				std::vector<uint64_t> blockAlgnCnt;
				// lock for blockAlgnCnt
				libmaus::parallel::PosixSpinLock blockAlgnCntLock;
				
				void fragmentAlignmentBufferReorderWorkPackageFinished(FragmentAlignmentBufferReorderWorkPackage * package)
				{
					uint64_t const id = package->copyReq.T->id;

					bool lastfrag = false;
					
					{
						libmaus::parallel::ScopePosixSpinLock llock(postSortRewriteLock);
						if ( ! --rewriteUnfinished[id] )
						{
							lastfrag = true;
						}
					}
					
					if ( lastfrag )
					{
						FragmentAlignmentBuffer::shared_ptr_type FAB;
						FragmentAlignmentBuffer::shared_ptr_type outFAB;

						{
							libmaus::parallel::ScopePosixSpinLock llock(postSortRewriteLock);

							std::map<uint64_t,FragmentAlignmentBuffer::shared_ptr_type>::iterator inIt = rewriteActiveIn.find(id);
							assert ( inIt != rewriteActiveIn.end() );
							FAB = inIt->second;
							rewriteActiveIn.erase(inIt);
							
							std::map<uint64_t,FragmentAlignmentBuffer::shared_ptr_type>::iterator outIt = rewriteActiveOut.find(id);
							assert ( outIt != rewriteActiveOut.end() );
							outFAB = outIt->second;
							rewriteActiveOut.erase(outIt);
							
							std::map<uint64_t,uint64_t>::iterator itUn = rewriteUnfinished.find(id);
							assert ( itUn != rewriteUnfinished.end() );
							assert ( itUn->second == 0 );
							rewriteUnfinished.erase(itUn);
						}
						
						{
							libmaus::parallel::ScopePosixSpinLock slock(blockAlgnCntLock);
							while ( ! (outFAB->id < blockAlgnCnt.size()) )
								blockAlgnCnt.push_back(0);
							assert ( outFAB->id < blockAlgnCnt.size() );
							blockAlgnCnt[outFAB->id] = outFAB->getFill();
						}

						#if 0
						FAB->compareBuffers(*outFAB);
						#endif

						// return FragmentAlignmentBuffer after copying is finished
						fragmentBufferFreeListPreSort.put(FAB);
						checkValidatedRewritePending();

						// put block in ready for compression queue
						{
							libmaus::parallel::ScopePosixSpinLock llock(rewriteLargeBlockLock);
							rewriteLargeBlockQueue.push(outFAB);
						}
								
						// check ready for compression queue
						checkLargeBlockCompressionPending();
					}
				}
				
				std::map<uint64_t,uint64_t> rewriteUnfinished;
				std::map<uint64_t,FragmentAlignmentBuffer::shared_ptr_type> rewriteActiveIn;
				std::map<uint64_t,FragmentAlignmentBuffer::shared_ptr_type> rewriteActiveOut;

				void checkPostSortPendingQueue()
				{
					bool running = true;
					
					while ( running )
					{
						FragmentAlignmentBuffer::shared_ptr_type FAB;
						
						{
							libmaus::parallel::ScopePosixSpinLock llock(postSortRewriteLock);
							if ( postSortPendingQueue.size() && postSortPendingQueue.top()->id == postSortNext )
							{
								FAB = postSortPendingQueue.top();
								postSortPendingQueue.pop();
							}
						}
						
						if ( FAB )
						{
							FragmentAlignmentBuffer::shared_ptr_type outFAB = fragmentBufferFreeListPostSort.getIf();
							
							if ( outFAB )
							{
								{
									libmaus::parallel::ScopePosixSpinLock llock(postSortRewriteLock);
									postSortNext += 1;
								}
						
								// get data copy requests		
								std::vector<libmaus::bambam::parallel::FragmentAlignmentBuffer::FragmentAlignmentBufferCopyRequest> 
									reqs = FAB->setupCopy(*outFAB);
									
								{
									libmaus::parallel::ScopePosixSpinLock llock(postSortRewriteLock);
									rewriteUnfinished[FAB->id] = reqs.size();
									rewriteActiveIn[FAB->id] = FAB;
									rewriteActiveOut[FAB->id] = outFAB;
								}
								
								for ( uint64_t i = 0; i < reqs.size(); ++i )
								{
									libmaus::bambam::parallel::FragmentAlignmentBufferReorderWorkPackage * pack = reorderPackages.getPackage();
									*pack = libmaus::bambam::parallel::FragmentAlignmentBufferReorderWorkPackage(reqs[i],0 /* prio */,FABROWPDid);
									STP.enque(pack);
								}
								
							}
							// no output buffer available, put buffer back in pending queue
							else
							{
								// put back input buffer
								{
									libmaus::parallel::ScopePosixSpinLock llock(postSortRewriteLock);
									postSortPendingQueue.push(FAB);
								}
								running = false;
							}
						}
						else
						{
							running = false;
						}
					}
				}

				void putSortFinished(FragmentAlignmentBuffer::shared_ptr_type FAB)
				{
					{
						libmaus::parallel::ScopePosixSpinLock lsortContextsActiveLock(sortContextsActiveLock);
						sortContextsActive.erase(sortContextsActive.find(FAB->id));
					}
					
					// std::cerr << "block sorted." << std::endl;
					
					#if 0
					bool const sortok = FAB->checkSort(order_type());
					// std::cerr << "sort finished for " << FAB->id << " " << sortok << std::endl;
					
					if ( ! sortok )
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "Failed to sort block " << FAB->id << std::endl;
						lme.finish();
						throw lme;
					}
					#endif
					
					{
						libmaus::parallel::ScopePosixSpinLock llock(postSortRewriteLock);
						postSortPendingQueue.push(FAB);
					}

					checkPostSortPendingQueue();
				}

				std::map<uint64_t, typename FragmentAlignmentBufferSortContext<order_type>::shared_ptr_type > sortContextsActive;
				libmaus::parallel::PosixSpinLock sortContextsActiveLock;

				void fragmentAlignmentBufferRewriteFragmentComplete(
					AlignmentBuffer::shared_ptr_type & algn,
					FragmentAlignmentBuffer::shared_ptr_type & FAB,
					uint64_t const /* j */
				)
				{
					bool blockFinished = false;
					
					{					
						libmaus::parallel::ScopePosixSpinLock slock(rewriteActiveLock);
						
						if ( -- rewriteActiveCnt[algn->id] == 0 )
						{
							blockFinished = true;
							rewriteActiveAlignmentBuffers.erase(rewriteActiveAlignmentBuffers.find(algn->id));
							rewriteActiveFragmentAlignmentBuffers.erase(rewriteActiveFragmentAlignmentBuffers.find(algn->id));
							rewriteActiveCnt.erase(rewriteActiveCnt.find(algn->id));
						}
					}
					
					// rewrite for block is complete
					if ( blockFinished )
					{
						typename FragmentAlignmentBufferSortContext<order_type>::shared_ptr_type
							sortcontext(new FragmentAlignmentBufferSortContext<order_type>(
								FAB,STP.getNumThreads(),STP,mergeSortPackages,
								FABMSWPDid /* merge dispatcher id */,*this /* finished */
								));
						{
						libmaus::parallel::ScopePosixSpinLock lsortContextsActiveLock(sortContextsActiveLock);
						sortContextsActive[FAB->id] = sortcontext;
						}
						sortcontext->enqueBaseSortPackages(baseSortPackages,FABBSWPDid /* base sort dispatcher */ );

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
							libmaus::parallel::ScopePosixSpinLock slock(validatedBlocksToBeRewrittenQueueLock);
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
							FragmentAlignmentBuffer::shared_ptr_type FAB = fragmentBufferFreeListPreSort.getIf();
							
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
									libmaus::parallel::ScopePosixSpinLock slock(rewriteActiveLock);
									rewriteActiveAlignmentBuffers[algn->id] = algn;
									rewriteActiveFragmentAlignmentBuffers[FAB->id] = FAB;
									rewriteActiveCnt[FAB->id] = FAB->size();
								}
								
								for ( uint64_t j = 0; j < FAB->size(); ++j )
								{
									FragmentAlignmentBufferRewriteReadEndsWorkPackage * pack = fragmentAlignmentBufferRewriteWorkPackages.getPackage();
									
									*pack = FragmentAlignmentBufferRewriteReadEndsWorkPackage(
										0 /* prio */,
										algn,
										FAB,
										j,
										parseInfo.Pheader.get(),
										FABRWPDid
									);
									
									STP.enque(pack);
								}
							}
							else
							{
								libmaus::parallel::ScopePosixSpinLock slock(validatedBlocksToBeRewrittenQueueLock);
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
						libmaus::parallel::ScopePosixSpinLock llock(validationActiveLock);
						validationOk[algn->id] = validationOk[algn->id] && ok;
						
						if ( -- validationFragmentsPerId[algn->id] == 0 )
						{
							bool const gok = validationOk.find(algn->id)->second;
							if ( ! gok )
							{
								libmaus::exception::LibMausException lme;
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
							libmaus::parallel::ScopePosixSpinLock slock(validatedBlocksToBeRewrittenQueueLock);
							validatedBlocksToBeRewrittenQueue.push(algn);
						}
						
						checkValidatedRewritePending();
					}					
				}
				
				void fragmentAlignmentBufferRewriteUpdateInterval(int64_t rmaxleftoff, int64_t rmaxrightoff)
				{
					libmaus::parallel::ScopePosixSpinLock lmaxofflock(maxofflock);
					maxleftoff  = std::max(static_cast<int64_t>(maxleftoff),rmaxleftoff);
					maxrightoff = std::max(static_cast<int64_t>(maxrightoff),rmaxrightoff);
				}

				libmaus::bambam::ReadEndsContainer::shared_ptr_type getFragContainer()
				{
					return readEndsFragContainerFreeList.get();
				}
				
				void returnFragContainer(libmaus::bambam::ReadEndsContainer::shared_ptr_type ptr)
				{
					readEndsFragContainerFreeList.put(ptr);
				}

				libmaus::bambam::ReadEndsContainer::shared_ptr_type getPairContainer()
				{
					return readEndsPairContainerFreeList.get();
				}
				
				void returnPairContainer(libmaus::bambam::ReadEndsContainer::shared_ptr_type ptr)
				{
					readEndsPairContainerFreeList.put(ptr);
				}
			};
		}
	}
}

#include <libmaus/aio/PosixFdInputStream.hpp>
#include <libmaus/random/Random.hpp>
#include <libmaus/util/ArgInfo.hpp>
#include <libmaus/bambam/BamWriter.hpp>

template<typename order_type>
void mapperm(int argc, char * argv[])
{
	try
	{
		libmaus::util::ArgInfo const arginfo(argc,argv);
		uint64_t const textlen = arginfo.getValue<int>("textlen",120);
		uint64_t const readlen = arginfo.getValue<int>("readlen",110);
		uint64_t const numreads = (textlen-readlen)+1;
		std::string const tempfileprefix = arginfo.getDefaultTmpFileName();
		libmaus::autoarray::AutoArray<char> T(textlen,false);
		libmaus::random::Random::setup(19);
		for ( uint64_t i = 0; i < textlen; ++i )
		{
			switch ( libmaus::random::Random::rand8() % 4 )
			{
				case 0: T[i] = 'A'; break;
				case 1: T[i] = 'C'; break;
				case 2: T[i] = 'G'; break;
				case 3: T[i] = 'T'; break;
			}
		}

		::libmaus::bambam::BamHeader header;
		header.addChromosome("text",textlen);
		
		std::vector<uint64_t> P;
		for ( uint64_t i = 0; i < numreads; ++i )
			P.push_back(i);

		uint64_t const check = std::min(static_cast<uint64_t>(arginfo.getValue<int>("check",8)),P.size());		
		std::vector<uint64_t> prev(check,numreads);

		uint64_t const numlogcpus = arginfo.getValue<int>("threads",libmaus::parallel::NumCpus::getNumLogicalProcessors());
		libmaus::parallel::SimpleThreadPool STP(numlogcpus);
		
		do
		{		
			std::ostringstream out;
			::libmaus::bambam::BamWriter::unique_ptr_type bamwriter(new ::libmaus::bambam::BamWriter(out,header,0,0));
			
			bool print = false;
			for ( uint64_t i = 0; i < check; ++i )
				if ( P[i] != prev[i] )
					print = true;

			if ( print )
			{
				for ( uint64_t i = 0; i < check; ++i )
				{
					std::cerr << P[i] << ";";
					prev[i] = P[i];
				}
				std::cerr << std::endl;
			}
					
			for ( uint64_t j = 0; j < P.size(); ++j )
			{
				uint64_t const i = P[j];
				
				std::ostringstream rnstr;
				rnstr << "r" << "_" << std::setw(6) << std::setfill('0') << i;
				std::string const rn = rnstr.str();
				
				std::string const read(T.begin()+i,T.begin()+i+readlen);
				// std::cerr << read << std::endl;

				bamwriter->encodeAlignment(rn,0 /* refid */,i,30, 0, 
					libmaus::util::NumberSerialisation::formatNumber(readlen,0) + "M", 
					-1,-1, -1, read, std::string(readlen,'H'));
				bamwriter->commit();
			}
			
			bamwriter.reset();

			std::istringstream in(out.str());
			std::ostringstream ignout;
			libmaus::bambam::parallel::BlockSortControl<order_type> VC(STP,in,0 /* comp */,ignout,tempfileprefix);
			VC.checkEnqueReadPackage();
			VC.waitDecodingFinished();

		} while ( std::next_permutation(P.begin(),P.end()) );
		
		// std::cout << out.str();

		STP.terminate();
		STP.join();
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
	}
}

#include <libmaus/aio/PosixFdOutputStream.hpp>
#include <libmaus/bambam/BamAlignmentHeapComparator.hpp>
#include <libmaus/bambam/BamAlignmentNameComparator.hpp>
#include <libmaus/bambam/BamAlignmentPosComparator.hpp>
#include <libmaus/bambam/BamBlockWriterBaseFactory.hpp>
#include <config.h>

#include <libmaus/bambam/parallel/GenericInputBlock.hpp>

struct GenericInputBase
{
	typedef GenericInputBase this_type;
	typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
	typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
	
	// meta data type for block reading
	typedef libmaus::bambam::parallel::GenericInputBlockSubBlockInfo meta_type;
	// block type
	typedef libmaus::bambam::parallel::GenericInputBlock<meta_type> block_type;
	// pointer to block type
	typedef block_type::unique_ptr_type block_ptr_type;
	// shared pointer to block type
	typedef block_type::shared_ptr_type shared_block_ptr_type;
	
	// free list type
	typedef libmaus::parallel::LockedFreeList<
		block_type,
		libmaus::bambam::parallel::GenericInputBlockAllocator<meta_type>,
		libmaus::bambam::parallel::GenericInputBlockTypeInfo<meta_type>
	> generic_input_block_free_list_type;
	// pointer to free list type
	typedef generic_input_block_free_list_type::unique_ptr_type generic_input_block_free_list_pointer_type;

	// stall array type
	typedef libmaus::autoarray::AutoArray<uint8_t,libmaus::autoarray::alloc_type_c> stall_array_type;
};

struct GenericInputHeapComparator
{
	bool operator()(GenericInputBase::shared_block_ptr_type const & A, GenericInputBase::shared_block_ptr_type const & B) const
	{
		assert ( A->meta.streamid == B->meta.streamid );
		return A->meta.blockid > B->meta.blockid;
	}
};

struct GenericInputControlSubBlockPending
{
	typedef GenericInputControlSubBlockPending this_type;
	typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
	typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
	
	GenericInputBase::block_type::shared_ptr_type block;
	uint64_t subblockid;

	libmaus::bambam::parallel::MemInputBlock::shared_ptr_type mib;
	libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type db;
	
	GenericInputControlSubBlockPending(
		GenericInputBase::block_type::shared_ptr_type rblock = GenericInputBase::block_type::shared_ptr_type(), 
		uint64_t rsubblockid = 0)
	: block(rblock), subblockid(rsubblockid), mib(), db() {}
};

struct GenericInputControlSubBlockPendingHeapComparator
{
	bool operator()(GenericInputControlSubBlockPending const & A, GenericInputControlSubBlockPending const & B)
	{
		if ( A.block->meta.blockid != B.block->meta.blockid )
			return A.block->meta.blockid > B.block->meta.blockid;
		else
			return A.subblockid > B.subblockid;
	}
};

#include <libmaus/bambam/parallel/MemInputBlock.hpp>

struct DecompressedBlockHeapComparator
{
	bool operator()(
		libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type const & A, 
		libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type const & B
	) const
	{
		if ( A->streamid != B->streamid )
			return A->streamid > B->streamid;
		else
			return A->blockid > B->blockid;
	}
};

struct GenericInputSingleDataBamParseInfo
{
	libmaus::autoarray::AutoArray<uint8_t,libmaus::autoarray::alloc_type_c> parsestallarray;
	size_t parsestallarrayused;

	libmaus::bambam::BamHeaderParserState BHPS;
	bool headercomplete;
	libmaus::bambam::BamHeaderLowMem::unique_ptr_type Pheader;

	GenericInputSingleDataBamParseInfo(bool const rheadercomplete = false)
	: parsestallarray(), parsestallarrayused(0), BHPS(), headercomplete(rheadercomplete), Pheader()
	{
	}

	void setupHeader()
	{
		libmaus::bambam::BamHeaderLowMem::unique_ptr_type tptr(
			libmaus::bambam::BamHeaderLowMem::constructFromText(BHPS.text.begin(),BHPS.text.begin()+BHPS.l_text)
		);
		Pheader = UNIQUE_PTR_MOVE(tptr);
	}

	void parseStallArrayPush(uint8_t const * p, size_t c)
	{
		if ( parsestallarrayused + c > parsestallarray.size() )
			parsestallarray.resize(parsestallarrayused + c);
		
		std::copy(p,p+c,parsestallarray.begin() + parsestallarrayused);
		parsestallarrayused += c;
	}

	void parseStallArrayPush(uint8_t const p)
	{
		if ( parsestallarrayused + 1 > parsestallarray.size() )
			parsestallarray.resize(parsestallarrayused + 1);
		
		parsestallarray[parsestallarrayused++] = p;
	}

	void parseBlock(libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type block)
	{
		uint8_t const * Da = reinterpret_cast<uint8_t const *>(block->D.begin());
		uint8_t const * Dc = Da;
		uint8_t const * De = Da + block->uncompdatasize;
		
		if ( ! this->headercomplete )
		{
			libmaus::util::GetObject<uint8_t const *> G(reinterpret_cast<uint8_t const *>(Da));
			std::pair<bool,uint64_t> const P = this->BHPS.parseHeader(G,De-Da);

			this->headercomplete = P.first;
			Dc += P.second;
		
			if ( this->headercomplete )
				this->setupHeader();
		}
		
		// reset pointer array
		block->resetParseArray();
		
		// anything in stall array?
		if ( this->parsestallarrayused )
		{
			// extend until we have the length of the next record
			while ( this->parsestallarrayused < sizeof(uint32_t) && Dc != De )
				this->parseStallArrayPush(*(Dc++));
			
			// do we have at least the record length now?
			if ( this->parsestallarrayused >= sizeof(uint32_t) )
			{
				// length of record
				uint32_t const n = libmaus::bambam::DecoderBase::getLEInteger(this->parsestallarray.begin(),4);
				// length we already have copied
				uint32_t const alcop = this->parsestallarrayused - sizeof(uint32_t);
				// rest in block
				ptrdiff_t rest = De-Dc;
				// number of bytes to copy
				size_t const tocopy = std::min(static_cast<ptrdiff_t>(n-alcop),rest);
				
				// append data
				this->parseStallArrayPush(Dc,tocopy);
				// update block pointer
				Dc += tocopy;
				
				// full record?
				if ( this->parsestallarrayused == n + sizeof(uint32_t) )
				{
					// get current offset from start of data block
					ptrdiff_t const o = Dc - Da;

					// push record
					block->pushParsePointer(							
						block->appendData(
							this->parsestallarray.begin(),
							this->parsestallarrayused
						)
					);
					
					// mark stall buffer as empty
					this->parsestallarrayused = 0;

					// set new block pointers (appendData may have reallocated the array)
					Da = reinterpret_cast<uint8_t const *>(block->D.begin());
					Dc = Da + o;
					De = Da + block->uncompdatasize;
				}
			}
		}
		
		uint32_t n = 0;
		while ( 
			De-Dc >= static_cast<ptrdiff_t>(sizeof(uint32_t)) &&
			De-Dc >= static_cast<ptrdiff_t>(sizeof(uint32_t) + (n=libmaus::bambam::DecoderBase::getLEInteger(Dc,4)))
		)
		{
			#if 0
			if ( block->final )
				std::cerr << "n=" << n << std::endl;
			#endif

			block->pushParsePointer(reinterpret_cast<char const *>(Dc));
			Dc += n + sizeof(uint32_t);
		}
		
		// overlap with next block?
		if ( Dc != De )
		{
			this->parseStallArrayPush(Dc,De-Dc);
			Dc += De-Dc;
		}
		
		// file truncated?
		if ( block->final && this->parsestallarrayused )
		{
			libmaus::exception::LibMausException lme;
			lme.getStream() << "Stream is truncated (end of file inside a BAM record)" << std::endl;
			lme.finish();
			throw lme;
		}
		
		#if 0
		::libmaus::bambam::BamFormatAuxiliary aux;
		for ( size_t i = 0; i < block->getNumParsePointers(); ++i )
		{
			uint8_t const * p = reinterpret_cast<uint8_t const *>(block->D.begin() + block->PP[i]);
			uint32_t const n = libmaus::bambam::DecoderBase::getLEInteger(p,sizeof(uint32_t));
			libmaus::bambam::BamAlignmentDecoderBase::formatAlignment(
				std::cout, p + sizeof(uint32_t), n, *(data[streamid]->Pheader),aux
			);
			std::cout.put('\n');
		}
		#endif
	
	}
};

struct GenericInputSingleData
{
	typedef GenericInputSingleData this_type;
	typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
	typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

	public:
	// stream, protected by inlock
	std::istream & in;
	// read lock
	libmaus::parallel::PosixSpinLock inlock;
	
	private:
	// stream id, constant, no lock needed
	uint64_t const streamid;
	// block id, protected by inlock
	uint64_t volatile blockid;
	
	public:
	// block size, constant, no lock needed
	uint64_t const blocksize;
	// block free list (has its own locking)
	GenericInputBase::generic_input_block_free_list_type blockFreeList;

	// finite amount of data to be read? constant, no lock needed
	bool const finite;
	// amount of data left if finite == true, protected by inlock
	uint64_t dataleft;

	private:
	// eof, protected by eoflock
	bool volatile eof;
	// lock for eof
	libmaus::parallel::PosixSpinLock eoflock;

	public:
	// stall array, protected by inlock
	GenericInputBase::stall_array_type stallArray;
	// number of bytes in stall array, protected by inlock
	uint64_t volatile stallArraySize;

	// lock
	libmaus::parallel::PosixSpinLock lock;
	
	// next block to be processed, protected by lock
	uint64_t volatile nextblockid;
	// pending queue, protected by lock
	std::priority_queue<
		GenericInputBase::shared_block_ptr_type,
		std::vector<GenericInputBase::shared_block_ptr_type>,
		GenericInputHeapComparator > pending;
		
	// decompression pending queue, protected by lock
	std::priority_queue<
		GenericInputControlSubBlockPending,
		std::vector<GenericInputControlSubBlockPending>,
		GenericInputControlSubBlockPendingHeapComparator
	> decompressionpending;
	// next block to be decompressed, protected by lock
	std::pair<uint64_t,uint64_t> decompressionpendingnext;
	// total number of sub-blocks to be decompressed for each block
	std::vector<uint64_t> decompressiontotal;

	// free list for compressed data meta blocks
	libmaus::parallel::LockedFreeList<
		libmaus::bambam::parallel::MemInputBlock,
		libmaus::bambam::parallel::MemInputBlockAllocator,
		libmaus::bambam::parallel::MemInputBlockTypeInfo
	> meminputblockfreelist;
	
	// free list for uncompressed bgzf blocks
	libmaus::parallel::LockedFreeList<
		libmaus::bambam::parallel::DecompressedBlock,
		libmaus::bambam::parallel::DecompressedBlockAllocator,
		libmaus::bambam::parallel::DecompressedBlockTypeInfo
	> decompressedblockfreelist;
	
	uint64_t volatile decompressedBlockIdAcc;
	uint64_t volatile decompressedBlocksAcc;
	
	uint64_t volatile parsependingnext;
	std::priority_queue<
		libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type,
		std::vector<libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type>,
		DecompressedBlockHeapComparator> parsepending;
		
	GenericInputSingleDataBamParseInfo parseInfo;
	
	std::deque<libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type> processQueue;
	libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type processActive;
	bool volatile processMissing;
	bool volatile processFirst;
	libmaus::parallel::PosixSpinLock processLock;

	GenericInputSingleData(
		std::istream & rin, 
		uint64_t const rstreamid,
		bool const rfinite,
		uint64_t const rdataleft,
		uint64_t const rblocksize, 
		unsigned int const numblocks
	)
	: in(rin), streamid(rstreamid), blockid(0), blocksize(rblocksize), 
	  blockFreeList(numblocks,libmaus::bambam::parallel::GenericInputBlockAllocator<GenericInputBase::meta_type>(blocksize)),
	  finite(rfinite),
	  dataleft(rdataleft),
	  eof(false),
	  stallArray(0),
	  stallArraySize(0),
	  nextblockid(0),
	  meminputblockfreelist(64),
	  decompressedblockfreelist(64),
	  decompressedBlockIdAcc(0),
	  decompressedBlocksAcc(0),
	  parsependingnext(0),
	  parsepending(),
	  parseInfo(),
	  processQueue(),
	  processActive(),
	  processMissing(true),
	  processFirst(true),
	  processLock()
	{
	
	}

	bool getEOF()
	{
		libmaus::parallel::ScopePosixSpinLock slock(eoflock);
		return eof;
	}
	
	void setEOF(bool const reof)
	{
		libmaus::parallel::ScopePosixSpinLock slock(eoflock);
		eof = reof;
	}

	uint64_t getStreamId() const
	{
		return streamid;
	}
	
	uint64_t getBlockId() const
	{
		return blockid;
	}
	
	void incrementBlockId()
	{
		blockid++;
	}
};

struct GenericInputControlReadWorkPackage : public libmaus::parallel::SimpleThreadWorkPackage
{
	typedef GenericInputControlReadWorkPackage this_type;
	typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
	typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

	GenericInputSingleData * data;

	GenericInputControlReadWorkPackage() : libmaus::parallel::SimpleThreadWorkPackage(), data(0) {}
	GenericInputControlReadWorkPackage(uint64_t const rpriority, uint64_t const rdispatcherid, GenericInputSingleData * rdata)
	: libmaus::parallel::SimpleThreadWorkPackage(rpriority,rdispatcherid), data(rdata)
	{
	
	}

	char const * getPackageName() const
	{
		return "GenericInputControlReadWorkPackage";
	}
};

struct GenericInputControlReadWorkPackageReturnInterface
{
	virtual ~GenericInputControlReadWorkPackageReturnInterface() {}
	virtual void genericInputControlReadWorkPackageReturn(GenericInputControlReadWorkPackage * package) = 0;
};

struct GenericInputControlReadAddPendingInterface
{
	virtual ~GenericInputControlReadAddPendingInterface() {}
	virtual void genericInputControlReadAddPending(GenericInputBase::block_type::shared_ptr_type block) = 0;
};

struct GenericInputControlReadWorkPackageDispatcher : public libmaus::parallel::SimpleThreadWorkPackageDispatcher
{
	typedef GenericInputControlReadWorkPackageDispatcher this_type;
	typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
	typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
	
	GenericInputControlReadWorkPackageReturnInterface & packageReturnInterface;
	GenericInputControlReadAddPendingInterface & addPendingInterface;

	GenericInputControlReadWorkPackageDispatcher(
		GenericInputControlReadWorkPackageReturnInterface & rpackageReturnInterface,
		GenericInputControlReadAddPendingInterface & raddPendingInterface
	)
	: packageReturnInterface(rpackageReturnInterface), addPendingInterface(raddPendingInterface)
	{
	
	}

	void dispatch(libmaus::parallel::SimpleThreadWorkPackage * P, libmaus::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */)
	{
		assert ( dynamic_cast<GenericInputControlReadWorkPackage *>(P) != 0 );
		GenericInputControlReadWorkPackage * BP = dynamic_cast<GenericInputControlReadWorkPackage *>(P);
		
		GenericInputSingleData & data = *(BP->data);
		std::vector<GenericInputBase::block_type::shared_ptr_type> fullBlocks;
		
		if ( data.inlock.trylock() )
		{
			libmaus::parallel::ScopePosixSpinLock slock(data.inlock,true /* pre locked */);
			
			GenericInputBase::block_type::shared_ptr_type sblock;

			while ( (!data.getEOF()) && (sblock=data.blockFreeList.getIf()) )
			{
				// reset buffer
				sblock->reset();
				// set stream id
				sblock->meta.streamid = data.getStreamId();
				// set block id
				sblock->meta.blockid = data.getBlockId();
				// insert stalled data
				sblock->insert(data.stallArray.begin(),data.stallArray.begin()+data.stallArraySize);

				// extend if there is no space
				if ( sblock->pe == sblock->A.end() )
					sblock->extend();
				
				// there should be free space now
				assert ( sblock->pe != sblock->A.end() );

				// fill buffer
				libmaus::bambam::parallel::GenericInputBlockFillResult const P = sblock->fill(
					data.in,
					data.finite,
					data.dataleft);

				if ( data.in.bad() )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "Stream error while filling buffer." << std::endl;
					lme.finish();
					throw lme;
				}

				if ( data.finite )
				{
					assert ( P.gcount <= data.dataleft );
					data.dataleft -= P.gcount;
				}

				data.setEOF(P.eof);
				sblock->meta.eof = P.eof;
				
				// parse bgzf block headers to determine how many full blocks we have				
				int64_t bs = -1;
				libmaus::bambam::parallel::GenericInputBlockSubBlockInfo & meta = sblock->meta;
				uint64_t f = 0;
				while ( (bs=libmaus::lz::BgzfInflateHeaderBase::getBlockSize(sblock->pc,sblock->pe)) >= 0 )
				{
					meta.addBlock(std::pair<uint8_t *,uint8_t *>(sblock->pc,sblock->pc+bs));
					sblock->pc += bs;
					f += 1;
				}

				#if 0
				// empty file? inject eof block
				if ( data.getEOF() && (sblock->pc == sblock->pe) && (!f) )
				{
					std::ostringstream ostr;
					libmaus::lz::BgzfDeflate<std::ostream> bgzfout(ostr);
					bgzfout.addEOFBlock();
					std::string const s = ostr.str();
					sblock->insert(s.begin(),s.end());
					
					meta.addBlock(std::pair<uint8_t *,uint8_t *>(sblock->pc,sblock->pe));
					f += 1;
				}
				#endif
				
				// empty file?
				if ( data.getEOF() && (sblock->pc == sblock->pe) && (!f) )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "Invalid empty BGZF stream." << std::endl;
					lme.finish();
					throw lme;
				}

				// extract rest of data for next block
				data.stallArraySize = sblock->extractRest(data.stallArray);

				if ( f )
				{
					fullBlocks.push_back(sblock);
					data.incrementBlockId();
				}
				else
				{
					// buffer is too small for a single block
					if ( ! data.getEOF() )
					{
						sblock->extend();
						data.blockFreeList.put(sblock);
					}
					else if ( sblock->pc != sblock->pe )
					{
						assert ( data.getEOF() );
						// throw exception, block is incomplete at EOF
						libmaus::exception::LibMausException lme;
						lme.getStream() << "Unexpected EOF." << std::endl;
						lme.finish();
						throw lme;
					}
				}
			}
		}

		packageReturnInterface.genericInputControlReadWorkPackageReturn(BP);

		for ( uint64_t i = 0; i < fullBlocks.size(); ++i )
		{
			addPendingInterface.genericInputControlReadAddPending(fullBlocks[i]);
		}		
	}
};

struct GenericInputBgzfDecompressionWorkPackage : public libmaus::parallel::SimpleThreadWorkPackage
{
	typedef GenericInputBgzfDecompressionWorkPackage this_type;
	typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
	typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

	GenericInputControlSubBlockPending data;

	GenericInputBgzfDecompressionWorkPackage() : libmaus::parallel::SimpleThreadWorkPackage(), data(0) {}
	GenericInputBgzfDecompressionWorkPackage(uint64_t const rpriority, uint64_t const rdispatcherid, GenericInputControlSubBlockPending rdata)
	: libmaus::parallel::SimpleThreadWorkPackage(rpriority,rdispatcherid), data(rdata)
	{
	
	}

	char const * getPackageName() const
	{
		return "GenericInputBgzfDecompressionWorkPackage";
	}
};

struct GenericInputBgzfDecompressionWorkPackageReturnInterface
{
	virtual ~GenericInputBgzfDecompressionWorkPackageReturnInterface() {}
	virtual void genericInputBgzfDecompressionWorkPackageReturn(GenericInputBgzfDecompressionWorkPackage * package) = 0;
};

struct GenericInputBgzfDecompressionWorkPackageMemInputBlockReturnInterface
{
	virtual ~GenericInputBgzfDecompressionWorkPackageMemInputBlockReturnInterface() {}
	virtual void genericInputBgzfDecompressionWorkPackageMemInputBlockReturn(uint64_t streamid, libmaus::bambam::parallel::MemInputBlock::shared_ptr_type ptr) = 0;
};

struct GenericInputBgzfDecompressionWorkPackageDecompressedBlockReturnInterface
{
	virtual ~GenericInputBgzfDecompressionWorkPackageDecompressedBlockReturnInterface() {}
	virtual void genericInputBgzfDecompressionWorkPackageDecompressedBlockReturn(uint64_t streamid, libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type ptr) = 0;
};

struct GenericInputBgzfDecompressionWorkSubBlockDecompressionFinishedInterface
{
	virtual ~GenericInputBgzfDecompressionWorkSubBlockDecompressionFinishedInterface() {}
	virtual void genericInputBgzfDecompressionWorkSubBlockDecompressionFinished(
		GenericInputBase::block_type::shared_ptr_type ptr, uint64_t subblockid
	) = 0;
};

struct GenericInputBgzfDecompressionWorkPackageDispatcher : public libmaus::parallel::SimpleThreadWorkPackageDispatcher
{
	typedef GenericInputControlReadWorkPackageDispatcher this_type;
	typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
	typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
	
	GenericInputBgzfDecompressionWorkPackageReturnInterface & packageReturnInterface;
	GenericInputBgzfDecompressionWorkPackageMemInputBlockReturnInterface & genericInputBgzfDecompressionWorkPackageMemInputBlockReturn;
	GenericInputBgzfDecompressionWorkPackageDecompressedBlockReturnInterface & genericInputBgzfDecompressionWorkPackageDecompressedBlockReturn;
	GenericInputBgzfDecompressionWorkSubBlockDecompressionFinishedInterface & genericInputBgzfDecompressionWorkSubBlockDecompressionFinished;

	libmaus::parallel::LockedFreeList<
		libmaus::lz::BgzfInflateZStreamBase,
		libmaus::lz::BgzfInflateZStreamBaseAllocator,
		libmaus::lz::BgzfInflateZStreamBaseTypeInfo
	> & deccont;


	GenericInputBgzfDecompressionWorkPackageDispatcher(
		GenericInputBgzfDecompressionWorkPackageReturnInterface & rpackageReturnInterface,
		GenericInputBgzfDecompressionWorkPackageMemInputBlockReturnInterface & rgenericInputBgzfDecompressionWorkPackageMemInputBlockReturn,
		GenericInputBgzfDecompressionWorkPackageDecompressedBlockReturnInterface & rgenericInputBgzfDecompressionWorkPackageDecompressedBlockReturn,
		GenericInputBgzfDecompressionWorkSubBlockDecompressionFinishedInterface & rgenericInputBgzfDecompressionWorkSubBlockDecompressionFinished,
		libmaus::parallel::LockedFreeList<
			libmaus::lz::BgzfInflateZStreamBase,
			libmaus::lz::BgzfInflateZStreamBaseAllocator,
			libmaus::lz::BgzfInflateZStreamBaseTypeInfo
		> & rdeccont
	)
	: packageReturnInterface(rpackageReturnInterface),
	  genericInputBgzfDecompressionWorkPackageMemInputBlockReturn(rgenericInputBgzfDecompressionWorkPackageMemInputBlockReturn),
	  genericInputBgzfDecompressionWorkPackageDecompressedBlockReturn(rgenericInputBgzfDecompressionWorkPackageDecompressedBlockReturn),
	  genericInputBgzfDecompressionWorkSubBlockDecompressionFinished(rgenericInputBgzfDecompressionWorkSubBlockDecompressionFinished),
	  deccont(rdeccont)
	{
	
	}

	void dispatch(libmaus::parallel::SimpleThreadWorkPackage * P, libmaus::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */)
	{
		assert ( dynamic_cast<GenericInputBgzfDecompressionWorkPackage *>(P) != 0 );
		GenericInputBgzfDecompressionWorkPackage * BP = dynamic_cast<GenericInputBgzfDecompressionWorkPackage *>(P);

		GenericInputControlSubBlockPending & data = BP->data;
		
		GenericInputBase::block_type::shared_ptr_type block = data.block;
		uint64_t const subblockid = data.subblockid;

		// parse header data
		data.mib->readBlock(
			block->meta.blocks[subblockid].first,
			block->meta.eof && ((subblockid+1) == block->meta.blocks.size())
		);
		// decompress block
		data.db->decompressBlock(deccont,*(data.mib));
		// set block id
		data.db->blockid = data.mib->blockid;
		// set stream id
		data.db->streamid = data.mib->streamid;
		// compute crc32
		uint32_t const crc = data.db->computeCrc();
				
		if ( crc != data.mib->crc )
		{
			libmaus::exception::LibMausException lme;
			lme.getStream() << "CRC mismatch." << std::endl;
			lme.finish();
			throw lme;
		}

		// return decompressed block		
		genericInputBgzfDecompressionWorkPackageDecompressedBlockReturn.genericInputBgzfDecompressionWorkPackageDecompressedBlockReturn(block->meta.streamid,data.db);		
		// return compressed block meta info
		genericInputBgzfDecompressionWorkPackageMemInputBlockReturn.genericInputBgzfDecompressionWorkPackageMemInputBlockReturn(block->meta.streamid,data.mib);
		// return input block
		genericInputBgzfDecompressionWorkSubBlockDecompressionFinished.genericInputBgzfDecompressionWorkSubBlockDecompressionFinished(block,subblockid);	
		// return work package
		packageReturnInterface.genericInputBgzfDecompressionWorkPackageReturn(BP);
	}
};

struct GenericInputBamParseObject
{
	typedef GenericInputBamParseObject this_type;
	typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
	typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

	GenericInputSingleDataBamParseInfo * parseInfo;
	libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type block;
	
	GenericInputBamParseObject() : parseInfo(0), block() {}
	GenericInputBamParseObject(
		GenericInputSingleDataBamParseInfo * rparseInfo,
		libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type rblock
	) : parseInfo(rparseInfo), block(rblock)
	{
	}
	
	void dispatch()
	{
		parseInfo->parseBlock(block);
	}
};

struct GenericInputBamParseWorkPackage : public libmaus::parallel::SimpleThreadWorkPackage
{
	typedef GenericInputBamParseWorkPackage this_type;
	typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
	typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

	GenericInputBamParseObject data;

	GenericInputBamParseWorkPackage() : libmaus::parallel::SimpleThreadWorkPackage(), data() {}
	GenericInputBamParseWorkPackage(uint64_t const rpriority, uint64_t const rdispatcherid, GenericInputBamParseObject rdata)
	: libmaus::parallel::SimpleThreadWorkPackage(rpriority,rdispatcherid), data(rdata)
	{
	
	}

	char const * getPackageName() const
	{
		return "GenericInputBamParseObject";
	}
};

struct GenericInputBamParseWorkPackageReturnInterface
{
	virtual ~GenericInputBamParseWorkPackageReturnInterface() {}
	virtual void genericInputBamParseWorkPackageReturn(GenericInputBamParseWorkPackage * package) = 0;
};

struct GenericInputBamParseWorkPackageBlockParsedInterface
{
	virtual ~GenericInputBamParseWorkPackageBlockParsedInterface() {}
	virtual void genericInputBamParseWorkPackageBlockParsed(libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type block) = 0;
};


struct GenericInputBamParseWorkPackageDispatcher : public libmaus::parallel::SimpleThreadWorkPackageDispatcher
{
	typedef GenericInputBamParseWorkPackageDispatcher this_type;
	typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
	typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
	
	GenericInputBamParseWorkPackageReturnInterface & packageReturnInterface;
	GenericInputBamParseWorkPackageBlockParsedInterface & blockParsedInterface;

	GenericInputBamParseWorkPackageDispatcher(
		GenericInputBamParseWorkPackageReturnInterface & rpackageReturnInterface,
		GenericInputBamParseWorkPackageBlockParsedInterface & rblockParsedInterface
	)
	: packageReturnInterface(rpackageReturnInterface),
	  blockParsedInterface(rblockParsedInterface)
	{
	
	}

	void dispatch(libmaus::parallel::SimpleThreadWorkPackage * P, libmaus::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */)
	{
		assert ( dynamic_cast<GenericInputBamParseWorkPackage *>(P) != 0 );
		GenericInputBamParseWorkPackage * BP = dynamic_cast<GenericInputBamParseWorkPackage *>(P);
		
		BP->data.dispatch();
		
		blockParsedInterface.genericInputBamParseWorkPackageBlockParsed(BP->data.block);
		packageReturnInterface.genericInputBamParseWorkPackageReturn(BP);
	}
};

struct GenericInputControlMergeHeapEntry
{
	libmaus::bambam::parallel::DecompressedBlock * block;
	uint64_t coord;
	
	inline void setup()
	{
		uint8_t const * algn4 = reinterpret_cast<uint8_t const *>(block->getNextParsePointer()) + sizeof(uint32_t);
		coord = 
			(static_cast<uint64_t>(static_cast<uint32_t>(libmaus::bambam::BamAlignmentDecoderBase::getRefID(algn4))) << 32)
			|
			static_cast<uint64_t>(static_cast<int64_t>(libmaus::bambam::BamAlignmentDecoderBase::getPos(algn4)) - std::numeric_limits<int32_t>::min());
			;	
	}
	
	GenericInputControlMergeHeapEntry()
	{
	
	}
	GenericInputControlMergeHeapEntry(libmaus::bambam::parallel::DecompressedBlock * rblock)
	: block(rblock)
	{
		setup();
	}
	
	bool isLast() const
	{
		return block->cPP == block->nPP;
	}
	
	bool operator<(GenericInputControlMergeHeapEntry const & A) const
	{
		if ( coord != A.coord )
			return coord < A.coord;
		else
			return block->streamid < A.block->streamid;
	}
};


template<typename _element_type, typename _comparator_type = std::less<_element_type> >
struct FiniteSizeHeap
{
	typedef _element_type element_type;
	typedef _comparator_type comparator_type;
	typedef FiniteSizeHeap<element_type,comparator_type> this_type;
	typedef typename libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
	typedef typename libmaus::util::shared_ptr<this_type>::type shared_ptr_type;	
	
	libmaus::autoarray::AutoArray<element_type> H;
	size_t f;
	comparator_type comp;
	
	FiniteSizeHeap(uint64_t const size, comparator_type rcomp = comparator_type())
	: H(size,false), f(0), comp(rcomp)
	{
	
	}
	
	bool empty() const
	{
		return (!f);
	}
	
	bool full() const
	{
		return f == H.size();
	}
	
	void push(element_type const & entry)
	{
		size_t i = f++;
		H[i] = entry;
		
		// while not root
		while ( i )
		{
			// parent index
			size_t p = (i-1)>>1;
			
			// order wrong?
			if ( comp(H[i],H[p]) )
			{
				std::swap(H[i],H[p]);
				i = p;
			}
			// order correct, break loop
			else
			{
				break;
			}
		}
	}
	
	void pop(element_type & E)
	{
		E = H[0];
		// put last element at root
		H[0] = H[--f];
		
		size_t i = 0;
		size_t r;
	
		// while both children exist	
		while ( (r=((i<<1)+2)) < f )
		{
			size_t const m = comp(H[r-1],H[r]) ? r-1 : r;
			
			// order correct?
			if ( comp(H[i],H[m]) )
				return;
			
			std::swap(H[i],H[m]);
			i = m;
		}
		
		// does left child exist?
		size_t l;
		if ( ((l = ((i<<1)+1)) < f) && (!(comp(H[i],H[l]))) )
			std::swap(H[i],H[l]);
	}
	
	element_type pop()
	{
		element_type E;
		pop(E);
		return E;
	}
};

struct GenericInputMergeDecompressedBlockReturnInterface
{
	virtual ~GenericInputMergeDecompressedBlockReturnInterface() {}
	virtual void genericInputMergeDecompressedBlockReturn(libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type block) = 0;
};

struct GenericInputMergeWorkPackage : public libmaus::parallel::SimpleThreadWorkPackage
{
	typedef GenericInputMergeWorkPackage this_type;
	typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
	typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

	libmaus::autoarray::AutoArray<GenericInputSingleData::unique_ptr_type> * data;
	FiniteSizeHeap<GenericInputControlMergeHeapEntry> * mergeheap;

	GenericInputMergeWorkPackage() : libmaus::parallel::SimpleThreadWorkPackage(), data(0), mergeheap(0) {}
	GenericInputMergeWorkPackage(
		uint64_t const rpriority, 
		uint64_t const rdispatcherid, 
		libmaus::autoarray::AutoArray<GenericInputSingleData::unique_ptr_type> * rdata,
		FiniteSizeHeap<GenericInputControlMergeHeapEntry> * rmergeheap
	)
	: libmaus::parallel::SimpleThreadWorkPackage(rpriority,rdispatcherid), data(rdata), mergeheap(rmergeheap)
	{
	
	}

	char const * getPackageName() const
	{
		return "GenericInputMergeWorkPackage";
	}
};	


struct GenericInputMergeWorkPackageReturnInterface
{
	virtual ~GenericInputMergeWorkPackageReturnInterface() {}
	virtual void genericInputMergeWorkPackageReturn(GenericInputMergeWorkPackage * package) = 0;
};


struct GenericInputMergeWorkPackageDispatcher : public libmaus::parallel::SimpleThreadWorkPackageDispatcher
{
	typedef GenericInputMergeWorkPackageDispatcher this_type;
	typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
	typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
	
	GenericInputMergeWorkPackageReturnInterface & packageReturnInterface;
	GenericInputMergeDecompressedBlockReturnInterface & decompressPackageReturnInterface;

	GenericInputMergeWorkPackageDispatcher(
		GenericInputMergeWorkPackageReturnInterface & rpackageReturnInterface,
		GenericInputMergeDecompressedBlockReturnInterface & rdecompressPackageReturnInterface
	)
	: packageReturnInterface(rpackageReturnInterface),
	  decompressPackageReturnInterface(rdecompressPackageReturnInterface)
	{
	
	}

	void dispatch(libmaus::parallel::SimpleThreadWorkPackage * P, libmaus::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */)
	{
		assert ( dynamic_cast<GenericInputMergeWorkPackage *>(P) != 0 );
		GenericInputMergeWorkPackage * BP = dynamic_cast<GenericInputMergeWorkPackage *>(P);
		
		libmaus::autoarray::AutoArray<GenericInputSingleData::unique_ptr_type> & data = *(BP->data);
		FiniteSizeHeap<GenericInputControlMergeHeapEntry> & mergeheap = *(BP->mergeheap);

		GenericInputControlMergeHeapEntry E;
		bool running = !mergeheap.empty();
		
		while ( running )
		{
			mergeheap.pop(E);

			#if 0
			libmaus::bambam::parallel::DecompressedBlock * eblock = E.block;
			uint8_t const * ublock = reinterpret_cast<uint8_t const *>(eblock->getPrevParsePointer());
			uint8_t const * ublock4 = ublock + sizeof(uint32_t);
			char const * name = libmaus::bambam::BamAlignmentDecoderBase::getReadName(ublock4);
			std::cerr << name 
				<< "\t" << libmaus::bambam::BamAlignmentDecoderBase::getRefID(ublock4) 
				<< "\t" << libmaus::bambam::BamAlignmentDecoderBase::getPos(ublock4) 
				<< std::endl;
			#endif
			
			// last alignment in block
			if ( E.isLast() )
			{
				// get stream id
				uint64_t const streamid = E.block->streamid;
				// get block
				libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type block = data[streamid]->processActive;
				// return block
				decompressPackageReturnInterface.genericInputMergeDecompressedBlockReturn(block);

				{	
					// get lock	
					libmaus::parallel::ScopePosixSpinLock qlock(data[streamid]->processLock);
					// erase block pointer
					data[streamid]->processActive = libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type();

					// if queue is not empty
					if ( data[streamid]->processQueue.size() )
					{
						// get next block
						libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type nextblock = data[streamid]->processQueue.front();
						// remove it from the queue
						data[streamid]->processQueue.pop_front();
						
						// this should be either non-empty or the last block
						assert ( nextblock->getNumParsePointers() || nextblock->final );

						// last block, return it			
						if ( nextblock->final )
						{
							// return block
							decompressPackageReturnInterface.genericInputMergeDecompressedBlockReturn(nextblock);
							// stop running if this was the last stream ending
							running = !(mergeheap.empty());
						}
						// non-empty block, get next alignment
						else
						{
							// set active block
							data[streamid]->processActive = nextblock;
							// this should be non-empty
							assert ( data[streamid]->processActive->getNumParsePointers() );
							// construct heap entry
							GenericInputControlMergeHeapEntry GICMHE(data[streamid]->processActive.get());
							// push it
							mergeheap.push(GICMHE);
						}
					}
					// nothing in queue, mark channel as missing and leave loop
					else
					{
						// missing
						data[streamid]->processMissing = true;
						// quit loop
						running = false;
					}
				}				
			}
			// not last element in block
			else
			{
				// construct heap entry
				GenericInputControlMergeHeapEntry GICMHE(data[E.block->streamid]->processActive.get());
				// push it
				mergeheap.push(GICMHE);			
			}
		}
		
		packageReturnInterface.genericInputMergeWorkPackageReturn(BP);
	}
};

struct GenericInputControl : 
	public GenericInputControlReadWorkPackageReturnInterface,
	public GenericInputControlReadAddPendingInterface,
	public GenericInputBgzfDecompressionWorkPackageReturnInterface,
	public GenericInputBgzfDecompressionWorkPackageMemInputBlockReturnInterface,
	public GenericInputBgzfDecompressionWorkPackageDecompressedBlockReturnInterface,
	public GenericInputBgzfDecompressionWorkSubBlockDecompressionFinishedInterface,
	public GenericInputBamParseWorkPackageReturnInterface,
	public GenericInputBamParseWorkPackageBlockParsedInterface,
	public GenericInputMergeWorkPackageReturnInterface,
	public GenericInputMergeDecompressedBlockReturnInterface
{
	typedef GenericInputControl this_type;
	typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
	typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

	libmaus::parallel::SimpleThreadPool & STP;
	
	libmaus::autoarray::AutoArray<GenericInputSingleData::unique_ptr_type> data;

	libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<GenericInputControlReadWorkPackage> readWorkPackages;
	libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<GenericInputBgzfDecompressionWorkPackage> decompressWorkPackages;
	libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<GenericInputBamParseWorkPackage> parseWorkPackages;
	libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<GenericInputMergeWorkPackage> mergeWorkPackages;

	libmaus::parallel::LockedFreeList<
		libmaus::lz::BgzfInflateZStreamBase,
		libmaus::lz::BgzfInflateZStreamBaseAllocator,
		libmaus::lz::BgzfInflateZStreamBaseTypeInfo
	> deccont;

	uint64_t const GICRPDid;
	GenericInputControlReadWorkPackageDispatcher GICRPD;
	
	uint64_t const GIBDWPDid;
	GenericInputBgzfDecompressionWorkPackageDispatcher GIBDWPD;
	
	uint64_t const GIBPWPDid;
	GenericInputBamParseWorkPackageDispatcher GIBPWPD;
	
	uint64_t const GIMWPDid;
	GenericInputMergeWorkPackageDispatcher GIMWPD;
	
	uint64_t volatile activedecompressionstreams;
	libmaus::parallel::PosixSpinLock activedecompressionstreamslock;
	
	uint64_t volatile streamParseUnstarted;
	libmaus::parallel::PosixSpinLock streamParseUnstartedLock;

	FiniteSizeHeap<GenericInputControlMergeHeapEntry> mergeheap;
	libmaus::parallel::PosixSpinLock mergeheaplock;

	GenericInputControl(
		libmaus::parallel::SimpleThreadPool & rSTP,
		std::vector<std::istream *> const & in, uint64_t const blocksize, uint64_t const numblocks
	)
	: STP(rSTP), data(in.size()), deccont(STP.getNumThreads()), GICRPDid(STP.getNextDispatcherId()), GICRPD(*this,*this),
	  GIBDWPDid(STP.getNextDispatcherId()), GIBDWPD(*this,*this,*this,*this,deccont),
	  GIBPWPDid(STP.getNextDispatcherId()), GIBPWPD(*this,*this),
	  GIMWPDid(STP.getNextDispatcherId()), GIMWPD(*this,*this),
	  activedecompressionstreams(in.size()), activedecompressionstreamslock(),
	  streamParseUnstarted(in.size()), streamParseUnstartedLock(),
	  mergeheap(in.size()), mergeheaplock()
	{
		for ( std::vector<std::istream *>::size_type i = 0; i < in.size(); ++i )
		{
			GenericInputSingleData::unique_ptr_type tptr(
				new GenericInputSingleData(*in[i],i,false /* finite */,0/* data left*/,blocksize,numblocks)
			);
			data[i] = UNIQUE_PTR_MOVE(tptr);
		}
		STP.registerDispatcher(GICRPDid,&GICRPD);
		STP.registerDispatcher(GIBDWPDid,&GIBDWPD);
		STP.registerDispatcher(GIBPWPDid,&GIBPWPD);
		STP.registerDispatcher(GIMWPDid,&GIMWPD);
	}
	
	uint64_t getActiveDecompressionStreams()
	{
		libmaus::parallel::ScopePosixSpinLock slock(activedecompressionstreamslock);
		return activedecompressionstreams;
	}
	
	bool decompressionFinished()
	{
		return getActiveDecompressionStreams() == 0;
	}
	
	void waitDecompressionFinished()
	{
		while ( 
			! decompressionFinished() && ! STP.isInPanicMode()
		)
			sleep(1);
	}
	
	void addPending()
	{
		for ( uint64_t streamid = 0; streamid < data.size(); ++streamid )
		{
			GenericInputControlReadWorkPackage * package = readWorkPackages.getPackage();
			*package = GenericInputControlReadWorkPackage(0 /* prio */, GICRPDid, data[streamid].get());
			STP.enque(package);		
		}
	}
	
	void genericInputControlReadWorkPackageReturn(GenericInputControlReadWorkPackage * package)
	{
		readWorkPackages.returnPackage(package);
	}

	void genericInputBgzfDecompressionWorkPackageReturn(GenericInputBgzfDecompressionWorkPackage * package)
	{
		decompressWorkPackages.returnPackage(package);
	}

	void genericInputBamParseWorkPackageReturn(GenericInputBamParseWorkPackage * package)
	{
		parseWorkPackages.returnPackage(package);	
	}

	void genericInputMergeWorkPackageReturn(GenericInputMergeWorkPackage * package)
	{
		mergeWorkPackages.returnPackage(package);
	}

	void genericInputBgzfDecompressionWorkPackageMemInputBlockReturn(uint64_t streamid, libmaus::bambam::parallel::MemInputBlock::shared_ptr_type ptr)
	{
		data[streamid]->meminputblockfreelist.put(ptr);
		checkDecompressionBlockPending(streamid);
	}

	void genericInputMergeDecompressedBlockReturn(libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type block)
	{
		// get stream id
		uint64_t const streamid = block->streamid;
		// return block
		data[streamid]->decompressedblockfreelist.put(block);
		// check whether we can decompress more
		checkDecompressionBlockPending(streamid);	
	}

	void genericInputBamParseWorkPackageBlockParsed(libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type block)
	{
		// stream id
		uint64_t const streamid = block->streamid;
		// is this the final block for this stream?
		bool const final = block->final;
		
		// enque parsed block for processing/merging if it is not empty or the last block of the stream
		{
			libmaus::parallel::ScopePosixSpinLock qlock(data[streamid]->processLock);
			// last block or block containing alignments
			if ( block->final || block->getNumParsePointers() )
			{
				data[streamid]->processQueue.push_back(block);
			}
			// empty block and not last, return it
			else
			{
				// return block
				data[streamid]->decompressedblockfreelist.put(block);
				// check whether we can decompress more
				checkDecompressionBlockPending(streamid);
			}
		}

		// ready for next block
		{
			libmaus::parallel::ScopePosixSpinLock slock(data[streamid]->lock);
			data[streamid]->parsependingnext += 1;
		}

		// check for next parse block
		checkParsePending(streamid);
		
		{
			// lock merge data structures
			libmaus::parallel::ScopePosixSpinLock qlock(data[streamid]->processLock);

			// is this stream missing for merging? do we have a block?
			if ( data[streamid]->processMissing && data[streamid]->processQueue.size() )
			{
				// block is supposed to be unset
				assert ( ! data[streamid]->processActive.get() );
				// get next block from queue
				libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type qblock = data[streamid]->processQueue.front();
				// deque
				data[streamid]->processQueue.pop_front();

				// no longer missing
				data[streamid]->processMissing = 0;
			
				// block with entries
				if ( qblock->getNumParsePointers() )
				{
					// set active block
					data[streamid]->processActive = qblock;

					// insert alignment in heap
					GenericInputControlMergeHeapEntry GICMHE(data[streamid]->processActive.get());
					libmaus::parallel::ScopePosixSpinLock slock(mergeheaplock);
					mergeheap.push(GICMHE);
				}
				// empty block
				else
				{
					// return block
					data[streamid]->decompressedblockfreelist.put(qblock);
					// check whether we can decompress more
					checkDecompressionBlockPending(streamid);					
				}

				// is this the first package?
				if ( data[streamid]->processFirst )
				{
					// no longer first
					data[streamid]->processFirst = false;	

					// update number of initialised streams
					libmaus::parallel::ScopePosixSpinLock lstreamParseUnstartedLock(streamParseUnstartedLock);
					streamParseUnstarted -= 1;

					if ( ! streamParseUnstarted )
					{
						// (re)start merging
						GenericInputMergeWorkPackage * package = mergeWorkPackages.getPackage();
						*package = GenericInputMergeWorkPackage(0/*prio*/,GIMWPDid,&data,&mergeheap);
						STP.enque(package);
					}
				}
				// not first package, merging has already started
				else
				{
					// (re)start merging
					GenericInputMergeWorkPackage * package = mergeWorkPackages.getPackage();
					*package = GenericInputMergeWorkPackage(0/*prio*/,GIMWPDid,&data,&mergeheap);
					STP.enque(package);	
				}					
			}
		}
		
		{
			libmaus::parallel::ScopePosixSpinLock slock(activedecompressionstreamslock);
			if ( final )
				activedecompressionstreams -= 1;
		}
	}
	
	void checkParsePending(uint64_t const streamid)
	{
		libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type block;
		
		{
			libmaus::parallel::ScopePosixSpinLock slock(data[streamid]->lock);
			if ( 
				data[streamid]->parsepending.size() &&
				data[streamid]->parsepending.top()->blockid == data[streamid]->parsependingnext
			)
			{
				block = data[streamid]->parsepending.top();
				data[streamid]->parsepending.pop();
			}
		}
		
		if ( block && block->final )
		{
			std::cerr << "[V] all blocks decompressed for stream " << streamid << std::endl;
		}

		if ( block )
		{
			GenericInputBamParseWorkPackage * package = parseWorkPackages.getPackage();
			*package = GenericInputBamParseWorkPackage(0/*prio*/,GIBPWPDid,GenericInputBamParseObject(&(data[streamid]->parseInfo),block));
			STP.enque(package);
		}
	}

	// bgzf block decompressed
	virtual void genericInputBgzfDecompressionWorkPackageDecompressedBlockReturn(uint64_t streamid, libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type ptr)
	{
		{
			libmaus::parallel::ScopePosixSpinLock slock(data[streamid]->lock);
			data[streamid]->decompressedBlocksAcc += 1;
			data[streamid]->parsepending.push(ptr);

			#if 0			
			if ( ptr->final )
				std::cerr << "stream " << streamid << " fully decompressed " << data[streamid]->decompressedBlockIdAcc << std::endl;			
			#endif
		}
		
		checkParsePending(streamid);
	}

	virtual void genericInputBgzfDecompressionWorkSubBlockDecompressionFinished(
		GenericInputBase::block_type::shared_ptr_type block, uint64_t /* subblockid */
	)
	{
		uint64_t const streamid = block->meta.streamid;

		// check whether this block is completely decompressed now		
		if ( block->meta.returnBlock() )
		{
			data[streamid]->blockFreeList.put(block);

			bool const eof = data[streamid]->getEOF();
		
			// not yet eof? try to read on	
			if ( ! eof )
			{
				GenericInputControlReadWorkPackage * package = readWorkPackages.getPackage();
				*package = GenericInputControlReadWorkPackage(0 /* prio */, GICRPDid, data[streamid].get());
				STP.enque(package);
			}
		}	
	}

	void checkDecompressionBlockPending(uint64_t const streamid)
	{
		libmaus::parallel::ScopePosixSpinLock slock(data[streamid]->lock);

		libmaus::bambam::parallel::MemInputBlock::shared_ptr_type mib;
		libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type db;
		
		while (
			data[streamid]->decompressionpending.size() &&
			data[streamid]->decompressionpending.top().block->meta.blockid == data[streamid]->decompressionpendingnext.first &&
			data[streamid]->decompressionpending.top().subblockid == data[streamid]->decompressionpendingnext.second &&
			(mib = data[streamid]->meminputblockfreelist.getIf()) &&
			(db = data[streamid]->decompressedblockfreelist.getIf())
		)
		{
			// input block id
			uint64_t const blockid = data[streamid]->decompressionpending.top().block->meta.blockid;
			
			GenericInputControlSubBlockPending pend = data[streamid]->decompressionpending.top();
			data[streamid]->decompressionpending.pop();
			// meta compressed block
			pend.mib = mib;
			// copy stream id
			pend.mib->streamid = streamid;
			// assign block id
			pend.mib->blockid = data[streamid]->decompressedBlockIdAcc++;
			// decompressed block
			pend.db = db;
			db = libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type();
			mib = libmaus::bambam::parallel::MemInputBlock::shared_ptr_type();

			GenericInputBgzfDecompressionWorkPackage * package = decompressWorkPackages.getPackage();
			*package = GenericInputBgzfDecompressionWorkPackage(0/*prio*/,GIBDWPDid,pend);
			STP.enque(package);

			data[streamid]->decompressionpendingnext.second += 1;					
			if ( data[streamid]->decompressionpendingnext.second == data[streamid]->decompressiontotal[blockid] )
			{
				data[streamid]->decompressionpendingnext.first += 1;
				data[streamid]->decompressionpendingnext.second = 0;
			}
		}

		if ( db )
			data[streamid]->decompressedblockfreelist.put(db);
		if ( mib )
			data[streamid]->meminputblockfreelist.put(mib);
	}
	
	void checkInputBlockPending(uint64_t const streamid)
	{
		std::vector<GenericInputBase::block_type::shared_ptr_type> readylist;
		
		{
			libmaus::parallel::ScopePosixSpinLock slock(data[streamid]->lock);
				
			while (
				data[streamid]->pending.size() &&
				(
					data[streamid]->pending.top()->meta.blockid == 
					data[streamid]->nextblockid
				)
			)
			{
				GenericInputBase::block_type::shared_ptr_type block = data[streamid]->pending.top();
				data[streamid]->pending.pop();
				readylist.push_back(block);	
				data[streamid]->decompressiontotal.push_back(block->meta.blocks.size());
				data[streamid]->nextblockid += 1;		
			}
		}
		
		// enque decompression requests in queue
		for ( std::vector<GenericInputBase::block_type::shared_ptr_type>::size_type i = 0; i < readylist.size(); ++i )
		{
			// get block
			GenericInputBase::block_type::shared_ptr_type block = readylist[i];

			// mark sub blocks as pending
			for ( uint64_t j = 0; j < block->meta.blocks.size(); ++j )
			{
				libmaus::parallel::ScopePosixSpinLock slock(data[streamid]->lock);
				data[streamid]->decompressionpending.push(GenericInputControlSubBlockPending(block,j));
			}
		}
		
		checkDecompressionBlockPending(streamid);
	}

	void genericInputControlReadAddPending(GenericInputBase::block_type::shared_ptr_type block)
	{
		uint64_t const streamid = block->meta.streamid;
		
		{
			libmaus::parallel::ScopePosixSpinLock slock(data[streamid]->lock);
			data[streamid]->pending.push(block);
		}
		
		checkInputBlockPending(streamid);
	}
};

int main(int argc, char * argv[])
{
	try
	{
		{
			libmaus::util::ArgInfo const arginfo(argc,argv);
			uint64_t const numlogcpus = arginfo.getValue<int>("threads",libmaus::parallel::NumCpus::getNumLogicalProcessors());
			
			std::vector<libmaus::aio::PosixFdInputStream::shared_ptr_type> Pin;
			std::vector<std::istream *> in;
			for ( uint64_t i = 0; i < arginfo.restargs.size(); ++i )
			{
				std::string const fn = arginfo.restargs[i];
				libmaus::aio::PosixFdInputStream::shared_ptr_type PFIS(new libmaus::aio::PosixFdInputStream(fn));
				Pin.push_back(PFIS);
				in.push_back(PFIS.get());
			}
			
			uint64_t const blocksize = 1024*1024;

			libmaus::parallel::SimpleThreadPool STP(numlogcpus);			
			GenericInputControl GIC(STP,in,blocksize,4);
			GIC.addPending();
			
			GIC.waitDecompressionFinished();
			std::cerr << "fini." << std::endl;

			STP.terminate();
			STP.join();
		}
		
		return 0;
	
		libmaus::util::ArgInfo const arginfo(argc,argv);
		int const fmapperm = arginfo.getValue<int>("mapperm",0);
		typedef libmaus::bambam::parallel::FragmentAlignmentBufferPosComparator order_type;
		// typedef libmaus::bambam::parallel::FragmentAlignmentBufferNameComparator order_type;
		
		if ( fmapperm )
		{
			mapperm<order_type>(argc,argv);
		}
		else
		{
			uint64_t const numlogcpus = arginfo.getValue<int>("threads",libmaus::parallel::NumCpus::getNumLogicalProcessors());
			libmaus::parallel::SimpleThreadPool STP(numlogcpus);
			libmaus::aio::PosixFdInputStream in(STDIN_FILENO);
			std::string const tmpfilebase = arginfo.getUnparsedValue("tmpfile",arginfo.getDefaultTmpFileName());
			std::string const alfn = tmpfilebase + ".algn";
			libmaus::aio::PosixFdOutputStream::unique_ptr_type out(new libmaus::aio::PosixFdOutputStream(alfn));
			int const templevel = arginfo.getValue<int>("level",1);
			libmaus::bambam::parallel::BlockSortControl<order_type> VC(STP,in,templevel,*out,tmpfilebase);
			VC.checkEnqueReadPackage();
			VC.waitDecodingFinished();
			VC.flushReadEndsLists();
			out->flush();
			out.reset();
			// system("ls -lrt 1>&2");
			STP.terminate();
			STP.join();
			
			std::cerr << "blocksParsed=" << VC.blocksParsed << std::endl;
			std::cerr << "maxleftoff=" << VC.maxleftoff << std::endl;
			std::cerr << "maxrightoff=" << VC.maxrightoff << std::endl;
			
			libmaus::aio::PosixFdInputStream PFIS(alfn);
			libmaus::bambam::BamHeaderLowMem const & loheader = *(VC.parseInfo.Pheader);
			std::pair<char const *, char const *> const headertext = loheader.getText();
			libmaus::bambam::BamHeader const header(std::string(headertext.first,headertext.second));
			::libmaus::bambam::BamHeader::unique_ptr_type uphead(libmaus::bambam::BamHeaderUpdate::updateHeader(arginfo,header,"testparallelbamblocksort",PACKAGE_VERSION));
			uphead->changeSortOrder("coordinate");
			libmaus::bitio::BitVector const & dupbit = VC.getDupBitVector();
			                                                                                                                                                                                
			libmaus::bambam::BamFormatAuxiliary formaux;
			
			libmaus::autoarray::AutoArray<libmaus::aio::PosixFdInputStream::unique_ptr_type> Ain(VC.blockAlgnCnt.size());
			libmaus::autoarray::AutoArray<libmaus::lz::BgzfInflateStream::unique_ptr_type> Bin(VC.blockAlgnCnt.size());
			::libmaus::autoarray::AutoArray< ::libmaus::bambam::BamAlignment > algns(VC.blockAlgnCnt.size());
			libmaus::bambam::BamAlignmentPosComparator const BAPC(0);
			::libmaus::bambam::BamAlignmentHeapComparator<libmaus::bambam::BamAlignmentPosComparator> heapcmp(BAPC,algns.begin());
			std::vector<uint64_t> cnt = VC.blockAlgnCnt;
			::std::priority_queue< uint64_t, std::vector<uint64_t>, ::libmaus::bambam::BamAlignmentHeapComparator<libmaus::bambam::BamAlignmentPosComparator> > Q(heapcmp);
			
			// open files and set up heap
			for ( uint64_t i = 0; i < VC.blockAlgnCnt.size(); ++i )
			{
				libmaus::aio::PosixFdInputStream::unique_ptr_type tptr(new libmaus::aio::PosixFdInputStream(alfn));
				Ain[i] = UNIQUE_PTR_MOVE(tptr);
				Ain[i]->clear();
				Ain[i]->seekg(VC.blockStarts[i]);
				libmaus::lz::BgzfInflateStream::unique_ptr_type bptr(new libmaus::lz::BgzfInflateStream(*Ain[i]));
				Bin[i] = UNIQUE_PTR_MOVE(bptr);
				
				if ( cnt[i]-- )
				{
					::libmaus::bambam::BamDecoder::readAlignmentGz(*Bin[i],algns[i],0 /* no header for validation */,false /* no validation */);
					Q.push(i);				
				}
			}
			
			uint32_t const dupflag = libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FDUP;
			uint32_t const dupmask = ~dupflag;

			// set up alignment writer
			libmaus::bambam::BamBlockWriterBase::unique_ptr_type Pwriter(libmaus::bambam::BamBlockWriterBaseFactory::construct(*uphead,arginfo));
			while ( Q.size() )
			{
				uint64_t const t = Q.top(); Q.pop();

				// mask out dup flag				
				algns[t].putFlags(algns[t].getFlags() & dupmask);

				// get rank
				int64_t const rank = algns[t].getRank();
				// mark as duplicate if in bit vector
				if ( rank >= 0 && dupbit.get(rank) )
					algns[t].putFlags(algns[t].getFlags() | dupflag);				

				// write alignment
				Pwriter->writeAlignment(algns[t]);
				
				// see if there is a next alignment for this block
				if ( cnt[t]-- )
				{
					::libmaus::bambam::BamDecoder::readAlignmentGz(*Bin[t],algns[t],0 /* bamheader */, false /* do not validate */);
					Q.push(t);
				}
			}
			Pwriter.reset();
		}
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
