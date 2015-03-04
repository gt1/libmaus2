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
#include <config.h>

#include <libmaus/aio/NamedTemporaryFileAllocator.hpp>
#include <libmaus/aio/NamedTemporaryFileTypeInfo.hpp>
#include <libmaus/aio/PosixFdInputStream.hpp>
#include <libmaus/aio/PosixFdOutputStream.hpp>

#include <libmaus/bambam/BamAlignmentHeapComparator.hpp>
#include <libmaus/bambam/BamAlignmentNameComparator.hpp>
#include <libmaus/bambam/BamAlignmentPosComparator.hpp>
#include <libmaus/bambam/BamBlockWriterBaseFactory.hpp>
#include <libmaus/bambam/BamWriter.hpp>
#include <libmaus/bambam/DuplicationMetrics.hpp>
#include <libmaus/bambam/DupMarkBase.hpp>
#include <libmaus/bambam/DupSetCallbackSharedVector.hpp>
#include <libmaus/bambam/DupSetCallbackVector.hpp>
#include <libmaus/bambam/ReadEndsBlockIndexSet.hpp>
#include <libmaus/bambam/SamInfoAllocator.hpp>
#include <libmaus/bambam/SamInfoTypeInfo.hpp>

#include <libmaus/bambam/parallel/AddDuplicationMetricsInterface.hpp>
#include <libmaus/bambam/parallel/AddWritePendingBgzfBlockInterface.hpp>
#include <libmaus/bambam/parallel/AlignmentBufferAllocator.hpp>
#include <libmaus/bambam/parallel/AlignmentBufferHeapComparator.hpp>
#include <libmaus/bambam/parallel/AlignmentBufferTypeInfo.hpp>
#include <libmaus/bambam/parallel/BgzfInflateZStreamBaseGetInterface.hpp>
#include <libmaus/bambam/parallel/BgzfInflateZStreamBaseReturnInterface.hpp>
#include <libmaus/bambam/parallel/BgzfLinearMemCompressWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/BgzfOutputBlockWrittenInterface.hpp>
#include <libmaus/bambam/parallel/BlockSortControlBase.hpp>
#include <libmaus/bambam/parallel/ControlInputInfo.hpp>
#include <libmaus/bambam/parallel/DecompressBlocksWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/DecompressBlocksWorkPackage.hpp>
#include <libmaus/bambam/parallel/DecompressBlockWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/DecompressBlockWorkPackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/DecompressedBlockAddPendingInterface.hpp>
#include <libmaus/bambam/parallel/DecompressedBlockAllocator.hpp>
#include <libmaus/bambam/parallel/DecompressedBlockReturnInterface.hpp>
#include <libmaus/bambam/parallel/DecompressedBlockTypeInfo.hpp>
#include <libmaus/bambam/parallel/DecompressedPendingObjectHeapComparator.hpp>
#include <libmaus/bambam/parallel/DecompressedPendingObject.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferAllocator.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferBaseSortWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferHeapComparator.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBuffer.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferMergeSortWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferNameComparator.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferPosComparator.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferReorderWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferReorderWorkPackage.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferRewriteFragmentCompleteInterface.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferRewriteReadEndsWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferRewriteWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferRewriteWorkPackage.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferRewriteWorkPackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferSortContext.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferSortFinishedInterface.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferTypeInfo.hpp>
#include <libmaus/bambam/parallel/FragReadEndsContainerFlushWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/FragReadEndsMergeWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/GenericInputBamParseObject.hpp>
#include <libmaus/bambam/parallel/GenericInputBamParseWorkPackageBlockParsedInterface.hpp>
#include <libmaus/bambam/parallel/GenericInputBamParseWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/GenericInputBamParseWorkPackage.hpp>
#include <libmaus/bambam/parallel/GenericInputBamParseWorkPackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/GenericInputBase.hpp>
#include <libmaus/bambam/parallel/GenericInputBgzfDecompressionWorkPackageDecompressedBlockReturnInterface.hpp>
#include <libmaus/bambam/parallel/GenericInputBgzfDecompressionWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/GenericInputBgzfDecompressionWorkPackage.hpp>
#include <libmaus/bambam/parallel/GenericInputBgzfDecompressionWorkPackageMemInputBlockReturnInterface.hpp>
#include <libmaus/bambam/parallel/GenericInputBgzfDecompressionWorkPackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/GenericInputBgzfDecompressionWorkSubBlockDecompressionFinishedInterface.hpp>
#include <libmaus/bambam/parallel/GenericInputBlockAllocator.hpp>
#include <libmaus/bambam/parallel/GenericInputBlock.hpp>
#include <libmaus/bambam/parallel/GenericInputBlock.hpp>
#include <libmaus/bambam/parallel/GenericInputBlockTypeInfo.hpp>
#include <libmaus/bambam/parallel/GenericInputControlBlockCompressionFinishedInterface.hpp>
#include <libmaus/bambam/parallel/GenericInputControlBlockCompressionWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/GenericInputControlBlockCompressionWorkPackage.hpp>
#include <libmaus/bambam/parallel/GenericInputControlBlockCompressionWorkPackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/GenericInputControlBlockWritePackageBlockWrittenInterface.hpp>
#include <libmaus/bambam/parallel/GenericInputControlBlockWritePackageDispatcher.hpp>
#include <libmaus/bambam/parallel/GenericInputControlBlockWritePackage.hpp>
#include <libmaus/bambam/parallel/GenericInputControlBlockWritePackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/GenericInputControlCompressionPendingHeapComparator.hpp>
#include <libmaus/bambam/parallel/GenericInputControlCompressionPending.hpp>
#include <libmaus/bambam/parallel/GenericInputControlGetCompressorInterface.hpp>
#include <libmaus/bambam/parallel/GenericInputControlMergeBlockFinishedInterface.hpp>
#include <libmaus/bambam/parallel/GenericInputControlMergeHeapEntry.hpp>
#include <libmaus/bambam/parallel/GenericInputControlMergeRequeue.hpp>
#include <libmaus/bambam/parallel/GenericInputControlPutCompressorInterface.hpp>
#include <libmaus/bambam/parallel/GenericInputControlReadAddPendingInterface.hpp>
#include <libmaus/bambam/parallel/GenericInputControlReadWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/GenericInputControlReadWorkPackage.hpp>
#include <libmaus/bambam/parallel/GenericInputControlReadWorkPackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/GenericInputControlReorderWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/GenericInputControlReorderWorkPackageFinishedInterface.hpp>
#include <libmaus/bambam/parallel/GenericInputControlReorderWorkPackage.hpp>
#include <libmaus/bambam/parallel/GenericInputControlReorderWorkPackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/GenericInputControlSetMergeStallSlotInterface.hpp>
#include <libmaus/bambam/parallel/GenericInputControlStreamInfo.hpp>
#include <libmaus/bambam/parallel/GenericInputControlSubBlockPendingHeapComparator.hpp>
#include <libmaus/bambam/parallel/GenericInputControlSubBlockPending.hpp>
#include <libmaus/bambam/parallel/GenericInputHeapComparator.hpp>
#include <libmaus/bambam/parallel/GenericInputMergeDecompressedBlockReturnInterface.hpp>
#include <libmaus/bambam/parallel/GenericInputMergeWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/GenericInputMergeWorkPackage.hpp>
#include <libmaus/bambam/parallel/GenericInputMergeWorkPackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/GenericInputMergeWorkRequest.hpp>
#include <libmaus/bambam/parallel/GenericInputSingleDataBamParseInfo.hpp>
#include <libmaus/bambam/parallel/GenericInputSingleData.hpp>
#include <libmaus/bambam/parallel/GetBgzfDeflateZStreamBaseInterface.hpp>
#include <libmaus/bambam/parallel/InputBlockAddPendingInterface.hpp>
#include <libmaus/bambam/parallel/InputBlockReturnInterface.hpp>
#include <libmaus/bambam/parallel/InputBlockWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/InputBlockWorkPackage.hpp>
#include <libmaus/bambam/parallel/InputBlockWorkPackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/MemInputBlock.hpp>
#include <libmaus/bambam/parallel/PairReadEndsContainerFlushWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/PairReadEndsMergeWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/ParseBlockWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/ParseBlockWorkPackage.hpp>
#include <libmaus/bambam/parallel/ParsedBlockAddPendingInterface.hpp>
#include <libmaus/bambam/parallel/ParsedBlockStallInterface.hpp>
#include <libmaus/bambam/parallel/ParseInfo.hpp>
#include <libmaus/bambam/parallel/ParsePackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/PutBgzfDeflateZStreamBaseInterface.hpp>
#include <libmaus/bambam/parallel/ReadEndsContainerAllocator.hpp>
#include <libmaus/bambam/parallel/ReadEndsContainerTypeInfo.hpp>
#include <libmaus/bambam/parallel/ReturnBgzfOutputBufferInterface.hpp>
#include <libmaus/bambam/parallel/SamParsePendingHeapComparator.hpp>
#include <libmaus/bambam/parallel/SamParseWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/SmallLinearBlockCompressionPendingObjectHeapComparator.hpp>
#include <libmaus/bambam/parallel/ValidateBlockFragmentAddPendingInterface.hpp>
#include <libmaus/bambam/parallel/ValidateBlockFragmentPackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/ValidateBlockFragmentWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/ValidateBlockFragmentWorkPackage.hpp>
#include <libmaus/bambam/parallel/ValidationFragment.hpp>
#include <libmaus/bambam/parallel/WriteBlockWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/WriteBlockWorkPackage.hpp>
#include <libmaus/bambam/parallel/WriteBlockWorkPackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/WritePendingObjectHeapComparator.hpp>
#include <libmaus/bambam/parallel/WritePendingObject.hpp>

#include <libmaus/lz/BgzfDeflate.hpp>
#include <libmaus/lz/BgzfDeflateOutputBufferBaseAllocator.hpp>
#include <libmaus/lz/BgzfDeflateOutputBufferBaseTypeInfo.hpp>
#include <libmaus/lz/BgzfDeflateZStreamBaseAllocator.hpp>
#include <libmaus/lz/BgzfDeflateZStreamBase.hpp>
#include <libmaus/lz/BgzfDeflateZStreamBaseTypeInfo.hpp>
#include <libmaus/lz/BgzfInflateZStreamBaseAllocator.hpp>
#include <libmaus/lz/BgzfInflateZStreamBaseTypeInfo.hpp>

