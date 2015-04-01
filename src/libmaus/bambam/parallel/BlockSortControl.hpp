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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_BLOCKSORTCONTROL_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_BLOCKSORTCONTROL_HPP

#include <libmaus/aio/NamedTemporaryFileAllocator.hpp>
#include <libmaus/aio/NamedTemporaryFileTypeInfo.hpp>
#include <libmaus/aio/PosixFdOutputStream.hpp>
#include <libmaus/bambam/SamInfoAllocator.hpp>
#include <libmaus/bambam/SamInfoTypeInfo.hpp>
#include <libmaus/bambam/parallel/AlignmentBufferAllocator.hpp>
#include <libmaus/bambam/parallel/AlignmentBufferHeapComparator.hpp>
#include <libmaus/bambam/parallel/AlignmentBufferTypeInfo.hpp>
#include <libmaus/bambam/parallel/BgzfLinearMemCompressWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/BlockSortControlBase.hpp>
#include <libmaus/bambam/parallel/DecompressedPendingObjectHeapComparator.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferAllocator.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferBaseSortWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferHeapComparator.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferMergeSortWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferReorderWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferRewriteReadEndsWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferSortContext.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferSortFinishedInterface.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferTypeInfo.hpp>
#include <libmaus/bambam/parallel/FragReadEndsContainerFlushWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/FragReadEndsMergeWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/GenericInputBgzfDecompressionWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/GenericInputControlReadWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/PairReadEndsContainerFlushWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/PairReadEndsMergeWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/ParseBlockWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/ReadEndsContainerAllocator.hpp>
#include <libmaus/bambam/parallel/ReadEndsContainerTypeInfo.hpp>
#include <libmaus/bambam/parallel/SamParseWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/SmallLinearBlockCompressionPendingObjectHeapComparator.hpp>
#include <libmaus/bambam/parallel/ValidateBlockFragmentWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/WriteBlockWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/WritePendingObjectHeapComparator.hpp>
#include <libmaus/lz/BgzfDeflateOutputBufferBaseAllocator.hpp>
#include <libmaus/lz/BgzfDeflateOutputBufferBaseTypeInfo.hpp>
#include <libmaus/lz/BgzfDeflateZStreamBaseAllocator.hpp>
#include <libmaus/lz/BgzfDeflateZStreamBaseTypeInfo.hpp>
#include <libmaus/parallel/LockedGrowingFreeList.hpp>
#include <libmaus/parallel/LockedHeap.hpp>
#include <libmaus/parallel/SimpleThreadPool.hpp>
#include <libmaus/parallel/SimpleThreadPoolWorkPackageFreeList.hpp>
#include <libmaus/util/UnitNum.hpp>

#include <libmaus/bambam/ChecksumsInterfaceAllocator.hpp>
#include <libmaus/bambam/ChecksumsInterfaceTypeInfo.hpp>
#include <libmaus/bambam/parallel/ChecksumsInterfaceGetInterface.hpp>
#include <libmaus/bambam/parallel/ChecksumsInterfacePutInterface.hpp>
			
namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			template<typename _order_type, bool _create_dup_mark_info>
			struct BlockSortControl :
				public BlockSortControlBase,
				public libmaus::bambam::parallel::GenericInputControlReadWorkPackageReturnInterface,
				public libmaus::bambam::parallel::GenericInputControlReadAddPendingInterface,
				public libmaus::bambam::parallel::GenericInputBgzfDecompressionWorkPackageReturnInterface,
				public libmaus::bambam::parallel::GenericInputBgzfDecompressionWorkPackageMemInputBlockReturnInterface,
				public libmaus::bambam::parallel::GenericInputBgzfDecompressionWorkPackageDecompressedBlockReturnInterface,
				public libmaus::bambam::parallel::GenericInputBgzfDecompressionWorkSubBlockDecompressionFinishedInterface,
				public DecompressedBlockAddPendingInterface,
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
				public SamParsePutSamInfoInterface,
				public ChecksumsInterfaceGetInterface,
				public ChecksumsInterfacePutInterface
			{
				typedef _order_type order_type;
				static bool const create_dup_mark_info = _create_dup_mark_info;
				typedef BlockSortControl<order_type,create_dup_mark_info> this_type;
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

				libmaus::parallel::LockedGrowingFreeList<
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
				FragmentAlignmentBufferRewriteReadEndsWorkPackageDispatcher<create_dup_mark_info /* create dup mark info */> FABRWPD;
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

				// read
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<libmaus::bambam::parallel::GenericInputControlReadWorkPackage> genericInputReadWorkPackages;
				// decmpress
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<libmaus::bambam::parallel::GenericInputBgzfDecompressionWorkPackage> genericInputDecompressWorkPackages;
				// parse
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<ParseBlockWorkPackage> parseBlockWorkPackages;
				// validate
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<ValidateBlockFragmentWorkPackage> validateBlockFragmentWorkPackages;
				// compress
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<BgzfLinearMemCompressWorkPackage> bgzfWorkPackages;
				// write
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<WriteBlockWorkPackage> writeWorkPackages;
				// rewrite
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<FragmentAlignmentBufferRewriteReadEndsWorkPackage> fragmentAlignmentBufferRewriteWorkPackages;
				// base sort
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<FragmentAlignmentBufferBaseSortPackage<order_type> > baseSortPackages;
				// merge
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<FragmentAlignmentBufferMergeSortWorkPackage<order_type> > mergeSortPackages;
				// reorder
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<FragmentAlignmentBufferReorderWorkPackage> reorderPackages;
				// frag container flushing
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<FragReadEndsContainerFlushWorkPackage> fragReadContainerFlushPackages;
				// pair container flushing
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<PairReadEndsContainerFlushWorkPackage> pairReadContainerFlushPackages;
				// frag merging
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<FragReadEndsMergeWorkPackage> fragReadEndsMergeWorkPackages;
				// pair merging
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<PairReadEndsMergeWorkPackage> pairReadEndsMergeWorkPackages;
				// sam parsing
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<SamParseWorkPackage> samParseWorkPackages;
				
				std::ostream & printPackageFreeListSizes(std::ostream & out)
				{
					out << "[C] genericInputReadWorkPackages: " << genericInputReadWorkPackages.size() << "\n";
					out << "[C] genericInputDecompressWorkPackages: " << genericInputDecompressWorkPackages.size() << "\n";
					out << "[C] parseBlockWorkPackages: " << parseBlockWorkPackages.size() << "\n";
					out << "[C] validateBlockFragmentWorkPackages: " << validateBlockFragmentWorkPackages.size() << "\n";
					out << "[C] bgzfWorkPackages: " << bgzfWorkPackages.size() << "\n";
					out << "[C] writeWorkPackages: " << writeWorkPackages.size() << "\n";
					out << "[C] fragmentAlignmentBufferRewriteWorkPackages: " << fragmentAlignmentBufferRewriteWorkPackages.size() << "\n";
					out << "[C] baseSortPackages: " << baseSortPackages.size() << "\n";
					out << "[C] mergeSortPackages: " << mergeSortPackages.size() << "\n";
					out << "[C] reorderPackages: " << reorderPackages.size() << "\n";
					out << "[C] fragReadContainerFlushPackages: " << fragReadContainerFlushPackages.size() << "\n";
					out << "[C] pairReadContainerFlushPackages: " << pairReadContainerFlushPackages.size() << "\n";
					out << "[C] fragReadEndsMergeWorkPackages: " << fragReadEndsMergeWorkPackages.size() << "\n";
					out << "[C] pairReadEndsMergeWorkPackages: " << pairReadEndsMergeWorkPackages.size() << "\n";
					out << "[C] samParseWorkPackages: " << samParseWorkPackages.size() << "\n";
					return out;
				}
				
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
				typedef libmaus::parallel::LockedFreeList<AlignmentBuffer,AlignmentBufferAllocator,AlignmentBufferTypeInfo> parse_block_free_list_type;
				typedef parse_block_free_list_type::unique_ptr_type parse_block_free_list_pointer_type;
				parse_block_free_list_pointer_type parseBlockFreeList;

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
				
				typedef libmaus::parallel::LockedFreeList<FragmentAlignmentBuffer,FragmentAlignmentBufferAllocator,FragmentAlignmentBufferTypeInfo>
					fragment_buffer_free_list_presort_type;
				typedef fragment_buffer_free_list_presort_type::unique_ptr_type fragment_buffer_free_list_presort_pointer_type;
				
				fragment_buffer_free_list_presort_pointer_type fragmentBufferFreeListPreSort;
				
				typedef libmaus::parallel::LockedFreeList<FragmentAlignmentBuffer,FragmentAlignmentBufferAllocator,FragmentAlignmentBufferTypeInfo>
					fragment_buffer_free_list_postsort_type;
				typedef fragment_buffer_free_list_postsort_type::unique_ptr_type fragment_buffer_free_list_postsort_pointer_type;
				
				fragment_buffer_free_list_postsort_pointer_type fragmentBufferFreeListPostSort;

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

				typedef libmaus::parallel::LockedGrowingFreeList<
					libmaus::bambam::ChecksumsInterface,
					libmaus::bambam::ChecksumsInterfaceAllocator,
					libmaus::bambam::ChecksumsInterfaceTypeInfo
				> checksums_free_list_type;
				typedef checksums_free_list_type::unique_ptr_type checksums_free_list_pointer_type;
				checksums_free_list_pointer_type checksums_free_list;
				
				ChecksumsInterface::shared_ptr_type combinedchecksums;
				
				std::string const hash;
				
				void freeBuffers()
				{
					fragmentBufferFreeListPreSort.reset();
					fragmentBufferFreeListPostSort.reset();
					parseBlockFreeList.reset();
				}

				std::ostream & printSizes(std::ostream & out)
				{
					out << "[M] fragmentBufferFreeListPreSort: "  << libmaus::util::UnitNum::unitNum(fragmentBufferFreeListPreSort->byteSize())
						<< " capacity=" << fragmentBufferFreeListPreSort->capacity() << " free=" << fragmentBufferFreeListPreSort->freeUnlocked()
						<< std::endl;
					out << "[M] fragmentBufferFreeListPostSort: " << libmaus::util::UnitNum::unitNum(fragmentBufferFreeListPostSort->byteSize())
						<< " capacity=" << fragmentBufferFreeListPostSort->capacity() << " free=" << fragmentBufferFreeListPostSort->freeUnlocked()
						<< std::endl;
					out << "[M] parseBlockFreeList: " << libmaus::util::UnitNum::unitNum(parseBlockFreeList->byteSize()) 
						<< " capacity=" << parseBlockFreeList->capacity() << " free=" << parseBlockFreeList->freeUnlocked()
						<< std::endl;
					out << "[M] bgzfDeflateOutputBufferFreeList: " << libmaus::util::UnitNum::unitNum(bgzfDeflateOutputBufferFreeList.byteSize()) 
						<< " capacity=" << bgzfDeflateOutputBufferFreeList.capacity() << " free=" << bgzfDeflateOutputBufferFreeList.freeUnlocked()
						<< std::endl;
					out << "[M] readEndsFragContainerFreeList: " << libmaus::util::UnitNum::unitNum(readEndsFragContainerFreeList.byteSize()) 
						<< " capacity=" << readEndsFragContainerFreeList.capacity() << " free=" << readEndsFragContainerFreeList.freeUnlocked()
						<< std::endl;
					out << "[M] readEndsPairContainerFreeList: " << libmaus::util::UnitNum::unitNum(readEndsPairContainerFreeList.byteSize()) 
						<< " capacity=" << readEndsPairContainerFreeList.capacity() << " free=" << readEndsPairContainerFreeList.freeUnlocked()
						<< std::endl;
					out << "[M] inputreadbase: " << libmaus::util::UnitNum::unitNum(inputreadbase.byteSize()) << std::endl;
					out << "[M] bgzfDeflateZStreamBaseFreeList.getAllSize(): " << bgzfDeflateZStreamBaseFreeList.getAllSize() << std::endl;
					out << "[M] tempFileFreeList->getAllSize()=" << tempFileFreeList->getAllSize() << std::endl;
					out << "[M] parseInfo: " << libmaus::util::UnitNum::unitNum(parseInfo.byteSize()) << "\n";
					return out;
				}

				BlockSortControl(
					block_sort_control_input_enum const rinputType,
					libmaus::parallel::SimpleThreadPool & rSTP,
					std::istream & in,
					int const level,
					std::string const & rtempfileprefix,
					std::string const & rhash,
					bool const rfixmates = true,
					bool const rdupmarksupport = true
				)
				: 
					procrtc(true),
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
					VBFWPD(*this,*this,*this,*this),
					VBFWPDid(STP.getNextDispatcherId()),
					BLMCWPD(*this,*this,*this,*this,*this),
					BLMCWPDid(STP.getNextDispatcherId()),
					WBWPD(*this,*this,*this),
					WBWPDid(STP.getNextDispatcherId()),
					FABRWPD(*this,*this,*this,*this,rfixmates /* fix mates */,rdupmarksupport /* dup mark support */),
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
						new parse_block_free_list_type(
							std::min(STP.getNumThreads(),static_cast<uint64_t>(8)),
							AlignmentBufferAllocator(getParseBufferSize(),1 /* pointer multiplier */)
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
					// pre sorting buffer
					fragmentBufferFreeListPreSort(new fragment_buffer_free_list_presort_type(std::min(STP.getNumThreads(),static_cast<uint64_t>(8)),FragmentAlignmentBufferAllocator(STP.getNumThreads() /* fragments */, 2 /* pointer multiplier */))),
					// post sorting (rewrite/reorder) buffer
					fragmentBufferFreeListPostSort(new fragment_buffer_free_list_postsort_type(std::min(STP.getNumThreads(),static_cast<uint64_t>(8)),FragmentAlignmentBufferAllocator(STP.getNumThreads() /* fragments */, 1 /* pointer multiplier */))),
					postSortNext(0),
					readEndsFragContainerAllocator(getReadEndsContainerSize(),tempfileprefix+"_readends_frag_"),
					readEndsPairContainerAllocator(getReadEndsContainerSize(),tempfileprefix+"_readends_pair_"),
					readEndsFragContainerFreeList(readEndsFragContainerAllocator),
					readEndsPairContainerFreeList(readEndsPairContainerAllocator),
					unflushedFragReadEndsContainers(0),
					unflushedPairReadEndsContainers(0),
					unmergeFragReadEndsRegions(0),
					unmergePairReadEndsRegions(0),
					hash(rhash)
				{
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
								
								if ( P.second != P.first )
								{
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
								}

								// header complete?
								if ( P.first != P.second || block->meta.eof )
								{
									parseInfo.setHeaderFromText(
										inputreadbase.samHeader.begin(),
										inputreadbase.samHeader.size()
									);


									libmaus::parallel::LockedGrowingFreeList<
										libmaus::bambam::SamInfo,
										libmaus::bambam::SamInfoAllocator,
										libmaus::bambam::SamInfoTypeInfo>::unique_ptr_type tsamInfoFreeList(
										new libmaus::parallel::LockedGrowingFreeList<
										libmaus::bambam::SamInfo,
										libmaus::bambam::SamInfoAllocator,
										libmaus::bambam::SamInfoTypeInfo>
										(libmaus::bambam::SamInfoAllocator(parseInfo.Pheader.get()))
									);
									samInfoFreeList = UNIQUE_PTR_MOVE(tsamInfoFreeList);
									
									bamHeaderComplete(parseInfo.BHPS);

									inputreadbase.samHeaderComplete = true;
								}
								
								// if block has been fully processed then return it
								if ( P.first == P.second )
								{
									// is this a file with a header but no reads?
									if ( inputreadbase.samHeaderComplete )
									{
										inputreadbase.pending.push(block);
									}
									else
									{
										returnList.push_back(block);
										inputreadbase.nextblockid += 1;	
									}
								}
								// block is not fully processed, header parsing is complete
								else
								{
									assert ( inputreadbase.samHeaderComplete );
									inputreadbase.pending.push(block);
								}
							}
						}

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
								
								if ( P.second != P.first )
								{
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
								}
								
								{
									libmaus::parallel::ScopePosixSpinLock slock(inputreadbase.samParsePendingQueueLock);
									for ( uint64_t i = 0; i < block->meta.blocks.size(); ++i )
										inputreadbase.samParsePendingQueue.push_back
											(SamParsePending(block,i,inputreadbase.samParsePendingQueueNextAbsId++));
								}
																
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

						#if 0
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

						#if 0
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
				
				void saveFragMergeInfo(std::ostream & out)
				{
					libmaus::util::shared_ptr< std::vector< ::libmaus::bambam::ReadEndsBlockDecoderBaseCollectionInfoBase > >::type
						sMI(getFragMergeInfo());
					std::vector< ::libmaus::bambam::ReadEndsBlockDecoderBaseCollectionInfoBase > & MI = *sMI;
					
					libmaus::util::NumberSerialisation::serialiseNumber(out,MI.size());
					for ( uint64_t i = 0; i < MI.size(); ++i )
						MI[i].moveAndSerialise(out);
				}
				
				void saveFragMergeInfo(std::string const & fn)
				{
					libmaus::aio::PosixFdOutputStream PFIS(fn);
					saveFragMergeInfo(PFIS);
				}
				
				static std::vector< ::libmaus::bambam::ReadEndsBlockDecoderBaseCollectionInfoBase > loadMergeInfo(std::istream & in)
				{
					uint64_t const n = libmaus::util::NumberSerialisation::deserialiseNumber(in);
					std::vector< ::libmaus::bambam::ReadEndsBlockDecoderBaseCollectionInfoBase > V;
					for ( uint64_t i = 0; i < n; ++i )
						V.push_back( ::libmaus::bambam::ReadEndsBlockDecoderBaseCollectionInfoBase(in) );
					return V;
				}
				
				static std::vector< ::libmaus::bambam::ReadEndsBlockDecoderBaseCollectionInfoBase > loadMergeInfo(std::string const & fn)
				{
					libmaus::aio::PosixFdInputStream PFIS(fn);
					return loadMergeInfo(PFIS);
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

				void savePairMergeInfo(std::ostream & out)
				{
					libmaus::util::shared_ptr< std::vector< ::libmaus::bambam::ReadEndsBlockDecoderBaseCollectionInfoBase > >::type
						sMI(getPairMergeInfo());
					std::vector< ::libmaus::bambam::ReadEndsBlockDecoderBaseCollectionInfoBase > & MI = *sMI;
					
					libmaus::util::NumberSerialisation::serialiseNumber(out,MI.size());
					for ( uint64_t i = 0; i < MI.size(); ++i )
						MI[i].moveAndSerialise(out);
				}

				void savePairMergeInfo(std::string const & fn)
				{
					libmaus::aio::PosixFdOutputStream PFIS(fn);
					savePairMergeInfo(PFIS);
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
				
				static void verifyReadEndsFragments(
					std::vector< ::libmaus::bambam::ReadEndsBlockDecoderBaseCollectionInfoBase > const & fraginfo,
					libmaus::parallel::PosixSpinLock & globallock,
					size_t const cachesize = 16*1024*1024
				)
				{
					#if defined(_OPENMP)
					#pragma omp parallel for
					#endif
					for ( uint64_t z = 0; z < fraginfo.size(); ++z )
					{
						try
						{
							::libmaus::bambam::ReadEndsBlockDecoderBaseCollectionInfoBase const frag = fraginfo[z];
							
							for ( uint64_t i = 0; i < frag.indexoffset.size(); ++i )
							{
								globallock.lock();
								std::cerr << "(" << z << "," << i << ")";
								globallock.unlock();
							
								std::string const & datafilename = frag.datafilename;
								std::string const & indexfilename = frag.indexfilename;
								uint64_t const indexoffset = frag.indexoffset[i];
								uint64_t const blockelcnt = frag.blockelcnt[i];
								
								libmaus::aio::PosixFdInputStream indexPFIS(indexfilename);
								indexPFIS.seekg(indexoffset,std::ios::beg);
								libmaus::index::ExternalMemoryIndexDecoder<
									libmaus::bambam::ReadEndsBase,
									libmaus::bambam::ReadEndsContainerBase::baseIndexShift,
									libmaus::bambam::ReadEndsContainerBase::innerIndexShift		
								> index(indexPFIS,cachesize);

								{
									std::pair<uint64_t,uint64_t> const pos = index[0];
									libmaus::aio::PosixFdInputStream dataPFIS(datafilename);
									dataPFIS.seekg(pos.first);
									libmaus::lz::SnappyInputStream dataSnappy(dataPFIS);
									dataSnappy.ignore(pos.second);
									
									libmaus::bambam::ReadEnds prev;
									bool prevvalid = false;
									libmaus::bambam::ReadEnds RE;
									for ( uint64_t j = 0; j < blockelcnt; ++j )
									{
										RE.get(dataSnappy);
										
										bool const ok = ( (! prevvalid) || (prev < RE) );
										
										if ( ! ok )
										{
											std::cerr << "prev=" << prev << std::endl;
											std::cerr << "RE=" << RE << std::endl;
										}
										
										assert ( ok );
										
										prev = RE;
										prevvalid = true;
									}
								}
								
								{
									uint64_t const baseblocksize = 1ull << libmaus::bambam::ReadEndsContainerBase::baseIndexShift;
									uint64_t const numbaseblocks = (blockelcnt + baseblocksize-1)/baseblocksize;
									
									for ( uint64_t b = 0; b < numbaseblocks; ++b )
									{
										std::pair<uint64_t,uint64_t> const pos = index[b];
										libmaus::aio::PosixFdInputStream dataPFIS(datafilename);
										dataPFIS.seekg(pos.first);
										libmaus::lz::SnappyInputStream dataSnappy(dataPFIS);
										dataSnappy.ignore(pos.second);

										uint64_t const blocklow = b * baseblocksize;
										uint64_t const blockhigh = std::min(blocklow + baseblocksize, blockelcnt);
										uint64_t const blockread = blockhigh-blocklow;
										
										libmaus::bambam::ReadEnds prev;
										bool prevvalid = false;
										libmaus::bambam::ReadEnds RE;
										for ( uint64_t j = 0; j < blockread; ++j )
										{
											RE.get(dataSnappy);
											
											if ( ! j )
											{
												libmaus::bambam::ReadEndsBase const first = index.getBaseLevelBlockStart(b);
												assert ( first == RE );
												
												uint64_t bb = b;
												unsigned int level = 0;
												while ( 
													(bb & ((1ull << libmaus::bambam::ReadEndsContainerBase::innerIndexShift)-1)) == 0
													&&
													(++level < index.levelstarts.size())
												)
												{
													bb >>= libmaus::bambam::ReadEndsContainerBase::innerIndexShift;
													libmaus::bambam::ReadEndsBase const lfirst = index.getLevelBlockStart(level,bb);
													assert ( lfirst == RE );
													// std::cerr << "[b=" << b << ",level=" << level << "]";
												}
											}
										
											bool const ok = ( (! prevvalid) || (prev < RE) );
											
											if ( ! ok )
											{
												std::cerr << "prev=" << prev << std::endl;
												std::cerr << "RE=" << RE << std::endl;
											}
										
											assert ( ok );
										
											prev = RE;
											prevvalid = true;
										}								
									}
								}
							}
						}
						catch(std::exception const & ex)
						{
							globallock.lock();
							std::cerr << ex.what() << std::endl;
							globallock.unlock();
						}
					}				
				}

				static void verifyReadEndsPairs(
					std::vector< ::libmaus::bambam::ReadEndsBlockDecoderBaseCollectionInfoBase > const & pairinfo,
					libmaus::parallel::PosixSpinLock & globallock,
					size_t const cachesize = 16*1024*1024
				)
				{
					#if defined(_OPENMP)
					#pragma omp parallel for
					#endif
					for ( uint64_t z = 0; z < pairinfo.size(); ++z )
					{
						try
						{
							::libmaus::bambam::ReadEndsBlockDecoderBaseCollectionInfoBase const pair = pairinfo[z];
							
							for ( uint64_t i = 0; i < pair.indexoffset.size(); ++i )
							{
								globallock.lock();
								std::cerr << "(" << z << "," << i << ")";
								globallock.unlock();
							
								std::string const & datafilename = pair.datafilename;
								std::string const & indexfilename = pair.indexfilename;
								uint64_t const indexoffset = pair.indexoffset[i];
								uint64_t const blockelcnt = pair.blockelcnt[i];
								
								libmaus::aio::PosixFdInputStream indexPFIS(indexfilename);
								indexPFIS.seekg(indexoffset,std::ios::beg);
								libmaus::index::ExternalMemoryIndexDecoder<
									libmaus::bambam::ReadEndsBase,
									libmaus::bambam::ReadEndsContainerBase::baseIndexShift,
									libmaus::bambam::ReadEndsContainerBase::innerIndexShift		
								> index(indexPFIS,cachesize);
								
								{
									std::pair<uint64_t,uint64_t> const pos = index[0];
									libmaus::aio::PosixFdInputStream dataPFIS(datafilename);
									dataPFIS.seekg(pos.first);
									libmaus::lz::SnappyInputStream dataSnappy(dataPFIS);
									dataSnappy.ignore(pos.second);
									
									libmaus::bambam::ReadEnds prev;
									bool prevvalid = false;
									libmaus::bambam::ReadEnds RE;
									for ( uint64_t j = 0; j < blockelcnt; ++j )
									{
										RE.get(dataSnappy);

										bool const ok = ( (! prevvalid) || (prev < RE) );
										
										if ( ! ok )
										{
											std::cerr << "prev=" << prev << std::endl;
											std::cerr << "RE=" << RE << std::endl;
										}
										
										assert ( ok );

										
										prev = RE;
										prevvalid = true;
									}
								}

								{
									uint64_t const baseblocksize = 1ull << libmaus::bambam::ReadEndsContainerBase::baseIndexShift;
									uint64_t const numbaseblocks = (blockelcnt + baseblocksize-1)/baseblocksize;
									
									for ( uint64_t b = 0; b < numbaseblocks; ++b )
									{
										std::pair<uint64_t,uint64_t> const pos = index[b];
										libmaus::aio::PosixFdInputStream dataPFIS(datafilename);
										dataPFIS.seekg(pos.first);
										libmaus::lz::SnappyInputStream dataSnappy(dataPFIS);
										dataSnappy.ignore(pos.second);

										uint64_t const blocklow = b * baseblocksize;
										uint64_t const blockhigh = std::min(blocklow + baseblocksize, blockelcnt);
										uint64_t const blockread = blockhigh-blocklow;
										
										libmaus::bambam::ReadEnds prev;
										bool prevvalid = false;
										libmaus::bambam::ReadEnds RE;
										for ( uint64_t j = 0; j < blockread; ++j )
										{
											RE.get(dataSnappy);

											if ( ! j )
											{
												libmaus::bambam::ReadEndsBase const first = index.getBaseLevelBlockStart(b);
												assert ( first == RE );

												uint64_t bb = b;
												unsigned int level = 0;
												while ( 
													(bb & ((1ull << libmaus::bambam::ReadEndsContainerBase::innerIndexShift)-1)) == 0
													&&
													(++level < index.levelstarts.size())
												)
												{
													bb >>= libmaus::bambam::ReadEndsContainerBase::innerIndexShift;
													libmaus::bambam::ReadEndsBase const lfirst = index.getLevelBlockStart(level,bb);
													assert ( lfirst == RE );
													// std::cerr << "[b=" << b << ",level=" << level << "]";
												}
											}
										
											bool const ok = ( (! prevvalid) || (prev < RE) );
											
											if ( ! ok )
											{
												std::cerr << "prev=" << prev << std::endl;
												std::cerr << "RE=" << RE << std::endl;
											}
										
											assert ( ok );
										
											prev = RE;
											prevvalid = true;
										}								
									}
								}
							}
						}
						catch(std::exception const & ex)
						{
							globallock.lock();
							std::cerr << ex.what() << std::endl;
							globallock.unlock();
						}
					}
				
				}
				
				void flushReadEndsLists()
				{
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
					{
						STP.join();
						return;
					}

					#if 0
					{
						std::cerr << "Verifying frags...";
					
						libmaus::util::shared_ptr< std::vector< ::libmaus::bambam::ReadEndsBlockDecoderBaseCollectionInfoBase > >::type
							sfraginfo = getFragMergeInfo();
						std::vector< ::libmaus::bambam::ReadEndsBlockDecoderBaseCollectionInfoBase > const & fraginfo = *sfraginfo;

						verifyReadEndsFragments(fraginfo);
						
						std::cerr << "done." << std::endl;
					}

					{
						std::cerr << "Verifying pairs...";
					
						libmaus::util::shared_ptr< std::vector< ::libmaus::bambam::ReadEndsBlockDecoderBaseCollectionInfoBase > >::type
							spairinfo = getPairMergeInfo();
						std::vector< ::libmaus::bambam::ReadEndsBlockDecoderBaseCollectionInfoBase > const & pairinfo = *spairinfo;

						verifyReadEndsPairs(pairinfo);
						
						std::cerr << "done." << std::endl;
					}
					#endif
				}

				void mergeReadEndsLists(std::ostream & metricsstr, std::string const progname)
				{
					// set up duplicate data structure
					uint64_t const ureadsParsed = static_cast<uint64_t>(readsParsed);
					libmaus::bitio::BitVector::unique_ptr_type Tdupbitvec(new libmaus::bitio::BitVector(ureadsParsed));
					Pdupbitvec = UNIQUE_PTR_MOVE(Tdupbitvec);
					
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
					{
						STP.join();
						return;
					}

					libmaus::bambam::DupSetCallbackSharedVector dvec(*Pdupbitvec);
					std::cerr << "[V] num dups " << dvec.getNumDups() << std::endl;

					// print computed metrics
					::libmaus::bambam::DuplicationMetrics::printFormatHeader(progname,metricsstr);
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
					
					if ( STP.isInPanicMode() )
						STP.join();
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
				
				ChecksumsInterface::shared_ptr_type getSeqChecksumsObject()
				{
					ChecksumsInterface::shared_ptr_type tptr(checksums_free_list->get());
					return tptr;
				}
				
				void returnSeqChecksumsObject(ChecksumsInterface::shared_ptr_type obj)
				{
					checksums_free_list->put(obj);
				}
				
				ChecksumsInterface::shared_ptr_type getCombinedChecksums()
				{
					if ( ! combinedchecksums )
					{
						if ( checksums_free_list )
						{
							std::vector < ChecksumsInterface::shared_ptr_type > V = checksums_free_list->getAll();
							ChecksumsInterface::shared_ptr_type sum = checksums_free_list->getAllocator()();

							for ( uint64_t i = 0; i < V.size(); ++i )
								sum->update(*V[i]);
							
							combinedchecksums = sum;
							checksums_free_list->put(V);
						}
					}
					
					return combinedchecksums;
				}
				
				void printChecksums(std::ostream & out)
				{
					ChecksumsInterface::shared_ptr_type sum = getCombinedChecksums();
					if ( sum )
						sum->printChecksums(out);						
				}
				
				void printChecksumsForBamHeader(std::ostream & out)
				{
					ChecksumsInterface::shared_ptr_type sum = getCombinedChecksums();
					if ( sum )
						sum->printChecksumsForBamHeader(out);						
				}
				
				void bamHeaderComplete(libmaus::bambam::BamHeaderParserState const & BHPS)
				{
					if ( hash.size() )
					{
						checksums_free_list_pointer_type tfreelist(
							new checksums_free_list_type(
								libmaus::bambam::ChecksumsInterfaceAllocator(hash,parseInfo.Pheader.get())
							)
						);
						checksums_free_list = UNIQUE_PTR_MOVE(tfreelist);
					}
					
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
						(algnbuffer = parseBlockFreeList->getIf())
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
							std::cerr << "[V] " << readsParsed << "\t" << procrtc.formatTime(procrtc.getElapsedSeconds()) << "\t" << libmaus::util::MemUsage() <<"\t" << libmaus::autoarray::AutoArrayMemUsage() << (algn->final?"\tfinal":"") << std::endl;
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
						fragmentBufferFreeListPostSort->put(algn);
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
						fragmentBufferFreeListPreSort->put(FAB);
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
							FragmentAlignmentBuffer::shared_ptr_type outFAB = fragmentBufferFreeListPostSort->getIf();
							
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
						parseBlockFreeList->put(algn);
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
							FragmentAlignmentBuffer::shared_ptr_type FAB = fragmentBufferFreeListPreSort->getIf();
							
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
#endif