#include <libmaus/parallel/LockedCounter.hpp>
#include <libmaus/parallel/LockedGrowingFreeList.hpp>
#include <libmaus/parallel/LockedHeap.hpp>
#include <libmaus/parallel/LockedQueue.hpp>
#include <libmaus/parallel/NumCpus.hpp>
#include <libmaus/parallel/SimpleThreadPool.hpp>
#include <libmaus/parallel/SimpleThreadPoolWorkPackageFreeList.hpp>

#include <libmaus/random/Random.hpp>

#include <libmaus/util/ArgInfo.hpp>
					
namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			template<typename _order_type>
			struct BlockSortControl :
				public BlockSortControlBase,
				public libmaus::bambam::parallel::GenericInputControlReadWorkPackageReturnInterface,
				public libmaus::bambam::parallel::GenericInputControlReadAddPendingInterface,
				public libmaus::bambam::parallel::GenericInputBgzfDecompressionWorkPackageReturnInterface,
				public libmaus::bambam::parallel::GenericInputBgzfDecompressionWorkPackageMemInputBlockReturnInterface,
				public libmaus::bambam::parallel::GenericInputBgzfDecompressionWorkPackageDecompressedBlockReturnInterface,
				public libmaus::bambam::parallel::GenericInputBgzfDecompressionWorkSubBlockDecompressionFinishedInterface,
				public DecompressedBlockAddPendingInterface,
				public DecompressBlocksWorkPackageReturnInterface,
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
				public ReadEndsContainerFreeListInterface,
				public FragReadEndsContainerFlushFinishedInterface,
				public PairReadEndsContainerFlushFinishedInterface,
				public FragReadEndsContainerFlushWorkPackageReturnInterface,
				public PairReadEndsContainerFlushWorkPackageReturnInterface,
				public FragReadEndsMergeWorkPackageReturnInterface,
				public PairReadEndsMergeWorkPackageReturnInterface,
				public FragReadEndsMergeWorkPackageFinishedInterface,
				public PairReadEndsMergeWorkPackageFinishedInterface,
				public AddDuplicationMetricsInterface,
				public SamParseWorkPackageReturnInterface,
				public SamParseDecompressedBlockFinishedInterface,
				public SamParseFragmentParsedInterface,
				public SamParseGetSamInfoInterface,
				public SamParsePutSamInfoInterface
			{
				typedef _order_type order_type;
				typedef BlockSortControl<order_type> this_type;
				typedef typename libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				
				libmaus::timing::RealTimeClock procrtc;
				
				block_sort_control_input_enum const inputType;

				libmaus::bambam::parallel::GenericInputControlStreamInfo const streaminfo;
				libmaus::bambam::parallel::GenericInputSingleDataReadBase inputreadbase;

				libmaus::parallel::LockedFreeList<
					libmaus::lz::BgzfInflateZStreamBase,
					libmaus::lz::BgzfInflateZStreamBaseAllocator,
					libmaus::lz::BgzfInflateZStreamBaseTypeInfo
				> deccont;

				libmaus::parallel::LockedFreeList<
					libmaus::bambam::SamInfo,
					libmaus::bambam::SamInfoAllocator,
					libmaus::bambam::SamInfoTypeInfo>::unique_ptr_type samInfoFreeList;

				std::string const tempfileprefix;

				libmaus::parallel::LockedBool decodingFinished;
				
				libmaus::parallel::SimpleThreadPool & STP;

				uint64_t const GICRPDid;
				libmaus::bambam::parallel::GenericInputControlReadWorkPackageDispatcher GICRPD;	
				uint64_t const GIBDWPDid;
				libmaus::bambam::parallel::GenericInputBgzfDecompressionWorkPackageDispatcher GIBDWPD;
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
				SamParseWorkPackageDispatcher SPWPD;
				uint64_t const SPWPDid;

				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<libmaus::bambam::parallel::GenericInputControlReadWorkPackage> genericInputReadWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<libmaus::bambam::parallel::GenericInputBgzfDecompressionWorkPackage> genericInputDecompressWorkPackages;
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
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<SamParseWorkPackage> samParseWorkPackages;

				// blocks pending to be decompressed
				libmaus::parallel::LockedQueue<ControlInputInfo::input_block_type::shared_ptr_type> decompressPendingQueue;
				
				libmaus::parallel::SynchronousCounter<uint64_t> inputBlockReturnCount;
				
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
				// number of unfinished compression sub blocks per block (signed key because header is in block -1)
				std::map< int64_t, uint64_t> compressionUnfinished;
				std::priority_queue<SmallLinearBlockCompressionPendingObject,std::vector<SmallLinearBlockCompressionPendingObject>,
					SmallLinearBlockCompressionPendingObjectHeapComparator> compressionUnqueuedPending;

				libmaus::parallel::SynchronousCounter<uint64_t> tempFileCounter;
				typedef libmaus::aio::PosixFdOutputStream temp_file_stream_type;
				typedef libmaus::aio::NamedTemporaryFile<temp_file_stream_type> temp_file_type;
				typedef libmaus::aio::NamedTemporaryFileAllocator<temp_file_stream_type> temp_file_allocator_type;
				typedef libmaus::parallel::LockedGrowingFreeList<
					libmaus::aio::NamedTemporaryFile<libmaus::aio::PosixFdOutputStream>,
					libmaus::aio::NamedTemporaryFileAllocator<libmaus::aio::PosixFdOutputStream>,
					libmaus::aio::NamedTemporaryFileTypeInfo<libmaus::aio::PosixFdOutputStream>
				> temp_file_free_list_type;
				
				temp_file_allocator_type tempFileAllocator;
				temp_file_free_list_type::unique_ptr_type tempFileFreeList;
				std::vector<temp_file_type::shared_ptr_type> tempFileVector;
				std::vector<uint64_t> tempFileUseCount;
				std::priority_queue<
					std::pair<uint64_t,uint64_t>,
					std::vector< std::pair<uint64_t,uint64_t> >,
					std::greater< std::pair<uint64_t,uint64_t> > >
					tempFileHeap;
				libmaus::parallel::PosixSpinLock tempFileLock;

				libmaus::parallel::PosixSpinLock writePendingCountLock;
				std::map<int64_t,uint64_t> writePendingCount;
				libmaus::parallel::PosixSpinLock writeNextLock;				
				std::pair<int64_t,uint64_t> writeNext;
				
				std::priority_queue<WritePendingObject,std::vector<WritePendingObject>,WritePendingObjectHeapComparator> writePendingQueue;
				libmaus::parallel::PosixSpinLock writePendingQueueLock;
				
				// bytes written per output stream
				std::vector<uint64_t> streamBytesWritten;
				// lock for streamBytesWritten
				libmaus::parallel::PosixSpinLock streamBytesWrittenLock;
				std::map< int64_t, uint64_t> blockStreamIds;
				std::vector<uint64_t> blockStarts;
				std::vector<uint64_t> blockEnds;
				libmaus::parallel::PosixSpinLock blockStartsLock;
				
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

				std::map<uint64_t,libmaus::bambam::DuplicationMetrics> metrics;
				libmaus::parallel::PosixSpinLock metricslock;
				libmaus::bitio::BitVector::unique_ptr_type Pdupbitvec;

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

				std::map<uint64_t,uint64_t> rewriteUnfinished;
				std::map<uint64_t,FragmentAlignmentBuffer::shared_ptr_type> rewriteActiveIn;
				std::map<uint64_t,FragmentAlignmentBuffer::shared_ptr_type> rewriteActiveOut;

				std::map<uint64_t, typename FragmentAlignmentBufferSortContext<order_type>::shared_ptr_type > sortContextsActive;
				libmaus::parallel::PosixSpinLock sortContextsActiveLock;

				BlockSortControl(
					block_sort_control_input_enum const rinputType,
					libmaus::parallel::SimpleThreadPool & rSTP,
					std::istream & in,
					int const level,
					std::string const & rtempfileprefix
				)
				: 
					inputType(rinputType),
					streaminfo("-",false/*finite*/,0/*start*/,0/*end*/,true/*hasheader*/),
					inputreadbase(in,streaminfo,0/*stream id*/,8*1024*1024/*blocksize*/,8/*numblocks*/,256/*complistsize*/),
					deccont(rSTP.getNumThreads()),
					tempfileprefix(rtempfileprefix),
					decodingFinished(false),
					STP(rSTP), 
					GICRPDid(STP.getNextDispatcherId()), GICRPD(*this,*this,(inputType == block_sort_control_input_bam) ? GenericInputControlReadWorkPackageDispatcher::parse_bam : GenericInputControlReadWorkPackageDispatcher::parse_sam ),
					GIBDWPDid(STP.getNextDispatcherId()), GIBDWPD(*this,*this,*this,*this,deccont),
					PBWPD(*this,*this,*this,*this,*this),
					PBWPDid(STP.getNextDispatcherId()),
					VBFWPD(*this,*this),
					VBFWPDid(STP.getNextDispatcherId()),
					BLMCWPD(*this,*this,*this,*this,*this),
					BLMCWPDid(STP.getNextDispatcherId()),
					WBWPD(*this,*this,*this),
					WBWPDid(STP.getNextDispatcherId()),
					FABRWPD(*this,*this,*this,*this),
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
					SPWPD(*this,*this,*this,*this,*this),
					SPWPDid(STP.getNextDispatcherId()),
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
					tempFileCounter(0),
					tempFileAllocator(tempfileprefix + "_algn_frag",&tempFileCounter),
					tempFileFreeList(new temp_file_free_list_type(tempFileAllocator)),
					writeNext(-1, 0),
					lastParseBlockWritten(false),
					fragmentBufferFreeListPreSort(STP.getNumThreads(),FragmentAlignmentBufferAllocator(STP.getNumThreads(), 2 /* pointer multiplier */)),
					fragmentBufferFreeListPostSort(STP.getNumThreads(),FragmentAlignmentBufferAllocator(STP.getNumThreads(), 1 /* pointer multiplier */)),
					postSortNext(0),
					readEndsFragContainerAllocator(getReadEndsContainerSize(),tempfileprefix+"_readends_frag_"),
					readEndsPairContainerAllocator(getReadEndsContainerSize(),tempfileprefix+"_readends_pair_"),
					readEndsFragContainerFreeList(readEndsFragContainerAllocator),
					readEndsPairContainerFreeList(readEndsPairContainerAllocator),
					unflushedFragReadEndsContainers(0),
					unflushedPairReadEndsContainers(0),
					unmergeFragReadEndsRegions(0),
					unmergePairReadEndsRegions(0)
				{
					procrtc.start();
					
					STP.registerDispatcher(GICRPDid,&GICRPD);
					STP.registerDispatcher(GIBDWPDid,&GIBDWPD);
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
					STP.registerDispatcher(SPWPDid,&SPWPD);
					
					setupTempFiles(STP.getNumThreads());
				}

				// SamParseGetSamInfoInterface
				libmaus::bambam::SamInfo::shared_ptr_type samParseGetSamInfo()
				{
					return samInfoFreeList->get();
				}

				// SamParsePutSamInfoInterface
				void samParsePutSamInfo(libmaus::bambam::SamInfo::shared_ptr_type ptr)
				{
					samInfoFreeList->put(ptr);
				}

				void genericInputControlReadWorkPackageReturn(libmaus::bambam::parallel::GenericInputControlReadWorkPackage * package)
				{
					genericInputReadWorkPackages.returnPackage(package);
				}

				void genericInputBgzfDecompressionWorkPackageReturn(libmaus::bambam::parallel::GenericInputBgzfDecompressionWorkPackage * package)
				{
					genericInputDecompressWorkPackages.returnPackage(package);
				}

				void samParseWorkPackageReturn(SamParseWorkPackage * package)
				{
					samParseWorkPackages.returnPackage(package);
				}

				void checkDecompressionBlockPending(uint64_t const streamid)
				{
					assert ( streamid == 0 );
					libmaus::parallel::ScopePosixSpinLock slock(inputreadbase.lock);

					libmaus::bambam::parallel::MemInputBlock::shared_ptr_type mib;
					libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type db;
					
					while (
						inputreadbase.decompressionpending.size() &&
						inputreadbase.decompressionpending.top().block->meta.blockid == inputreadbase.decompressionpendingnext.first &&
						inputreadbase.decompressionpending.top().subblockid == inputreadbase.decompressionpendingnext.second &&
						(mib = inputreadbase.meminputblockfreelist.getIf()) &&
						(db = inputreadbase.decompressedblockfreelist.getIf())
					)
					{
						// input block id
						uint64_t const blockid = inputreadbase.decompressionpending.top().block->meta.blockid;
						
						libmaus::bambam::parallel::GenericInputControlSubBlockPending pend = inputreadbase.decompressionpending.top();
						inputreadbase.decompressionpending.pop();
						// meta compressed block
						pend.mib = mib;
						// copy stream id
						pend.mib->streamid = streamid;
						// assign block id
						pend.mib->blockid = inputreadbase.decompressedBlockIdAcc++;
						// decompressed block
						pend.db = db;
						db = libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type();
						mib = libmaus::bambam::parallel::MemInputBlock::shared_ptr_type();

						libmaus::bambam::parallel::GenericInputBgzfDecompressionWorkPackage * package = genericInputDecompressWorkPackages.getPackage();
						*package = libmaus::bambam::parallel::GenericInputBgzfDecompressionWorkPackage(0/*prio*/,GIBDWPDid,pend);
						STP.enque(package);

						inputreadbase.decompressionpendingnext.second += 1;					
						if ( inputreadbase.decompressionpendingnext.second == inputreadbase.decompressiontotal[blockid] )
						{
							inputreadbase.decompressionpendingnext.first += 1;
							inputreadbase.decompressionpendingnext.second = 0;
						}
					}

					if ( db )
						inputreadbase.decompressedblockfreelist.put(db);
					if ( mib )
						inputreadbase.meminputblockfreelist.put(mib);
				}
				
				void checkInputBlockPending(uint64_t const streamid)
				{
					assert ( streamid == 0 );
					
					if ( inputType == block_sort_control_input_bam )
					{					
						std::vector<libmaus::bambam::parallel::GenericInputBase::generic_input_shared_block_ptr_type> readylist;
						
						{
							libmaus::parallel::ScopePosixSpinLock slock(inputreadbase.lock);

							while (
								inputreadbase.pending.size() &&
								(
									inputreadbase.pending.top()->meta.blockid == 
									inputreadbase.nextblockid
								)
							)
							{
								libmaus::bambam::parallel::GenericInputBase::generic_input_shared_block_ptr_type block = inputreadbase.pending.top();
								inputreadbase.pending.pop();
								readylist.push_back(block);	
								inputreadbase.decompressiontotal.push_back(block->meta.blocks.size());
								inputreadbase.nextblockid += 1;		
							}
						}

						// enque decompression requests in queue
						for ( std::vector<libmaus::bambam::parallel::GenericInputBase::generic_input_shared_block_ptr_type>::size_type i = 0; i < readylist.size(); ++i )
						{
							// get block
							libmaus::bambam::parallel::GenericInputBase::generic_input_shared_block_ptr_type block = readylist[i];

							// mark sub blocks as pending
							for ( uint64_t j = 0; j < block->meta.blocks.size(); ++j )
							{
								libmaus::parallel::ScopePosixSpinLock slock(inputreadbase.lock);
								inputreadbase.decompressionpending.push(libmaus::bambam::parallel::GenericInputControlSubBlockPending(block,j));
							}
						}
						
						checkDecompressionBlockPending(streamid);
					}
					else if ( inputType == block_sort_control_input_sam )
					{
						std::vector<libmaus::bambam::parallel::GenericInputBase::generic_input_shared_block_ptr_type> returnList;

						{
							libmaus::parallel::ScopePosixSpinLock slock(inputreadbase.lock);

							while (
								(!inputreadbase.samHeaderComplete)
								&&
								inputreadbase.pending.size() &&
								(
									inputreadbase.pending.top()->meta.blockid == 
									inputreadbase.nextblockid
								)
							)
							{
								// get block
								libmaus::bambam::parallel::GenericInputBase::generic_input_shared_block_ptr_type block = inputreadbase.pending.top();
								inputreadbase.pending.pop();
								
								assert ( block->meta.blocks.size() );
								assert ( block->meta.blocks.size() == 1 );
								
								std::pair<uint8_t *,uint8_t *> & P = block->meta.blocks[0];
								
								assert ( P.second != P.first );								
								assert ( *(P.second-1) == '\n' );
								
								uint8_t * Pfirst = P.first;
								while ( Pfirst != P.second )
								{
									if ( *Pfirst != '@' )
										break;
									
									while ( *Pfirst != '\n' )
										++Pfirst;
									assert ( *Pfirst == '\n' );
									
									++Pfirst;
								}
								
								// add data
								if ( Pfirst - P.first )
									inputreadbase.samHeaderAdd(reinterpret_cast<char const *>(P.first),Pfirst-P.first);
								
								// skip header data
								P.first = Pfirst;
								
								// header complete?
								if ( P.first != P.second || block->meta.eof )
								{
									parseInfo.setHeaderFromText(
										inputreadbase.samHeader.begin(),
										inputreadbase.samHeader.size()
									);


									libmaus::parallel::LockedFreeList<
										libmaus::bambam::SamInfo,
										libmaus::bambam::SamInfoAllocator,
										libmaus::bambam::SamInfoTypeInfo>::unique_ptr_type tsamInfoFreeList(
										new libmaus::parallel::LockedFreeList<
										libmaus::bambam::SamInfo,
										libmaus::bambam::SamInfoAllocator,
										libmaus::bambam::SamInfoTypeInfo>
										(
											STP.getNumThreads(),
											libmaus::bambam::SamInfoAllocator(parseInfo.Pheader.get())	
										)
									);
									samInfoFreeList = UNIQUE_PTR_MOVE(tsamInfoFreeList);
									
									bamHeaderComplete(parseInfo.BHPS);

									inputreadbase.samHeaderComplete = true;
								}
								
								// if block has been fully processed then return it
								if ( P.first == P.second )
								{
									returnList.push_back(block);
									inputreadbase.nextblockid += 1;	
								}
								// block is not fully processed, header parsing is complete
								else
								{
									assert ( inputreadbase.samHeaderComplete );
									inputreadbase.pending.push(block);
								}
							}
						}

						std::vector<libmaus::bambam::parallel::GenericInputBase::generic_input_shared_block_ptr_type> readyList;

						{
							libmaus::parallel::ScopePosixSpinLock slock(inputreadbase.lock);

							while (
								(inputreadbase.samHeaderComplete)
								&&
								inputreadbase.pending.size() &&
								(
									inputreadbase.pending.top()->meta.blockid == 
									inputreadbase.nextblockid
								)
							)
							{
								// get block
								libmaus::bambam::parallel::GenericInputBase::generic_input_shared_block_ptr_type block = inputreadbase.pending.top();
								inputreadbase.pending.pop();
								
								assert ( block->meta.blocks.size() );
								assert ( block->meta.blocks.size() == 1 );
								
								std::pair<uint8_t *,uint8_t *> P = block->meta.blocks[0];
								block->meta.blocks.pop_back();
								
								assert ( P.second != P.first );								
								assert ( *(P.second-1) == '\n' );
								
								// std::cerr << "---\n" << std::string(P.first,P.second);
								
								while ( P.first != P.second )
								{
									ptrdiff_t const r = (P.second-P.first)-1;
									ptrdiff_t const maxblock = getSamMaxParseBlockSize();
									ptrdiff_t const est = std::min(r,maxblock);
									uint8_t * pp = P.first + est;
									while ( *pp != '\n' )
										++pp;
									assert ( *pp == '\n' );
									pp++;

									block->meta.blocks.push_back(std::pair<uint8_t *,uint8_t *>(P.first,pp));
									
									P.first = pp;
								}
								
								{
									libmaus::parallel::ScopePosixSpinLock slock(inputreadbase.samParsePendingQueueLock);
									for ( uint64_t i = 0; i < block->meta.blocks.size(); ++i )
										inputreadbase.samParsePendingQueue.push_back
											(SamParsePending(block,i,inputreadbase.samParsePendingQueueNextAbsId++));
								}
																
								readyList.push_back(block);
								inputreadbase.nextblockid += 1;
							}
						}

						for ( uint64_t i = 0; i < returnList.size(); ++i )
							enqueReadBlock(returnList[i]);
							
						checkSamParsePendingQueue(streamid);
					}
				}
				
				void checkSamParsePendingQueue(uint64_t const streamid)
				{
					assert ( streamid == 0 );
					
					{
						libmaus::parallel::ScopePosixSpinLock slock(inputreadbase.samParsePendingQueueLock);
						libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type deblock;

						while ( 
							inputreadbase.samParsePendingQueue.size() 
							&&
							(deblock = inputreadbase.decompressedblockfreelist.getIf())
						)
						{
							SamParsePending SPP = inputreadbase.samParsePendingQueue.front();
							inputreadbase.samParsePendingQueue.pop_front();

							SamParseWorkPackage * package = samParseWorkPackages.getPackage();
							*package = SamParseWorkPackage(							
								0 /* prio */,
								SPWPDid,
								streamid,
								SPP,
								deblock
							);
							STP.enque(package);
						}
					}
				}

				void samParseDecompressedBlockFinished(uint64_t const streamid, libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type ptr)
				{
					assert ( streamid == 0 );
					
					{
						libmaus::parallel::ScopePosixSpinLock slock(parsePendingLock);
						parsePending.push(DecompressedPendingObject(ptr->blockid,ptr));

						#if 1
						if ( ptr->final )
							std::cerr << "stream fully decoded" << std::endl;			
						#endif
					}
					
					checkParsePendingList();
				}

				// input block fragment parsed from SAM to BAM records
				void samParseFragmentParsed(SamParsePending SPP)
				{
					if ( SPP.block->meta.returnBlock() )
						enqueReadBlock(SPP.block);
				}

				void genericInputControlReadAddPending(libmaus::bambam::parallel::GenericInputBase::generic_input_shared_block_ptr_type block)
				{
					uint64_t const streamid = block->meta.streamid;
					assert ( streamid == 0 );
					
					// check whether input is BAM or SAM
					{
						libmaus::parallel::ScopePosixSpinLock slock(inputreadbase.lock);
						inputreadbase.pending.push(block);
					}
					
					checkInputBlockPending(streamid);
				}

				void genericInputBgzfDecompressionWorkPackageMemInputBlockReturn(uint64_t streamid, libmaus::bambam::parallel::MemInputBlock::shared_ptr_type ptr)
				{
					assert ( streamid == 0 );
					inputreadbase.meminputblockfreelist.put(ptr);
					checkDecompressionBlockPending(streamid);
				}

				// bgzf block decompressed
				void genericInputBgzfDecompressionWorkPackageDecompressedBlockReturn(uint64_t streamid, libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type ptr)
				{
					assert ( streamid == 0 );
					
					{
						libmaus::parallel::ScopePosixSpinLock slock(inputreadbase.lock);
						inputreadbase.decompressedBlocksAcc += 1;
					}
					
					{
						libmaus::parallel::ScopePosixSpinLock slock(parsePendingLock);
						parsePending.push(DecompressedPendingObject(ptr->blockid,ptr));

						#if 1
						if ( ptr->final )
							std::cerr << "stream fully decompressed" << std::endl;			
						#endif
					}
					
					checkParsePendingList();
				}
				
				void enqueReadBlock(libmaus::bambam::parallel::GenericInputBase::generic_input_shared_block_ptr_type block)
				{
						inputreadbase.blockFreeList.put(block);

						bool const eof = inputreadbase.getEOF();
					
						// not yet eof? try to read on	
						if ( ! eof )
						{
							libmaus::bambam::parallel::GenericInputControlReadWorkPackage * package = genericInputReadWorkPackages.getPackage();
							*package = libmaus::bambam::parallel::GenericInputControlReadWorkPackage(
								0 /* prio */, GICRPDid, &inputreadbase);
							STP.enque(package);
						}				
				}

				void genericInputBgzfDecompressionWorkSubBlockDecompressionFinished(
					libmaus::bambam::parallel::GenericInputBase::generic_input_shared_block_ptr_type block, uint64_t /* subblockid */
				)
				{
					uint64_t const streamid = block->meta.streamid;
					assert ( streamid == 0 );

					// check whether this block is completely decompressed now		
					if ( block->meta.returnBlock() )
						enqueReadBlock(block);
				}
				
				std::vector<libmaus::bambam::parallel::GenericInputControlStreamInfo> getBlockInfo()
				{
					flushTempFiles();
					
					std::vector<libmaus::bambam::parallel::GenericInputControlStreamInfo> V;
					
					for ( uint64_t i = 0; i < blockStarts.size(); ++i )
					{
						V.push_back(
							libmaus::bambam::parallel::GenericInputControlStreamInfo
							(
								tempFileVector[blockStreamIds[i]]->name,
								true /* finite */,
								blockStarts[i],
								blockEnds[i],
								false /* has header */
							)
						);
					}
					
					tempFileVector.resize(0);
					tempFileFreeList.reset();
					
					return V;
				}
				
				void flushTempFiles()
				{
					for ( uint64_t i = 0; i < tempFileVector.size(); ++i )
						if ( tempFileVector[i] )
							tempFileVector[i]->stream.flush();
				}
				
				void addTempFile()
				{
					libmaus::aio::NamedTemporaryFile<libmaus::aio::PosixFdOutputStream>::shared_ptr_type ptr = tempFileFreeList->get();
					uint64_t const id = ptr->id;
					libmaus::util::TempFileRemovalContainer::addTempFile(ptr->name);
					
					while ( ! (id < tempFileVector.size()) )
						tempFileVector.push_back(temp_file_type::shared_ptr_type());
					while ( ! (id < tempFileUseCount.size()) )
						tempFileUseCount.push_back(0);
						
					tempFileVector.at(id) = ptr;
					tempFileUseCount.at(id) = 0;
					
					tempFileHeap.push(std::pair<uint64_t,uint64_t>(tempFileUseCount[id],id));					
				}

				libmaus::aio::NamedTemporaryFile<libmaus::aio::PosixFdOutputStream>::shared_ptr_type getTempFile()
				{
					libmaus::parallel::ScopePosixSpinLock slock(tempFileLock);
					
					if ( !tempFileHeap.size() )
						addTempFile();
						
					assert ( tempFileHeap.size() );

					std::pair<uint64_t,uint64_t> P = tempFileHeap.top();
					tempFileHeap.pop();
					uint64_t const id = P.second;
					tempFileUseCount[id] += 1;
					
					libmaus::aio::NamedTemporaryFile<libmaus::aio::PosixFdOutputStream>::shared_ptr_type ptr = tempFileVector.at(id);
					
					return ptr;
				}
				
				void putTempFile(uint64_t const id)
				{
					libmaus::parallel::ScopePosixSpinLock slock(tempFileLock);
					tempFileHeap.push(std::pair<uint64_t,uint64_t>(tempFileUseCount[id],id));
				}
				
				void setupTempFiles(uint64_t const numtempfiles)
				{
					for ( uint64_t i = 0; i < numtempfiles; ++i )
						addTempFile();
				}

				static uint64_t getParseBufferSize()
				{
					return (1ull<<28);
				}
				
				static uint64_t getReadEndsContainerSize()
				{
					return 16*1024*1024;
				}

				void fragReadEndsContainerFlushFinished(libmaus::bambam::ReadEndsContainer::shared_ptr_type REC)
				{
					assert ( REC.get() );
					readEndsFragContainerFreeList.put(REC);
					unflushedFragReadEndsContainers -= 1;
				}
				
				void pairReadEndsContainerFlushFinished(libmaus::bambam::ReadEndsContainer::shared_ptr_type REC)
				{
					assert ( REC.get() );
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
				
				libmaus::bitio::BitVector const & getDupBitVector()
				{
					return *Pdupbitvec;
				}
				
				libmaus::bitio::BitVector::unique_ptr_type releaseDupBitVector()
				{
					return UNIQUE_PTR_MOVE(Pdupbitvec);
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

				static uint64_t getSamMaxParseBlockSize()
				{
					return 256*1024;
				}


				void enqueReadPackage()
				{
					libmaus::bambam::parallel::GenericInputControlReadWorkPackage * package = genericInputReadWorkPackages.getPackage();
					*package = libmaus::bambam::parallel::GenericInputControlReadWorkPackage(0 /* prio */, GICRPDid, &inputreadbase);
					STP.enque(package);
				}
				
				void waitDecodingFinished()
				{
					while ( ( ! decodingFinished.get() ) && (!STP.isInPanicMode()) )
					{
						sleep(1);
						// STP.printStateHistogram(std::cerr);
					}
				}
				
				libmaus::autoarray::AutoArray<char> getSerialisedBamHeader()
				{
					libmaus::autoarray::AutoArray<char> S = parseInfo.BHPS.getSerialised();
					return S;
				}
				
				libmaus::bambam::BamHeader::unique_ptr_type getHeader()
				{
					libmaus::bambam::BamHeader::unique_ptr_type tptr(parseInfo.getHeader());
					return UNIQUE_PTR_MOVE(tptr);
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
						compressionUnqueuedPending.push(SmallLinearBlockCompressionPendingObject(-1 /* block id -1 for header, ahead of data */,i,plow,phigh));
					}
				}

				// get bgzf compressor object
				libmaus::lz::BgzfDeflateZStreamBase::shared_ptr_type getBgzfDeflateZStreamBase()
				{
					libmaus::lz::BgzfDeflateZStreamBase::shared_ptr_type tptr(bgzfDeflateZStreamBaseFreeList.get());
					return tptr;
				}

				// return bgzf compressor object
				void putBgzfDeflateZStreamBase(libmaus::lz::BgzfDeflateZStreamBase::shared_ptr_type & ptr)
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
					// return block
					inputreadbase.decompressedblockfreelist.put(block);
					
					if ( inputType == block_sort_control_input_bam )
					{
						// check for pending operations
						checkDecompressionBlockPending(0 /*stream id */);
					}
					else if ( inputType == block_sort_control_input_sam )
					{
						// check for sam parsing
						checkSamParsePendingQueue(0 /* stream id */);
					}

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
							std::cerr << "[V] " << readsParsed << "\t" << procrtc.formatTime(procrtc.getElapsedSeconds()) << "\t" << libmaus::util::MemUsage() << (algn->final?"\tfinal":"") << std::endl;
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
				
				void returnBgzfOutputBufferInterface(libmaus::lz::BgzfDeflateOutputBufferBase::shared_ptr_type & obuf)
				{
					bgzfDeflateOutputBufferFreeList.put(obuf);
					checkSmallBlockCompressionPending();				
				}

				void bgzfOutputBlockWritten(uint64_t const streamid, int64_t const blockid, uint64_t const subid, uint64_t const n)
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
						if ( blockid >= 0 )
						{
							libmaus::parallel::ScopePosixSpinLock sblock(blockStartsLock);
							while ( ! (static_cast<uint64_t>(blockid) < blockEnds.size()) )
								blockEnds.push_back(0);
							blockEnds[blockid] = streamBytesWritten[streamid] + n;
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
							{
								// std::cerr << "putting back stream " << streamid << " for block " << blockid << std::endl;
								{
									libmaus::parallel::ScopePosixSpinLock sblock(blockStartsLock);
									assert ( blockStreamIds.find(blockid) != blockStreamIds.end() );
									assert ( blockStreamIds.find(blockid)->second == streamid );
								
								}
								
								// return temporary file
								putTempFile(streamid);
							}
						
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
									
									#if 0
									// add EOF
									libmaus::lz::BgzfDeflate<std::ostream> defl(outref);
									defl.addEOFBlock();
									#endif
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
						
						libmaus::aio::NamedTemporaryFile<libmaus::aio::PosixFdOutputStream>::shared_ptr_type tmpptr;
						
						{
							libmaus::parallel::ScopePosixSpinLock sblock(blockStartsLock);
							if ( blockStreamIds.find(blockid) == blockStreamIds.end() )
							{
								tmpptr = getTempFile();
								blockStreamIds[blockid] = tmpptr->id;
							}
							else
							{
								libmaus::parallel::ScopePosixSpinLock slock(tempFileLock);
								tmpptr = tempFileVector[blockStreamIds.find(blockid)->second];
							}
						}
						
						#if 0
						if ( subid == 0 )
						{
							std::cerr << "got stream id " << tmpptr->id << " for block " << blockid << std::endl;
						}
						#endif

						writePendingQueue.push(
							WritePendingObject(tmpptr->id /* stream id */,&(tmpptr->stream),blockid,subid,obuf,flushinfo)
						);
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
			libmaus::bambam::parallel::BlockSortControl<order_type> VC(
				libmaus::bambam::parallel::BlockSortControlBase::block_sort_control_input_bam,STP,in,0 /* comp */,tempfileprefix);
			VC.enqueReadPackage();
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

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			struct GenericInputControl : 
				public libmaus::bambam::parallel::GenericInputControlReadWorkPackageReturnInterface,
				public libmaus::bambam::parallel::GenericInputControlReadAddPendingInterface,
				public libmaus::bambam::parallel::GenericInputBgzfDecompressionWorkPackageReturnInterface,
				public libmaus::bambam::parallel::GenericInputBgzfDecompressionWorkPackageMemInputBlockReturnInterface,
				public libmaus::bambam::parallel::GenericInputBgzfDecompressionWorkPackageDecompressedBlockReturnInterface,
				public libmaus::bambam::parallel::GenericInputBgzfDecompressionWorkSubBlockDecompressionFinishedInterface,
				public libmaus::bambam::parallel::GenericInputBamParseWorkPackageReturnInterface,
				public libmaus::bambam::parallel::GenericInputBamParseWorkPackageBlockParsedInterface,
				public libmaus::bambam::parallel::GenericInputMergeWorkPackageReturnInterface,
				public libmaus::bambam::parallel::GenericInputMergeDecompressedBlockReturnInterface,
				public libmaus::bambam::parallel::GenericInputControlSetMergeStallSlotInterface,
				public libmaus::bambam::parallel::GenericInputControlMergeBlockFinishedInterface,
				public libmaus::bambam::parallel::GenericInputControlMergeRequeue,
				public libmaus::bambam::parallel::GenericInputControlReorderWorkPackageReturnInterface,
				public libmaus::bambam::parallel::GenericInputControlReorderWorkPackageFinishedInterface,
				public libmaus::bambam::parallel::GenericInputControlBlockCompressionFinishedInterface,
				public libmaus::bambam::parallel::GenericInputControlBlockCompressionWorkPackageReturnInterface,
				public libmaus::bambam::parallel::GenericInputControlGetCompressorInterface,
				public libmaus::bambam::parallel::GenericInputControlPutCompressorInterface,
				public libmaus::bambam::parallel::GenericInputControlBlockWritePackageReturnInterface,
				public libmaus::bambam::parallel::GenericInputControlBlockWritePackageBlockWrittenInterface
			{
				typedef GenericInputControl this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

				libmaus::parallel::SimpleThreadPool & STP;
				
				std::ostream & out;
				
				libmaus::autoarray::AutoArray<char> & sheader;

				libmaus::bitio::BitVector & BV;
				
				libmaus::autoarray::AutoArray<libmaus::bambam::parallel::GenericInputSingleData::unique_ptr_type> data;

				libmaus::parallel::LockedFreeList<
					libmaus::lz::BgzfInflateZStreamBase,
					libmaus::lz::BgzfInflateZStreamBaseAllocator,
					libmaus::lz::BgzfInflateZStreamBaseTypeInfo
				> deccont;

				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<libmaus::bambam::parallel::GenericInputControlReadWorkPackage> readWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<libmaus::bambam::parallel::GenericInputBgzfDecompressionWorkPackage> decompressWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<libmaus::bambam::parallel::GenericInputBamParseWorkPackage> parseWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<libmaus::bambam::parallel::GenericInputMergeWorkPackage> mergeWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<libmaus::bambam::parallel::GenericInputControlReorderWorkPackage> rewriteWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<libmaus::bambam::parallel::GenericInputControlBlockCompressionWorkPackage> compressWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<libmaus::bambam::parallel::GenericInputControlBlockWritePackage> writeWorkPackages;

				uint64_t const GICRPDid;
				libmaus::bambam::parallel::GenericInputControlReadWorkPackageDispatcher GICRPD;
				
				uint64_t const GIBDWPDid;
				libmaus::bambam::parallel::GenericInputBgzfDecompressionWorkPackageDispatcher GIBDWPD;
				
				uint64_t const GIBPWPDid;
				libmaus::bambam::parallel::GenericInputBamParseWorkPackageDispatcher GIBPWPD;
				
				uint64_t const GIMWPDid;
				libmaus::bambam::parallel::GenericInputMergeWorkPackageDispatcher GIMWPD;
				
				uint64_t const GICRWPDid;
				libmaus::bambam::parallel::GenericInputControlReorderWorkPackageDispatcher GICRWPD;
				
				uint64_t const GICBCWPDid;
				libmaus::bambam::parallel::GenericInputControlBlockCompressionWorkPackageDispatcher GICBCWPD;
				
				uint64_t const GICBWPDid;
				libmaus::bambam::parallel::GenericInputControlBlockWritePackageDispatcher GICBWPD;
				
				uint64_t volatile activedecompressionstreams;
				libmaus::parallel::PosixSpinLock activedecompressionstreamslock;
				
				uint64_t volatile streamParseUnstarted;
				libmaus::parallel::PosixSpinLock streamParseUnstartedLock;

				libmaus::util::FiniteSizeHeap<libmaus::bambam::parallel::GenericInputControlMergeHeapEntry> mergeheap;
				libmaus::parallel::PosixSpinLock mergeheaplock;
				
				uint64_t const alignbuffersize;
				uint64_t const numalgnbuffers;
				
				libmaus::parallel::LockedFreeList<
					libmaus::bambam::parallel::AlignmentBuffer,
					libmaus::bambam::parallel::AlignmentBufferAllocator,
					libmaus::bambam::parallel::AlignmentBufferTypeInfo
				> mergebufferfreelist;
				libmaus::bambam::parallel::AlignmentBuffer::shared_ptr_type mergestallslot;
				libmaus::parallel::PosixSpinLock mergestallslotlock;
				
				std::deque<libmaus::bambam::parallel::GenericInputMergeWorkRequest> mergeworkrequests;
				libmaus::parallel::PosixSpinLock mergeworkrequestslock;
				
				uint64_t volatile mergedReads;
				uint64_t volatile mergedReadsLastPrint;
				libmaus::parallel::PosixSpinLock mergedReadsLock;
				
				uint64_t volatile nextMergeBufferId;
				libmaus::parallel::PosixSpinLock nextMergeBufferIdLock;
				
				bool volatile mergingFinished;
				libmaus::parallel::PosixSpinLock mergingFinishedLock;
				
				libmaus::parallel::LockedFreeList<
					libmaus::bambam::parallel::FragmentAlignmentBuffer,
					libmaus::bambam::parallel::FragmentAlignmentBufferAllocator,
					libmaus::bambam::parallel::FragmentAlignmentBufferTypeInfo>
					rewriteBlockFreeList;

				std::deque<libmaus::bambam::parallel::AlignmentBuffer::shared_ptr_type> rewritePendingQueue;
				std::map<uint64_t,uint64_t> rewriteUnfinished;
				libmaus::parallel::PosixSpinLock rewritePendingQueueLock;
				
				std::priority_queue<
					libmaus::bambam::parallel::FragmentAlignmentBuffer::shared_ptr_type,
					std::vector<libmaus::bambam::parallel::FragmentAlignmentBuffer::shared_ptr_type>,
					libmaus::bambam::parallel::FragmentAlignmentBufferHeapComparator
				> rewriteReorderQueue;
				uint64_t volatile rewriteReorderNext;
				libmaus::parallel::PosixSpinLock rewriteReorderQueueLock;
				
				bool volatile rewriteFinished;
				libmaus::parallel::PosixSpinLock rewriteFinishedLock;

				std::deque<libmaus::bambam::parallel::GenericInputControlCompressionPending> compressionPending;
				uint64_t volatile compressionPendingAbsIdNext;
				libmaus::parallel::PosixSpinLock compressionPendingLock;

				std::map<int64_t,uint64_t> compressionUnfinished;
				libmaus::parallel::PosixSpinLock compressionUnfinishedLock;	

				std::map<int64_t,libmaus::bambam::parallel::FragmentAlignmentBuffer::shared_ptr_type> compressionActive;
				libmaus::parallel::PosixSpinLock compressionActiveLock;

				libmaus::parallel::LockedFreeList<
					libmaus::lz::BgzfDeflateOutputBufferBase,
					libmaus::lz::BgzfDeflateOutputBufferBaseAllocator,
					libmaus::lz::BgzfDeflateOutputBufferBaseTypeInfo
				> compressBufferFreeList;
				
				libmaus::parallel::LockedGrowingFreeList<
					libmaus::lz::BgzfDeflateZStreamBase,
					libmaus::lz::BgzfDeflateZStreamBaseAllocator,
					libmaus::lz::BgzfDeflateZStreamBaseTypeInfo>
					compressorFreeList;

				std::priority_queue<
					libmaus::bambam::parallel::GenericInputControlCompressionPending,
					std::vector<libmaus::bambam::parallel::GenericInputControlCompressionPending>,
					libmaus::bambam::parallel::GenericInputControlCompressionPendingHeapComparator
				> compressedBlockWriteQueue;
				uint64_t volatile compressedBlockWriteQueueNext;
				libmaus::parallel::PosixSpinLock compressedBlockWriteQueueLock;
				
				bool volatile lastBlockWritten;
				libmaus::parallel::PosixSpinLock lastBlockWrittenLock;

				void enqueHeader()
				{
					libmaus::parallel::ScopePosixSpinLock rlock(compressionPendingLock);
					
					uint64_t const maxblocksize = libmaus::lz::BgzfConstants::getBgzfMaxBlockSize();
					uint64_t const headersize = sheader.size();
					uint64_t const tnumblocks = (headersize + maxblocksize - 1)/maxblocksize;
					uint64_t const blocksize = (headersize+tnumblocks-1)/tnumblocks;
					uint64_t const numblocks = (headersize+blocksize-1)/blocksize;	

					// enque compression requests
					for ( uint64_t i = 0; i < numblocks; ++i )
					{
						uint64_t const ilow = i*blocksize;
						uint64_t const ihigh = std::min(ilow+blocksize,headersize);
						uint8_t * plow  = reinterpret_cast<uint8_t *>(sheader.begin()) + ilow;
						uint8_t * phigh = reinterpret_cast<uint8_t *>(sheader.begin()) + ihigh;

						compressionPending.push_back(
							libmaus::bambam::parallel::GenericInputControlCompressionPending(
								-1 /* block id */, 
								i  /* sub id */,
								compressionPendingAbsIdNext++ /* absolute block id */,
								false /* final */,
								std::pair<uint8_t *,uint8_t *>(plow,phigh)
							)
						);						
					}
				}

				GenericInputControl(
					libmaus::parallel::SimpleThreadPool & rSTP,
					std::ostream & rout,
					libmaus::autoarray::AutoArray<char> & rsheader,
					std::vector<libmaus::bambam::parallel::GenericInputControlStreamInfo> const & in, 
					libmaus::bitio::BitVector & rBV,
					int const level,
					uint64_t const blocksize, // read block size
					uint64_t const numblocks, // number of read blocks per channel
					uint64_t const ralgnbuffersize, // merge alignment buffer size
					uint64_t const rnumalgnbuffers, // number of merge alignment buffers
					uint64_t const complistsize
				)
				: STP(rSTP), 
				  out(rout),
				  sheader(rsheader),
				  BV(rBV),
				  data(in.size()), deccont(STP.getNumThreads()), 
				  GICRPDid(STP.getNextDispatcherId()), GICRPD(*this,*this),
				  GIBDWPDid(STP.getNextDispatcherId()), GIBDWPD(*this,*this,*this,*this,deccont),
				  GIBPWPDid(STP.getNextDispatcherId()), GIBPWPD(*this,*this),
				  GIMWPDid(STP.getNextDispatcherId()), GIMWPD(*this,*this,*this,*this,*this),
				  GICRWPDid(STP.getNextDispatcherId()), GICRWPD(*this,*this,BV),
				  GICBCWPDid(STP.getNextDispatcherId()), GICBCWPD(*this,*this,*this,*this),
				  GICBWPDid(STP.getNextDispatcherId()), GICBWPD(*this,*this),
				  activedecompressionstreams(in.size()), activedecompressionstreamslock(),
				  streamParseUnstarted(in.size()), streamParseUnstartedLock(),
				  mergeheap(in.size()), mergeheaplock(),
				  alignbuffersize(ralgnbuffersize), numalgnbuffers(rnumalgnbuffers),
				  mergebufferfreelist(numalgnbuffers,libmaus::bambam::parallel::AlignmentBufferAllocator(alignbuffersize,1 /* ptr diff */)),
				  mergedReads(0), mergedReadsLastPrint(0), mergedReadsLock(), nextMergeBufferId(0), nextMergeBufferIdLock(),
				  mergingFinished(false), mergingFinishedLock(),
				  rewriteBlockFreeList(numalgnbuffers,libmaus::bambam::parallel::FragmentAlignmentBufferAllocator(STP.getNumThreads(),1 /* pointer mult */)),
				  rewriteReorderNext(0), rewriteFinished(false), rewriteFinishedLock(),
				  compressionPending(), compressionPendingAbsIdNext(0), compressionPendingLock(),
				  compressionUnfinished(), compressionUnfinishedLock(),
				  compressionActive(), compressionActiveLock(),
				  compressBufferFreeList(256,libmaus::lz::BgzfDeflateOutputBufferBaseAllocator(level)),
				  compressorFreeList(libmaus::lz::BgzfDeflateZStreamBaseAllocator(level)),
				  compressedBlockWriteQueueNext(0),
				  lastBlockWritten(false), lastBlockWrittenLock()
				{
					for ( std::vector<std::istream *>::size_type i = 0; i < in.size(); ++i )
					{
						libmaus::bambam::parallel::GenericInputSingleData::unique_ptr_type tptr(
							new libmaus::bambam::parallel::GenericInputSingleData(in[i],i,blocksize,numblocks,complistsize)
						);
						data[i] = UNIQUE_PTR_MOVE(tptr);
					}
					STP.registerDispatcher(GICRPDid,&GICRPD);
					STP.registerDispatcher(GIBDWPDid,&GIBDWPD);
					STP.registerDispatcher(GIBPWPDid,&GIBPWPD);
					STP.registerDispatcher(GIMWPDid,&GIMWPD);
					STP.registerDispatcher(GICRWPDid,&GICRWPD);
					STP.registerDispatcher(GICBCWPDid,&GICBCWPD);
					STP.registerDispatcher(GICBWPDid,&GICBWPD);


					// put BAM header in compression queue
					enqueHeader();
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

				void waitMergingFinished()
				{
					while ( 
						! mergingFinished && ! STP.isInPanicMode()
					)
						sleep(1);
				}

				void waitRewriteFinished()
				{
					while ( 
						! getRewriteFinished() && ! STP.isInPanicMode()
					)
						sleep(1);
				}
				
				void waitWritingFinished()
				{
					while ( 
						! getLastBlockWritten() && ! STP.isInPanicMode()
					)
						sleep(1);	
				}
				
				void addPending()
				{
					for ( uint64_t streamid = 0; streamid < data.size(); ++streamid )
					{
						libmaus::bambam::parallel::GenericInputControlReadWorkPackage * package = readWorkPackages.getPackage();
						*package = libmaus::bambam::parallel::GenericInputControlReadWorkPackage(0 /* prio */, GICRPDid, data[streamid].get());
						STP.enque(package);		
					}
				}
				
				void genericInputControlReadWorkPackageReturn(libmaus::bambam::parallel::GenericInputControlReadWorkPackage * package)
				{
					readWorkPackages.returnPackage(package);
				}

				void genericInputBgzfDecompressionWorkPackageReturn(libmaus::bambam::parallel::GenericInputBgzfDecompressionWorkPackage * package)
				{
					decompressWorkPackages.returnPackage(package);
				}

				void genericInputBamParseWorkPackageReturn(libmaus::bambam::parallel::GenericInputBamParseWorkPackage * package)
				{
					parseWorkPackages.returnPackage(package);	
				}

				void genericInputMergeWorkPackageReturn(libmaus::bambam::parallel::GenericInputMergeWorkPackage * package)
				{
					mergeWorkPackages.returnPackage(package);
				}

				void genericInputControlReorderWorkPackageReturn(libmaus::bambam::parallel::GenericInputControlReorderWorkPackage * package)
				{
					rewriteWorkPackages.returnPackage(package);
				}

				void genericInputControlBlockCompressionWorkPackageReturn(libmaus::bambam::parallel::GenericInputControlBlockCompressionWorkPackage * package)
				{
					compressWorkPackages.returnPackage(package);
				}

				void genericInputControlBlockWritePackageReturn(libmaus::bambam::parallel::GenericInputControlBlockWritePackage * package)
				{
					writeWorkPackages.returnPackage(package);	
				}

				libmaus::lz::BgzfDeflateZStreamBase::shared_ptr_type genericInputControlGetCompressor()
				{
					libmaus::lz::BgzfDeflateZStreamBase::shared_ptr_type comp = compressorFreeList.get();
					return comp;
				}

				void genericInputControlPutCompressor(libmaus::lz::BgzfDeflateZStreamBase::shared_ptr_type comp)
				{
					compressorFreeList.put(comp);
				}

				void genericInputBgzfDecompressionWorkPackageMemInputBlockReturn(uint64_t streamid, libmaus::bambam::parallel::MemInputBlock::shared_ptr_type ptr)
				{
					data[streamid]->meminputblockfreelist.put(ptr);
					checkDecompressionBlockPending(streamid);
				}

				void genericInputControlSetMergeStallSlot(libmaus::bambam::parallel::AlignmentBuffer::shared_ptr_type rmergestallslot)
				{
					libmaus::parallel::ScopePosixSpinLock slock(mergestallslotlock);
					assert ( ! mergestallslot );
					mergestallslot = rmergestallslot;
				}
				
				bool getRewriteFinished()
				{
					libmaus::parallel::ScopePosixSpinLock rlock(rewriteFinishedLock);
					return rewriteFinished;	
				}
				
				bool getLastBlockWritten()
				{
					libmaus::parallel::ScopePosixSpinLock slock(lastBlockWrittenLock);
					return lastBlockWritten;
				}
				
				void checkBlockOutputQueue()
				{
					libmaus::parallel::ScopePosixSpinLock slock(compressedBlockWriteQueueLock);
					if ( compressedBlockWriteQueue.size() && compressedBlockWriteQueue.top().absid == compressedBlockWriteQueueNext )
					{
						libmaus::bambam::parallel::GenericInputControlCompressionPending GICCP = compressedBlockWriteQueue.top();
						compressedBlockWriteQueue.pop();
						
						libmaus::bambam::parallel::GenericInputControlBlockWritePackage * package = writeWorkPackages.getPackage();
						*package = libmaus::bambam::parallel::GenericInputControlBlockWritePackage(0/*prio*/, GICBWPDid, GICCP, &out);
						STP.enque(package);
					}
				}

				void genericInputControlBlockWritePackageBlockWritten(libmaus::bambam::parallel::GenericInputControlCompressionPending GICCP)
				{
					// return output block
					compressBufferFreeList.put(GICCP.outblock);
					checkCompressionPending();

					{
					libmaus::parallel::ScopePosixSpinLock slock(compressedBlockWriteQueueLock);
					compressedBlockWriteQueueNext += 1;
					}
					
					checkBlockOutputQueue();
					
					if ( GICCP.final )
					{
						// add EOF block
						libmaus::lz::BgzfDeflate<std::ostream> defl(out);
						defl.addEOFBlock();
						
						// std::cerr << "last block written." << std::endl;
						libmaus::parallel::ScopePosixSpinLock slock(lastBlockWrittenLock);
						lastBlockWritten = true;
					}
				}

				void genericInputControlBlockCompressionFinished(libmaus::bambam::parallel::GenericInputControlCompressionPending GICCP)
				{
					// block finished
					{
						libmaus::parallel::ScopePosixSpinLock rlock(compressionUnfinishedLock);
						if ( ! --compressionUnfinished[GICCP.blockid] )
						{
							{
								libmaus::parallel::ScopePosixSpinLock rlock(compressionActiveLock);
								if ( compressionActive.find(GICCP.blockid) != compressionActive.end() )
								{
									libmaus::bambam::parallel::FragmentAlignmentBuffer::shared_ptr_type block =
										compressionActive.find(GICCP.blockid)->second;
									compressionActive.erase(compressionActive.find(GICCP.blockid));

									// return block
									rewriteBlockFreeList.put(block);
									checkRewritePendingQueue();
								}
							}		
						}
					}

					{
						libmaus::parallel::ScopePosixSpinLock slock(compressedBlockWriteQueueLock);
						compressedBlockWriteQueue.push(GICCP);
					}
					
					checkBlockOutputQueue();				
				}

				void checkCompressionPending()
				{
					std::vector<libmaus::bambam::parallel::GenericInputControlCompressionPending> readylist;
					
					{
						libmaus::parallel::ScopePosixSpinLock rlock(compressionPendingLock);
						libmaus::lz::BgzfDeflateOutputBufferBase::shared_ptr_type ptr;
						
						while ( 
							compressionPending.size()
							&&
							(ptr=compressBufferFreeList.getIf())
						)
						{
							libmaus::bambam::parallel::GenericInputControlCompressionPending GICCP = compressionPending.front();
							compressionPending.pop_front();
							GICCP.outblock = ptr;				
							readylist.push_back(GICCP);
						}
					}
					
					for ( uint64_t i = 0; i < readylist.size(); ++i )
					{
						libmaus::bambam::parallel::GenericInputControlBlockCompressionWorkPackage * package = compressWorkPackages.getPackage();
						*package = libmaus::bambam::parallel::GenericInputControlBlockCompressionWorkPackage(
							0 /* prio */,GICBCWPDid,readylist[i]
						);
						STP.enque(package);
					}
				}	

				void checkRewriteReorderQueue()
				{
					bool final = false;
					
					libmaus::parallel::ScopePosixSpinLock slock(rewriteReorderQueueLock);
				
					while ( rewriteReorderQueue.size() && rewriteReorderQueue.top()->id == rewriteReorderNext )
					{
						libmaus::bambam::parallel::FragmentAlignmentBuffer::shared_ptr_type block = rewriteReorderQueue.top();
						rewriteReorderQueue.pop();

						std::vector<std::pair<uint8_t *,uint8_t *> > V;
						block->getLinearOutputFragments(libmaus::lz::BgzfConstants::getBgzfMaxBlockSize(),V);
						
						// std::cerr << "finished rewrite for block " << block->id << "\t" << block->getFill() << "\t" << V.size() << std::endl;

						{
						libmaus::parallel::ScopePosixSpinLock rlock(compressionPendingLock);
						for ( uint64_t i = 0; i < V.size(); ++i )
						{
							compressionPending.push_back(
								libmaus::bambam::parallel::GenericInputControlCompressionPending(
									block->id,
									i,
									compressionPendingAbsIdNext++,
									((i+1==V.size())&&block->final),
									V[i]
								)
							);
						}
						}
						
						{
						libmaus::parallel::ScopePosixSpinLock rlock(compressionUnfinishedLock);
						compressionUnfinished[block->id] = V.size();
						}
						
						{
						libmaus::parallel::ScopePosixSpinLock rlock(compressionActiveLock);
						compressionActive[block->id] = block;
						}

						if ( block->final )
							final = true;
						
						rewriteReorderNext += 1;

						#if 0
						rewriteBlockFreeList.put(block);
						checkRewritePendingQueue();
						#endif
					}
					
					checkCompressionPending();

					if ( final )
					{
						libmaus::parallel::ScopePosixSpinLock rlock(rewriteFinishedLock);
						rewriteFinished = true;
					}
				}

				void genericInputControlReorderWorkPackageFinished(
					libmaus::bambam::parallel::AlignmentBuffer::shared_ptr_type in,
					libmaus::bambam::parallel::FragmentAlignmentBuffer::shared_ptr_type out
				)
				{
					bool finished = false;
					
					{
						libmaus::parallel::ScopePosixSpinLock slock(rewritePendingQueueLock);
						if ( ! --rewriteUnfinished[in->id] )
							finished = true;
					}
					
					if ( finished )
					{
						// return merge buffer
						mergebufferfreelist.put(in);
						checkMergeWorkRequests();
					
						{
						libmaus::parallel::ScopePosixSpinLock slock(rewriteReorderQueueLock);
						rewriteReorderQueue.push(out);
						}

						checkRewriteReorderQueue();
					}
				}
				
				void checkRewritePendingQueue()
				{
					libmaus::parallel::ScopePosixSpinLock slock(rewritePendingQueueLock);
					libmaus::bambam::parallel::FragmentAlignmentBuffer::shared_ptr_type ptr;
					
					while ( 
						rewritePendingQueue.size() 
						&&
						(ptr=rewriteBlockFreeList.getIf())
					)
					{
						libmaus::bambam::parallel::AlignmentBuffer::shared_ptr_type block = rewritePendingQueue.front();
						rewritePendingQueue.pop_front();
						
						ptr->reset();
						ptr->id = block->id;
						ptr->final = block->final;
						
						uint64_t const el = block->fill();
						uint64_t const threads = STP.getNumThreads();
						uint64_t const elperthread = (el + threads - 1) / threads;
						std::vector< std::pair<uint64_t,uint64_t> > packs;
						for ( uint64_t i = 0; i < threads; ++i )
						{
							uint64_t const low = std::min(i*elperthread,el);
							uint64_t const high = std::min(low+elperthread,el);
							packs.push_back(std::pair<uint64_t,uint64_t>(low,high));
							// std::cerr << "block " << block->id << " pack [" << low << "," << high << ")" << std::endl;
						}

						rewriteUnfinished[block->id] = packs.size();

						for ( uint64_t i = 0; i < packs.size(); ++i )
						{
							libmaus::bambam::parallel::GenericInputControlReorderWorkPackage * package = rewriteWorkPackages.getPackage();
							*package = libmaus::bambam::parallel::GenericInputControlReorderWorkPackage(0/* prio */, GICRWPDid, block,ptr, packs[i], i);
							STP.enque(package);
						}

						#if 0
						// return merge buffer
						mergebufferfreelist.put(block);
						checkMergeWorkRequests();
						// return rewrite buffer
						rewriteBlockFreeList.put(ptr);
						#endif
						
						ptr = libmaus::bambam::parallel::FragmentAlignmentBuffer::shared_ptr_type();
					}
					
					if ( ptr )
					{
						rewriteBlockFreeList.put(ptr);
						ptr = libmaus::bambam::parallel::FragmentAlignmentBuffer::shared_ptr_type();		
					}
				}

				// merge block finished
				void genericInputControlMergeBlockFinished(libmaus::bambam::parallel::AlignmentBuffer::shared_ptr_type rmergeblock)
				{
					// is this the last block?
					bool const final = rmergeblock->final;
					
					// bring pointers to the correct order
					rmergeblock->reorder();

					// update counter
					{
						libmaus::parallel::ScopePosixSpinLock lmergedReadsLock(mergedReadsLock);
						mergedReads += rmergeblock->fill();
						
						if ( (mergedReads / (1024*1024) != mergedReadsLastPrint / (1024*1024)) )
						{
							std::cerr << "[V] " << mergedReads << std::endl;
							mergedReadsLastPrint = mergedReads;
						}
					}

					// put block in rewrite pending queue		
					{
						libmaus::parallel::ScopePosixSpinLock slock(rewritePendingQueueLock);
						rewritePendingQueue.push_back(rmergeblock);
					}
					
					checkRewritePendingQueue();

					if ( final )
					{
						mergingFinishedLock.lock();
						mergingFinished = true;
						mergingFinishedLock.unlock();
						std::cerr << "[V] " << mergedReads << std::endl;
						// std::cerr << static_cast<uint64_t>(GIMWPD.LC) << "\t" << mergedReads << std::endl;
					}
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
				
				void checkMergeWorkRequests()
				{
					{
						libmaus::parallel::ScopePosixSpinLock slock(mergeworkrequestslock);
						assert ( mergeworkrequests.size() <= 1 );

						if ( mergeworkrequests.size() )
						{
							libmaus::parallel::ScopePosixSpinLock smergestallslotlock(mergestallslotlock);
							
							libmaus::bambam::parallel::AlignmentBuffer::shared_ptr_type buffer;
							
							if ( !buffer && mergestallslot )
							{
								buffer = mergestallslot; 
								mergestallslot = libmaus::bambam::parallel::AlignmentBuffer::shared_ptr_type();
							}
							if ( !buffer )
							{
								buffer = mergebufferfreelist.getIf();

								if ( buffer )
								{
									nextMergeBufferIdLock.lock();
									buffer->id = nextMergeBufferId++;
									buffer->reset();
									nextMergeBufferIdLock.unlock();
								}
							}
							
							if ( buffer )
							{
							
								mergeworkrequests.pop_front();
							
								libmaus::bambam::parallel::GenericInputMergeWorkPackage * package = mergeWorkPackages.getPackage();
								*package = libmaus::bambam::parallel::GenericInputMergeWorkPackage(0/*prio*/,GIMWPDid,&data,&mergeheap,buffer);
								STP.enque(package);
							}
						}
					}
				}

				void genericInputControlMergeRequeue()
				{
					{
					libmaus::parallel::ScopePosixSpinLock slock(mergeworkrequestslock);
					mergeworkrequests.push_back(libmaus::bambam::parallel::GenericInputMergeWorkRequest());
					}
					
					checkMergeWorkRequests();
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

						#if 0			
						if ( final )
							std::cerr << "final block for stream " << streamid << std::endl;
						#endif

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
								libmaus::bambam::parallel::GenericInputControlMergeHeapEntry GICMHE(data[streamid]->processActive.get());
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
									libmaus::parallel::ScopePosixSpinLock slock(mergeworkrequestslock);
									assert ( ! mergeworkrequests.size() );
									mergeworkrequests.push_back(libmaus::bambam::parallel::GenericInputMergeWorkRequest());
								}
							}
							// not first package, merging has already started
							else
							{
								// (re)start merging
								libmaus::parallel::ScopePosixSpinLock slock(mergeworkrequestslock);
								assert ( ! mergeworkrequests.size() );
								mergeworkrequests.push_back(libmaus::bambam::parallel::GenericInputMergeWorkRequest());
							}					
						}
					}

					checkMergeWorkRequests();
					
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
					
					#if 0
					if ( block && block->final )
					{
						std::cerr << "[V] all blocks decompressed for stream " << streamid << std::endl;
					}
					#endif

					if ( block )
					{
						libmaus::bambam::parallel::GenericInputBamParseWorkPackage * package = parseWorkPackages.getPackage();
						*package = libmaus::bambam::parallel::GenericInputBamParseWorkPackage(0/*prio*/,GIBPWPDid,libmaus::bambam::parallel::GenericInputBamParseObject(&(data[streamid]->parseInfo),block));
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
					libmaus::bambam::parallel::GenericInputBase::generic_input_shared_block_ptr_type block, uint64_t /* subblockid */
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
							libmaus::bambam::parallel::GenericInputControlReadWorkPackage * package = readWorkPackages.getPackage();
							*package = libmaus::bambam::parallel::GenericInputControlReadWorkPackage(0 /* prio */, GICRPDid, data[streamid].get());
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
						
						libmaus::bambam::parallel::GenericInputControlSubBlockPending pend = data[streamid]->decompressionpending.top();
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

						libmaus::bambam::parallel::GenericInputBgzfDecompressionWorkPackage * package = decompressWorkPackages.getPackage();
						*package = libmaus::bambam::parallel::GenericInputBgzfDecompressionWorkPackage(0/*prio*/,GIBDWPDid,pend);
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
					std::vector<libmaus::bambam::parallel::GenericInputBase::generic_input_shared_block_ptr_type> readylist;
					
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
							libmaus::bambam::parallel::GenericInputBase::generic_input_shared_block_ptr_type block = data[streamid]->pending.top();
							data[streamid]->pending.pop();
							readylist.push_back(block);	
							data[streamid]->decompressiontotal.push_back(block->meta.blocks.size());
							data[streamid]->nextblockid += 1;		
						}
					}
					
					// enque decompression requests in queue
					for ( std::vector<libmaus::bambam::parallel::GenericInputBase::generic_input_shared_block_ptr_type>::size_type i = 0; i < readylist.size(); ++i )
					{
						// get block
						libmaus::bambam::parallel::GenericInputBase::generic_input_shared_block_ptr_type block = readylist[i];

						// mark sub blocks as pending
						for ( uint64_t j = 0; j < block->meta.blocks.size(); ++j )
						{
							libmaus::parallel::ScopePosixSpinLock slock(data[streamid]->lock);
							data[streamid]->decompressionpending.push(libmaus::bambam::parallel::GenericInputControlSubBlockPending(block,j));
						}
					}
					
					checkDecompressionBlockPending(streamid);
				}

				void genericInputControlReadAddPending(libmaus::bambam::parallel::GenericInputBase::generic_input_shared_block_ptr_type block)
				{
					uint64_t const streamid = block->meta.streamid;
					
					{
						libmaus::parallel::ScopePosixSpinLock slock(data[streamid]->lock);
						data[streamid]->pending.push(block);
					}
					
					checkInputBlockPending(streamid);
				}
			};
		}
	}
}

int main(int argc, char * argv[])
{
	try
	{
		#if 0
		{
			libmaus::util::ArgInfo const arginfo(argc,argv);
			uint64_t const numlogcpus = arginfo.getValue<int>("threads",libmaus::parallel::NumCpus::getNumLogicalProcessors());
			
			std::vector<libmaus::bambam::parallel::GenericInputControlStreamInfo> in;
			for ( uint64_t i = 0; i < arginfo.restargs.size(); ++i )
			{
				std::string const fn = arginfo.restargs[i];
				in.push_back(
					GenericInputControlStreamInfo(fn,false /* finite */,
						0, /* start */
						0, /* end */
						true /* has header */
					)
				);
			}
			
			uint64_t const inputblocksize = 1024*1024;
			uint64_t const inputblocksperfile = 8;
			uint64_t const mergebuffersize = 1024*1024;
			uint64_t const mergebuffers = 16;
			uint64_t const complistsize = 64;

			libmaus::parallel::SimpleThreadPool STP(numlogcpus);			
			GenericInputControl GIC(STP,in,inputblocksize,inputblocksperfile /* blocks per channel */,mergebuffersize /* merge buffer size */,mergebuffers /* number of merge buffers */, complistsize /* number of bgzf preload blocks */);
			GIC.addPending();
			
			GIC.waitCompressionFinished();
			std::cerr << "fini." << std::endl;

			STP.terminate();
			STP.join();
		}
		
		return 0;
		#endif
	
		libmaus::timing::RealTimeClock progrtc; progrtc.start();
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
			libmaus::timing::RealTimeClock rtc;
			
			rtc.start();
			uint64_t const numlogcpus = arginfo.getValue<int>("threads",libmaus::parallel::NumCpus::getNumLogicalProcessors());
			libmaus::aio::PosixFdInputStream in(STDIN_FILENO,256*1024);
			std::string const tmpfilebase = arginfo.getUnparsedValue("tmpfile",arginfo.getDefaultTmpFileName());
			int const templevel = arginfo.getValue<int>("level",1);

			std::string const sinputformat = arginfo.getUnparsedValue("inputformat","bam");
			libmaus::bambam::parallel::BlockSortControlBase::block_sort_control_input_enum inform = libmaus::bambam::parallel::BlockSortControlBase::block_sort_control_input_bam;
			
			if ( sinputformat == "bam" )
			{
				inform = libmaus::bambam::parallel::BlockSortControlBase::block_sort_control_input_bam;
			}
			else if ( sinputformat == "sam" )
			{
				inform = libmaus::bambam::parallel::BlockSortControlBase::block_sort_control_input_sam;			
			}
			else
			{
				libmaus::exception::LibMausException lme;
				lme.getStream() << "Unknown input format " << sinputformat << std::endl;
				lme.finish();
				throw lme;				
			}
						
			libmaus::parallel::SimpleThreadPool STP(numlogcpus);
			libmaus::bambam::parallel::BlockSortControl<order_type>::unique_ptr_type VC(
				new libmaus::bambam::parallel::BlockSortControl<order_type>(
					inform,STP,in,templevel,tmpfilebase
				)
			);
			VC->enqueReadPackage();
			VC->waitDecodingFinished();
			VC->flushReadEndsLists();
			// VC->flushBlockFile();

			std::vector<libmaus::bambam::parallel::GenericInputControlStreamInfo> const BI = VC->getBlockInfo();
			libmaus::bitio::BitVector::unique_ptr_type Pdupvec(VC->releaseDupBitVector());
			libmaus::bambam::BamHeader::unique_ptr_type Pheader(VC->getHeader());
			::libmaus::bambam::BamHeader::unique_ptr_type uphead(libmaus::bambam::BamHeaderUpdate::updateHeader(arginfo,*Pheader,"testparallelbamblocksort",PACKAGE_VERSION));
			uphead->changeSortOrder("coordinate");
			std::ostringstream hostr;
			uphead->serialise(hostr);
			std::string const hostrstr = hostr.str();
			libmaus::autoarray::AutoArray<char> sheader(hostrstr.size(),false);
			std::copy(hostrstr.begin(),hostrstr.end(),sheader.begin());
			
			VC.reset();
			
			std::cerr << "[V] blocks generated in time " << rtc.formatTime(rtc.getElapsedSeconds()) << std::endl;
			
			rtc.start();
			uint64_t const inputblocksize = 1024*1024;
			uint64_t const inputblocksperfile = 8;
			uint64_t const mergebuffersize = 256*1024*1024;
			uint64_t const mergebuffers = 4;
			uint64_t const complistsize = 32;
			int const level = arginfo.getValue<int>("level",Z_DEFAULT_COMPRESSION);

			libmaus::bambam::parallel::GenericInputControl GIC(
				STP,std::cout,sheader,BI,*Pdupvec,level,inputblocksize,inputblocksperfile /* blocks per channel */,mergebuffersize /* merge buffer size */,mergebuffers /* number of merge buffers */, complistsize /* number of bgzf preload blocks */);
			GIC.addPending();			
			GIC.waitWritingFinished();
			std::cerr << "[V] blocks merged in time " << rtc.formatTime(rtc.getElapsedSeconds()) << std::endl;

			STP.terminate();
			STP.join();

			#if 0			
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

			libmaus::bambam::parallel::GenericInputControlStreamInfo(blockfilename,
				true /* finite */,
				blockStarts[i],
				blockEnds[i],
				false /* has header */
			)
			
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
			libmaus::bambam::BamAuxFilterVector filter;
			filter.set('Z','R');
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
					
				algns[t].filterOutAux(filter);

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
			
			for ( uint64_t i = 0; i < VC.blockAlgnCnt.size(); ++i )
			{
				std::cerr << "[B] " << Ain[i]->tellg() << "\t" << VC.blockEnds[i] << std::endl;
			}
			#endif
		}
		
		std::cerr << "[V] run time " << progrtc.formatTime(progrtc.getElapsedSeconds()) << " (" << progrtc.getElapsedSeconds() << " s)" << std::endl;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
