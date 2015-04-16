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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_BLOCKMERGECONTROL_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_BLOCKMERGECONTROL_HPP

#include <libmaus/aio/PosixFdOutputStream.hpp>
#include <libmaus/bambam/ChecksumsInterfaceAllocator.hpp>
#include <libmaus/bambam/ChecksumsInterfaceTypeInfo.hpp>
#include <libmaus/bambam/parallel/AlignmentBufferAllocator.hpp>
#include <libmaus/bambam/parallel/AlignmentBufferTypeInfo.hpp>
#include <libmaus/bambam/parallel/BamBlockIndexingWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/GenericInputMergeWorkRequest.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferAllocator.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferTypeInfo.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferHeapComparator.hpp>
#include <libmaus/bambam/parallel/FileChecksumBlockWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/GenericInputControlReadWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/GenericInputBgzfDecompressionWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/GenericInputBamParseWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/GenericInputMergeWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/GenericInputControlReorderWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/GenericInputControlBlockCompressionWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/GenericInputControlBlockWritePackageDispatcher.hpp>
#include <libmaus/bambam/parallel/GenericInputControlCompressionPendingHeapComparator.hpp>
#include <libmaus/lz/BgzfDeflateOutputBufferBaseAllocator.hpp>
#include <libmaus/lz/BgzfDeflateOutputBufferBaseTypeInfo.hpp>
#include <libmaus/lz/BgzfDeflateZStreamBaseAllocator.hpp>
#include <libmaus/lz/BgzfDeflateZStreamBaseTypeInfo.hpp>
#include <libmaus/lz/BgzfDeflate.hpp>
#include <libmaus/parallel/SimpleThreadPool.hpp>
#include <libmaus/parallel/SimpleThreadPoolWorkPackageFreeList.hpp>
#include <libmaus/parallel/LockedGrowingFreeList.hpp>

#include <libmaus/bambam/parallel/CramOutputBlockWritePackageDispatcher.hpp>
#include <libmaus/bambam/parallel/SamEncoderObject.hpp>
#include <libmaus/bambam/parallel/CramOutputBlockIdComparator.hpp>
#include <libmaus/bambam/parallel/CramOutputBlockSizeComparator.hpp>

#include <libmaus/bambam/parallel/CramPassPointerObjectAllocator.hpp>
#include <libmaus/bambam/parallel/CramPassPointerObjectTypeInfo.hpp>
#include <libmaus/bambam/parallel/CramPassPointerObjectComparator.hpp>
#include <libmaus/bambam/parallel/ScramCramEncoding.hpp>

#include <libmaus/bambam/parallel/BlockMergeControlTypeBase.hpp>
#include <libmaus/bambam/parallel/CramEncodingSupportData.hpp>
#include <libmaus/bambam/parallel/CramEncodingWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/SamEncodingWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/CramEncodingWorkPackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/SamEncodingWorkPackageWrapperReturnInterface.hpp>
#include <libmaus/bambam/parallel/CramOutputBlockChecksumPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/CramOutputBlockChecksumComputedInterface.hpp>
#include <libmaus/bambam/parallel/CramOutputBlockChecksumPackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/CramOutputBlockChecksumPackage.hpp>
#include <libmaus/bambam/parallel/SamEncodingWorkPackageWrapper.hpp>
#include <libmaus/bambam/parallel/SamEncodingWorkPackage.hpp>
#include <libmaus/bambam/parallel/CramEncodingWorkPackage.hpp>
		
namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			template<typename _heap_element_type>
			struct BlockMergeControl : 
				public BlockMergeControlTypeBase,
				public libmaus::bambam::parallel::GenericInputControlReadWorkPackageReturnInterface,
				public libmaus::bambam::parallel::GenericInputControlReadAddPendingInterface,
				public libmaus::bambam::parallel::GenericInputBgzfDecompressionWorkPackageReturnInterface,
				public libmaus::bambam::parallel::GenericInputBgzfDecompressionWorkPackageMemInputBlockReturnInterface,
				public libmaus::bambam::parallel::GenericInputBgzfDecompressionWorkPackageDecompressedBlockReturnInterface,
				public libmaus::bambam::parallel::GenericInputBgzfDecompressionWorkSubBlockDecompressionFinishedInterface,
				public libmaus::bambam::parallel::GenericInputBamParseWorkPackageReturnInterface,
				public libmaus::bambam::parallel::GenericInputBamParseWorkPackageBlockParsedInterface,
				public libmaus::bambam::parallel::GenericInputMergeWorkPackageReturnInterface<_heap_element_type>,
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
				public libmaus::bambam::parallel::GenericInputControlBlockWritePackageBlockWrittenInterface,
				public ChecksumsInterfaceGetInterface,
				public ChecksumsInterfacePutInterface,
				public FileChecksumBlockFinishedInterface,
				public FileChecksumBlockWorkPackageReturnInterface,
				public BamBlockIndexingWorkPackageReturnInterface,
				public BamBlockIndexingBlockFinishedInterface,
				public SamEncodingWorkPackageWrapperReturnInterface,
				public CramOutputBlockWritePackageReturnInterface,
				public CramOutputBlockWritePackageFinishedInterface,
				public CramEncodingWorkPackageReturnInterface,
				public CramOutputBlockChecksumPackageReturnInterface,
				public CramOutputBlockChecksumComputedInterface
			{
				// GenericInputControlMergeHeapEntryCoordinate
				typedef _heap_element_type heap_element_type;
				typedef BlockMergeControl<heap_element_type> this_type;
				typedef typename libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef typename libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

				libmaus::parallel::SimpleThreadPool & STP;
				
				block_merge_output_format_t const block_merge_output_format;
				
				libmaus::bambam::parallel::ScramCramEncoding::unique_ptr_type PcramEncoder;
				
				std::ostream & out;
				
				libmaus::autoarray::AutoArray<char> & sheader;

				libmaus::bitio::BitVector * BV;
				
				libmaus::autoarray::AutoArray<libmaus::bambam::parallel::GenericInputSingleData::unique_ptr_type> data;

				libmaus::parallel::LockedFreeList<
					libmaus::lz::BgzfInflateZStreamBase,
					libmaus::lz::BgzfInflateZStreamBaseAllocator,
					libmaus::lz::BgzfInflateZStreamBaseTypeInfo
				> deccont;

				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<libmaus::bambam::parallel::GenericInputControlReadWorkPackage> readWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<libmaus::bambam::parallel::GenericInputBgzfDecompressionWorkPackage> decompressWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<libmaus::bambam::parallel::GenericInputBamParseWorkPackage> parseWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<libmaus::bambam::parallel::GenericInputMergeWorkPackage<heap_element_type> > mergeWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<libmaus::bambam::parallel::GenericInputControlReorderWorkPackage> rewriteWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<libmaus::bambam::parallel::GenericInputControlBlockCompressionWorkPackage> compressWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<libmaus::bambam::parallel::GenericInputControlBlockWritePackage> writeWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList< libmaus::bambam::parallel::FileChecksumBlockWorkPackage > checksumWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList< libmaus::bambam::parallel::BamBlockIndexingWorkPackage > indexingWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList< libmaus::bambam::parallel::SamEncodingWorkPackageWrapper > samEncodingWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList< libmaus::bambam::parallel::CramOutputBlockWritePackage > cramWriteWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList< libmaus::bambam::parallel::CramEncodingWorkPackage > cramEncodingWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList< libmaus::bambam::parallel::CramOutputBlockChecksumPackage > cramChecksumWorkPackages;

				uint64_t const GICRPDid;
				libmaus::bambam::parallel::GenericInputControlReadWorkPackageDispatcher GICRPD;
				
				uint64_t const GIBDWPDid;
				libmaus::bambam::parallel::GenericInputBgzfDecompressionWorkPackageDispatcher GIBDWPD;
				
				uint64_t const GIBPWPDid;
				libmaus::bambam::parallel::GenericInputBamParseWorkPackageDispatcher GIBPWPD;
				
				uint64_t const GIMWPDid;
				libmaus::bambam::parallel::GenericInputMergeWorkPackageDispatcher<heap_element_type> GIMWPD;
				
				uint64_t const GICRWPDid;
				libmaus::bambam::parallel::GenericInputControlReorderWorkPackageDispatcher GICRWPD;
				
				uint64_t const GICBCWPDid;
				libmaus::bambam::parallel::GenericInputControlBlockCompressionWorkPackageDispatcher GICBCWPD;
				
				uint64_t const GICBWPDid;
				libmaus::bambam::parallel::GenericInputControlBlockWritePackageDispatcher GICBWPD;

				uint64_t const FCBWPDid;
				libmaus::bambam::parallel::FileChecksumBlockWorkPackageDispatcher FCBWPD;
				
				uint64_t const BBIWPDid;
				BamBlockIndexingWorkPackageDispatcher BBIWPD;

				uint64_t const SEWPDid;
				SamEncodingWorkPackageDispatcher SEWPD;

				uint64_t const COBWPDid;
				CramOutputBlockWritePackageDispatcher COBWPD;

				uint64_t const CEWPDid;
				CramEncodingWorkPackageDispatcher CEWPD;

				uint64_t const COBCPDid;
				CramOutputBlockChecksumPackageDispatcher COBCPD;

				uint64_t volatile activedecompressionstreams;
				libmaus::parallel::PosixSpinLock activedecompressionstreamslock;
				
				uint64_t volatile streamParseUnstarted;
				libmaus::parallel::PosixSpinLock streamParseUnstartedLock;

				libmaus::util::FiniteSizeHeap<heap_element_type> mergeheap;
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

				typedef libmaus::parallel::LockedGrowingFreeList<
					libmaus::bambam::ChecksumsInterface,
					libmaus::bambam::ChecksumsInterfaceAllocator,
					libmaus::bambam::ChecksumsInterfaceTypeInfo
				> checksums_free_list_type;
				typedef checksums_free_list_type::unique_ptr_type checksums_free_list_pointer_type;
				checksums_free_list_pointer_type checksums_free_list;
				
				ChecksumsInterface::shared_ptr_type combinedchecksums;
				
				std::string const hash;
				
				libmaus::bambam::BamHeaderLowMem::unique_ptr_type checksumheader;

				std::map<uint64_t,uint64_t> outputBlocksUnfinishedTasks;
				libmaus::parallel::PosixSpinLock outputBlocksUnfinishedTasksLock;

				std::priority_queue<
					libmaus::bambam::parallel::GenericInputControlCompressionPending,
					std::vector<libmaus::bambam::parallel::GenericInputControlCompressionPending>,
					libmaus::bambam::parallel::GenericInputControlCompressionPendingHeapComparator
					> fileChksumQueue;
				uint64_t volatile fileChksumQueueNext;
				libmaus::parallel::PosixSpinLock fileChksumQueueLock;
								
				std::string const tempfileprefix;
				
				libmaus::bambam::BamIndexGenerator::unique_ptr_type Pbamindexgenerator;
				
				libmaus::digest::DigestInterface * filechecksum;

				CramEncodingSupportData samsupport;
				CramEncodingSupportData cramsupport;
				
				bool const bamindex;
				
				void enqueHeader()
				{
					switch ( block_merge_output_format )
					{
						case output_format_sam:
						{
							char const * headera = sheader.begin();
							char const * headere = sheader.end();
							std::istringstream istr(std::string(headera,headere));
							libmaus::bambam::BamHeaderLowMem::unique_ptr_type uheader(libmaus::bambam::BamHeaderLowMem::constructFromBAM(istr));
							std::pair<char const *, char const *> ptext = uheader->getText();
							std::string const text(ptext.first,ptext.second);
							samsupport.context = sam_allocate_encoder(this,ptext.first,ptext.second-ptext.first,sam_data_write_function);
							if ( ! samsupport.context )
							{
								libmaus::exception::LibMausException lme;
								lme.getStream() << "[E] Failed to allocate sam encoder" << std::endl;
								lme.finish();
								throw lme;
							}
							break;
						}
						case output_format_cram:
						{
							char const * headera = sheader.begin();
							char const * headere = sheader.end();
							std::istringstream istr(std::string(headera,headere));
							libmaus::bambam::BamHeaderLowMem::unique_ptr_type uheader(libmaus::bambam::BamHeaderLowMem::constructFromBAM(istr));
							std::pair<char const *, char const *> ptext = uheader->getText();
							std::string const text(ptext.first,ptext.second);
							cramsupport.context = cram_allocate_encoder(this,ptext.first,ptext.second-ptext.first,sam_data_write_function);
							if ( ! cramsupport.context )
							{
								libmaus::exception::LibMausException lme;
								lme.getStream() << "[E] Failed to allocate cram encoder" << std::endl;
								lme.finish();
								throw lme;
							}
							break;
						}
						case output_format_bam:
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
							break;
						}
						default:
						{
							libmaus::exception::LibMausException lme;
							lme.getStream() << "BlockMergeControl::enqueHeader: unknown output format" << std::endl;
							lme.finish();
							throw lme;						
						}
					}
				}
				
				static libmaus::bambam::parallel::ScramCramEncoding::unique_ptr_type allocateCramEncoder(block_merge_output_format_t const block_merge_output_format)
				{
					if ( block_merge_output_format == output_format_cram )
					{
						libmaus::bambam::parallel::ScramCramEncoding::unique_ptr_type TcramEncoder(new libmaus::bambam::parallel::ScramCramEncoding);
						return UNIQUE_PTR_MOVE(TcramEncoder);
					}
					else
					{
						libmaus::bambam::parallel::ScramCramEncoding::unique_ptr_type TcramEncoder;
						return UNIQUE_PTR_MOVE(TcramEncoder);
					}
				}

				void * cram_allocate_encoder(void *userdata, char const *header, size_t const headerlength, cram_data_write_function_t writefunc)
				{
					return PcramEncoder->cram_allocate_encoder(userdata,header,headerlength,writefunc);
				}
				
				void cram_deallocate_encoder(void * context)
				{
					PcramEncoder->cram_deallocate_encoder(context);
				}

				int cram_enque_compression_block(
					void *userdata,
					void *context,
					size_t const inblockid,
					char const **block,
					size_t const *blocksize,
					size_t const *blockelements,
					size_t const numblocks,
					int const final,
					cram_enque_compression_work_package_function_t workenqueuefunction,
					cram_data_write_function_t writefunction,
					cram_compression_work_package_finished_t workfinishedfunction
				)
				{				
					return PcramEncoder->cram_enque_compression_block(userdata,context,inblockid,block,blocksize,blockelements,numblocks,final,workenqueuefunction,writefunction,workfinishedfunction);
				}
				
				int cram_process_work_package(void *workpackage)
				{
					return PcramEncoder->cram_process_work_package(workpackage);					
				}

				BlockMergeControl(
					libmaus::parallel::SimpleThreadPool & rSTP,
					std::ostream & rout,
					libmaus::autoarray::AutoArray<char> & rsheader,
					std::vector<libmaus::bambam::parallel::GenericInputControlStreamInfo> const & in, 
					libmaus::bitio::BitVector * rBV,
					int const level,
					uint64_t const blocksize, // read block size
					uint64_t const numblocks, // number of read blocks per channel
					uint64_t const ralgnbuffersize, // merge alignment buffer size
					uint64_t const rnumalgnbuffers, // number of merge alignment buffers
					uint64_t const complistsize,
					std::string const & rhash,
					std::string const & rtempfileprefix,
					libmaus::digest::DigestInterface * rfilechecksum,
					block_merge_output_format_t const rblock_merge_output_format,
					bool const rbamindex,
					bool const rcomputerefidintervals
				)
				: STP(rSTP), 
				  block_merge_output_format(rblock_merge_output_format),
				  PcramEncoder(allocateCramEncoder(block_merge_output_format)),
				  out(rout),
				  sheader(rsheader),
				  BV(rBV),
				  data(in.size()), deccont(STP.getNumThreads()), 
				  GICRPDid(STP.getNextDispatcherId()), GICRPD(*this,*this),
				  GIBDWPDid(STP.getNextDispatcherId()), GIBDWPD(*this,*this,*this,*this,deccont),
				  GIBPWPDid(STP.getNextDispatcherId()), GIBPWPD(*this,*this),
				  GIMWPDid(STP.getNextDispatcherId()), GIMWPD(*this,*this,*this,*this,*this),
				  GICRWPDid(STP.getNextDispatcherId()), GICRWPD(*this,*this,*this,*this,BV,rcomputerefidintervals),
				  GICBCWPDid(STP.getNextDispatcherId()), GICBCWPD(*this,*this,*this,*this),
				  GICBWPDid(STP.getNextDispatcherId()), GICBWPD(*this,*this),
				  FCBWPDid(STP.getNextDispatcherId()), FCBWPD(*this,*this),
				  BBIWPDid(STP.getNextDispatcherId()), BBIWPD(*this,*this),
				  SEWPDid(STP.getNextDispatcherId()), SEWPD(*this),
				  COBWPDid(STP.getNextDispatcherId()), COBWPD(*this,*this),
				  CEWPDid(STP.getNextDispatcherId()), CEWPD(*this),
				  COBCPDid(STP.getNextDispatcherId()), COBCPD(*this,*this),
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
				  lastBlockWritten(false), lastBlockWrittenLock(),
				  hash(rhash),
				  fileChksumQueueNext(0),
				  tempfileprefix(rtempfileprefix),
				  Pbamindexgenerator(
				  	rbamindex ? new libmaus::bambam::BamIndexGenerator(tempfileprefix+"_index",0 /* verbose */,false /* validate */,false /* debug */) : 0
				  ),
				  filechecksum(rfilechecksum),
				  samsupport(STP.getNumThreads()),
				  cramsupport(STP.getNumThreads()),
				  bamindex(rbamindex)
				{
					for ( std::vector<libmaus::bambam::parallel::GenericInputControlStreamInfo>::size_type i = 0; i < in.size(); ++i )
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
					STP.registerDispatcher(FCBWPDid,&FCBWPD);
					STP.registerDispatcher(BBIWPDid,&BBIWPD);
					STP.registerDispatcher(SEWPDid,&SEWPD);
					STP.registerDispatcher(COBWPDid,&COBWPD);
					STP.registerDispatcher(CEWPDid,&CEWPD);
					STP.registerDispatcher(COBCPDid,&COBCPD);

					std::string headertext(sheader.begin(),sheader.end());
					std::istringstream headerin(headertext);
					libmaus::bambam::BamHeaderLowMem::unique_ptr_type theader(libmaus::bambam::BamHeaderLowMem::constructFromBAM(headerin));
					checksumheader = UNIQUE_PTR_MOVE(theader);

					if ( hash.size() )
					{
						checksums_free_list_pointer_type tfreelist(
							new checksums_free_list_type(
								libmaus::bambam::ChecksumsInterfaceAllocator(hash,checksumheader.get())
							)
						);
						checksums_free_list = UNIQUE_PTR_MOVE(tfreelist);
					}

					// put BAM header in compression queue
					enqueHeader();
					
					filechecksum->vinit();
				}
				
				std::string getFileDigest()
				{
					return filechecksum->vdigestAsString();
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

				ChecksumsInterface::shared_ptr_type getSeqChecksumsObject()
				{
					ChecksumsInterface::shared_ptr_type tptr(checksums_free_list->get());
					return tptr;
				}
				
				void returnSeqChecksumsObject(ChecksumsInterface::shared_ptr_type obj)
				{
					checksums_free_list->put(obj);
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
				
				void fileChecksumBlockWorkPackageReturn(FileChecksumBlockWorkPackage * package)
				{
					checksumWorkPackages.returnPackage(package);
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

				void genericInputMergeWorkPackageReturn(libmaus::bambam::parallel::GenericInputMergeWorkPackage<heap_element_type> * package)
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
				
				void bamBlockIndexingWorkPackageReturn(BamBlockIndexingWorkPackage * package) { indexingWorkPackages.returnPackage(package); }

				void samEncodingWorkPackageWrapperReturn(SamEncodingWorkPackageWrapper * package)
				{
					samEncodingWorkPackages.returnPackage(package);
				}

				void cramEncodingWorkPackageReturn(CramEncodingWorkPackage * package)
				{
					cramEncodingWorkPackages.returnPackage(package);
				}

				void cramOutputBlockWritePackageReturn(CramOutputBlockWritePackage * package)
				{
					cramWriteWorkPackages.returnPackage(package);
				}

				void cramOutputBlockChecksumPackageReturn(CramOutputBlockChecksumPackage * package)
				{
					cramChecksumWorkPackages.returnPackage(package);
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

						{
							libmaus::parallel::ScopePosixSpinLock llock(outputBlocksUnfinishedTasksLock);
							// we enque three work packages below, return block when all of them have been processed
							outputBlocksUnfinishedTasks[GICCP.absid] = 3;
						}

						{
							BamBlockIndexingWorkPackage * package = indexingWorkPackages.getPackage();
							*package = BamBlockIndexingWorkPackage(0,BBIWPDid,GICCP,Pbamindexgenerator.get());
							STP.enque(package);
						}

						{						
							libmaus::bambam::parallel::GenericInputControlBlockWritePackage * package = writeWorkPackages.getPackage();
							*package = libmaus::bambam::parallel::GenericInputControlBlockWritePackage(0/*prio*/, GICBWPDid, GICCP, &out);
							STP.enque(package);
						}

						{
							FileChecksumBlockWorkPackage * package = checksumWorkPackages.getPackage();
							*package = FileChecksumBlockWorkPackage(0 /* prio */, FCBWPDid, GICCP, filechecksum);
							STP.enque(package);
						}
					}
				}
				
				void returnCompressedBlock(libmaus::bambam::parallel::GenericInputControlCompressionPending GICCP)
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
						// std::cerr << "last block written." << std::endl;
						libmaus::parallel::ScopePosixSpinLock slock(lastBlockWrittenLock);
						lastBlockWritten = true;
					}				
				}

				void bamBlockIndexingBlockFinished(libmaus::bambam::parallel::GenericInputControlCompressionPending GICCP)
				{
					// we are now finished with the uncompressed data for this chunk, check whether we can return the block
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
					
					bool finished = false;
					
					// reduce number of references to this block by one
					{
						libmaus::parallel::ScopePosixSpinLock llock(outputBlocksUnfinishedTasksLock);
						assert ( outputBlocksUnfinishedTasks.find(GICCP.absid) != outputBlocksUnfinishedTasks.end() );
						assert ( outputBlocksUnfinishedTasks.find(GICCP.absid)->second > 0 );						
						
						if ( -- outputBlocksUnfinishedTasks[GICCP.absid] == 0 )
							finished = true;
					}
					
					// check whether block is finished now	
					if ( finished )
						returnCompressedBlock(GICCP);
				}

				void fileChecksumBlockFinished(libmaus::bambam::parallel::GenericInputControlCompressionPending GICCP)
				{
					bool finished = false;
					
					// reduce number of references to this block by one
					{
						libmaus::parallel::ScopePosixSpinLock llock(outputBlocksUnfinishedTasksLock);
						assert ( outputBlocksUnfinishedTasks.find(GICCP.absid) != outputBlocksUnfinishedTasks.end() );
						assert ( outputBlocksUnfinishedTasks.find(GICCP.absid)->second > 0 );						
						
						if ( -- outputBlocksUnfinishedTasks[GICCP.absid] == 0 )
							finished = true;
					}
					
					// check whether block is finished now	
					if ( finished )
						returnCompressedBlock(GICCP);				
				}

				void genericInputControlBlockWritePackageBlockWritten(libmaus::bambam::parallel::GenericInputControlCompressionPending GICCP)
				{
					bool finished = false;
					
					// reduce number of references to this block by one
					{
						libmaus::parallel::ScopePosixSpinLock llock(outputBlocksUnfinishedTasksLock);
						assert ( outputBlocksUnfinishedTasks.find(GICCP.absid) != outputBlocksUnfinishedTasks.end() );
						assert ( outputBlocksUnfinishedTasks.find(GICCP.absid)->second > 0 );						
						
						if ( -- outputBlocksUnfinishedTasks[GICCP.absid] == 0 )
							finished = true;
					}
					
					// check whether block is finished now	
					if ( finished )
						returnCompressedBlock(GICCP);
				}

				void genericInputControlBlockCompressionFinished(libmaus::bambam::parallel::GenericInputControlCompressionPending GICCP)
				{
					// enque block for writing
					{
						libmaus::parallel::ScopePosixSpinLock slock(compressedBlockWriteQueueLock);
						compressedBlockWriteQueue.push(GICCP);
					}

					#if 0					
					// block finished ZZZ
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
					#endif

					// check write queue
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
				
				void sam_enque_compression_work_package_function(void *workpackage)
				{				
					SamEncodingWorkPackageWrapper * package = samEncodingWorkPackages.getPackage();
					*package = SamEncodingWorkPackageWrapper(0/*prio*/,SEWPDid,reinterpret_cast<SamEncodingWorkPackage *>(workpackage));
					STP.enque(package);
				}

				void cram_enque_compression_work_package_function(void *workpackage)
				{				
					CramEncodingWorkPackage * package = cramEncodingWorkPackages.getPackage();
					*package = CramEncodingWorkPackage(0/*prio*/,CEWPDid,workpackage,PcramEncoder.get());
					STP.enque(package);
				}

				void sam_compression_work_package_finished_function(
					size_t const inblockid, 
					int const /* final */
				)
				{
					CramEncodingSupportData & supportdata = (block_merge_output_format==output_format_sam) ? samsupport : cramsupport;
					
					libmaus::bambam::parallel::CramPassPointerObject::shared_ptr_type passpointer;
					
					// get pass pointer object
					{
						libmaus::parallel::ScopePosixSpinLock slock(supportdata.passPointerActiveLock);
						
						std::map<uint64_t,CramPassPointerObject::shared_ptr_type>::iterator it = supportdata.passPointerActive.find(inblockid);
						assert ( it != supportdata.passPointerActive.end() );
						
						passpointer = it->second;
						supportdata.passPointerActive.erase(it);
					}

					// return block
					rewriteBlockFreeList.put(passpointer->block);
					checkRewritePendingQueue();
				}

				void cramOutputBlockWriteFinished(CramOutputBlock::shared_ptr_type block)
				{				
					CramEncodingSupportData & supportdata = (block_merge_output_format==output_format_sam) ? samsupport : cramsupport;

					bool const blockfinal = 
						(block->blocktype == cram_data_write_block_type_block_final) || 
						(block->blocktype == cram_data_write_block_type_file_final);
					bool const filefinal = block->blocktype == cram_data_write_block_type_file_final;
				
					// put block in free list
					{
						libmaus::parallel::ScopePosixSpinLock slock(supportdata.outputBlockFreeListLock);
						supportdata.outputBlockFreeList.insert(std::pair<size_t,CramOutputBlock::shared_ptr_type>(block->size(),block));
					}
					
					// increment write index
					{					
						libmaus::parallel::PosixSpinLock slock(supportdata.outputWriteNextLock);
						
						if ( blockfinal )
						{
							supportdata.outputWriteNext.first += 1;
							supportdata.outputWriteNext.second = 0;
						}
						else
						{
							supportdata.outputWriteNext.second += 1;
						}
					}
					
					// check for next package
					samCheckWritePendingQueue();

					// block is fully written, return the compress token
					if ( blockfinal )
					{
						supportdata.putCramEncodingToken();
						checkRewriteReorderQueue();
					}
					// file writing finished?
					if ( filefinal )
					{
						libmaus::parallel::ScopePosixSpinLock slock(lastBlockWrittenLock);
						lastBlockWritten = true;
						
						if ( block_merge_output_format==output_format_sam )
							sam_deallocate_encoder(samsupport.context);
						else if ( block_merge_output_format==output_format_cram )
							cram_deallocate_encoder(cramsupport.context);
					}
				}
				
				void cramOutputBlockDecrementUsedCounter(CramOutputBlock::shared_ptr_type block)
				{
					CramEncodingSupportData & supportdata = (block_merge_output_format==output_format_sam) ? samsupport : cramsupport;
					bool blockfinished = false;

					{
						libmaus::parallel::ScopePosixSpinLock slock(supportdata.outputBlockUnfinishedLock);
						assert ( supportdata.outputBlockUnfinished );
						
						blockfinished = (--(supportdata.outputBlockUnfinished) == 0);
					}

					if ( blockfinished )
						cramOutputBlockWriteFinished(block);					
				
				}
				
				void cramOutputBlockChecksumComputed(CramOutputBlock::shared_ptr_type block)
				{
					cramOutputBlockDecrementUsedCounter(block);
				}

				void cramOutputBlockWritePackageFinished(CramOutputBlock::shared_ptr_type block)
				{
					cramOutputBlockDecrementUsedCounter(block);
				}
				
				void samCheckWritePendingQueue()
				{
					CramEncodingSupportData & supportdata = (block_merge_output_format==output_format_sam) ? samsupport : cramsupport;

					CramOutputBlock::shared_ptr_type block;
				
					{
						libmaus::parallel::PosixSpinLock slock(supportdata.outputWriteNextLock);
						libmaus::parallel::ScopePosixSpinLock llock(supportdata.outputBlockPendingListLock);

						std::set<CramOutputBlock::shared_ptr_type,CramOutputBlockIdComparator>::iterator ita =
							supportdata.outputBlockPendingList.begin();
						
						if ( 
							ita != supportdata.outputBlockPendingList.end() &&
							(*ita)->blockid == supportdata.outputWriteNext.first &&
							(*ita)->subblockid == supportdata.outputWriteNext.second
						)
						{
							block = *ita;
							supportdata.outputBlockPendingList.erase(ita);
						}		
					}

					if ( block )
					{
						{
							libmaus::parallel::ScopePosixSpinLock slock(supportdata.outputBlockUnfinishedLock);
							supportdata.outputBlockUnfinished = 2;
						}
					
						CramOutputBlockWritePackage * package = cramWriteWorkPackages.getPackage();
						*package = CramOutputBlockWritePackage(0/*prio*/, COBWPDid, block, &out);
						STP.enque(package);

						CramOutputBlockChecksumPackage * cpackage = cramChecksumWorkPackages.getPackage();
						*cpackage = CramOutputBlockChecksumPackage(0/*prio*/, COBCPDid, block, filechecksum);
						STP.enque(cpackage);
					}
				}

				void sam_data_write_function(ssize_t const inblockid, size_t const outblockid, char const *data, size_t const n,
					cram_data_write_block_type const blocktype
				)
				{
					CramEncodingSupportData & supportdata = (block_merge_output_format==output_format_sam) ? samsupport : cramsupport;
					
					CramOutputBlock::shared_ptr_type outblock;

					{
						libmaus::parallel::ScopePosixSpinLock slock(supportdata.outputBlockFreeListLock);
					
						// try to find a block with a least size n	
						std::multimap<size_t,CramOutputBlock::shared_ptr_type>::iterator ita = supportdata.outputBlockFreeList.lower_bound(n);
						
						// got one?
						if ( ita != supportdata.outputBlockFreeList.end() )
						{
							// sanity check
							assert ( ita->first >= n );
							outblock = ita->second;
							supportdata.outputBlockFreeList.erase(ita);
						}
						// none present
						else
						{
							// is there any block in the free list?
							if ( supportdata.outputBlockFreeList.size() )
							{
								// get largest one
								std::multimap<size_t,CramOutputBlock::shared_ptr_type>::reverse_iterator rita = supportdata.outputBlockFreeList.rbegin();
								// get size
								size_t const size = rita->first;
								// get block
								outblock = rita->second;
								// erase block from free list
								supportdata.outputBlockFreeList.erase(supportdata.outputBlockFreeList.find(size));
							}
							// nothing in the free list
							else
							{
								// create a new block
								CramOutputBlock::shared_ptr_type tptr(new CramOutputBlock);
								outblock = tptr;
							}
						}
					}

					assert ( outblock );

					// copy the data
					outblock->set(data,n,inblockid,outblockid,
						blocktype
						// blockfinal,filefinal
					);

					// enque the block
					{
						libmaus::parallel::ScopePosixSpinLock slock(supportdata.outputBlockPendingListLock);
						supportdata.outputBlockPendingList.insert(outblock);
					}
					
					samCheckWritePendingQueue();
				}

				static void *sam_allocate_encoder(
					void *userdata,
					char const *sam_header,
					size_t const sam_headerlength,
					cram_data_write_function_t write_func
				)
				{
					libmaus::bambam::BamHeaderLowMem::unique_ptr_type Pheader(
						libmaus::bambam::BamHeaderLowMem::constructFromText(sam_header,sam_header+sam_headerlength)
					);
					
					SamEncoderObject * encoderobject = new SamEncoderObject(Pheader);

					write_func(
						userdata,
						-1 /* header id */,
						0  /* out block id */,
						sam_header /* data */,
						sam_headerlength /* length of data */,
						cram_data_write_block_type_block_final
					);
					
					return encoderobject;
				}

				static void sam_deallocate_encoder(void *context)
				{
					SamEncoderObject * encoderobject = reinterpret_cast<SamEncoderObject *>(context);
					delete encoderobject;
				}

				static void sam_enque_compression_work_package_function(void *userdata, void *workpackage)
				{
					this_type * t = reinterpret_cast<this_type *>(userdata);
					t->sam_enque_compression_work_package_function(workpackage);
				}

				static void cram_enque_compression_work_package_function(void *userdata, void *workpackage)
				{
					this_type * t = reinterpret_cast<this_type *>(userdata);
					t->cram_enque_compression_work_package_function(workpackage);
				}
				
				static void sam_compression_work_package_finished_function(void * userdata, size_t const inblockid, int const final)
				{
					this_type * t = reinterpret_cast<this_type *>(userdata);
					t->sam_compression_work_package_finished_function(inblockid,final);
				}

				static void sam_data_write_function(void *userdata, ssize_t const inblockid, size_t const outblockid, char const *data, size_t const n,
					cram_data_write_block_type const blocktype)
				{
					this_type * t = reinterpret_cast<this_type *>(userdata);
					t->sam_data_write_function(inblockid,outblockid,data,n,blocktype);
				}

				static int sam_enque_compression_block(
					void *userdata,
					void *context,
					size_t const inblockid,
					char const **block,
					size_t const *blocksize,
					size_t const *blockelements,
					size_t const numblocks,
					int const final,
					cram_enque_compression_work_package_function_t workenqueuefunction,
					cram_data_write_function_t writefunction,
					cram_compression_work_package_finished_t workfinishedfunction
				)
				{
					SamEncodingWorkPackage * package = (SamEncodingWorkPackage *)malloc(sizeof(SamEncodingWorkPackage));
					
					if ( ! package )
						return -1;

					package->userdata = userdata;
					package->context = context;
					package->inblockid = inblockid;
					package->block = block;
					package->blocksize = blocksize;
					package->blockelements = blockelements;
					package->numblocks = numblocks;
					package->final = final;
					package->writefunction = writefunction;
					package->workfinishedfunction = workfinishedfunction;
					
					workenqueuefunction(userdata,package);
	
					return 0;
				}

				void checkRewriteReorderQueue()
				{
					bool final = false;
					
					libmaus::parallel::ScopePosixSpinLock slock(rewriteReorderQueueLock);
					
					switch ( block_merge_output_format )
					{
						case output_format_sam:
					        {
							CramEncodingSupportData & supportdata = samsupport;
							
							while ( 
								rewriteReorderQueue.size() && rewriteReorderQueue.top()->id == rewriteReorderNext && 
								supportdata.getCramEncodingToken()
							)
							{
								// get block
								libmaus::bambam::parallel::FragmentAlignmentBuffer::shared_ptr_type block = rewriteReorderQueue.top();
								rewriteReorderQueue.pop();

								// get pass pointer object
								libmaus::bambam::parallel::CramPassPointerObject::shared_ptr_type passPointerObject =
									supportdata.passPointerFreeList.get();
								passPointerObject->set(block);

								// mark it as active
								{
									libmaus::parallel::ScopePosixSpinLock slock(supportdata.passPointerActiveLock);
									supportdata.passPointerActive[block->id] = passPointerObject;
								}

								// call work package enque
								int const r = sam_enque_compression_block(this,
									supportdata.context /* context */,
									passPointerObject->block->id,
									passPointerObject->D->begin(),
									passPointerObject->S->begin(),
									passPointerObject->L->begin(),
									passPointerObject->numblocks,
									passPointerObject->block->final,
									sam_enque_compression_work_package_function,
									sam_data_write_function,
									sam_compression_work_package_finished_function
								);
								
								if ( r < 0 )
								{
									libmaus::exception::LibMausException lme;
									lme.getStream() << "Failed to enque SAM encoding package for block " << passPointerObject->block->id << "\n";
									lme.finish();
									throw lme;
								}
			
								if ( block->final )
									final = true;

								rewriteReorderNext += 1;
							}

							break;
					        }                                                                                                                                                                        
						case output_format_cram:
					        {
							CramEncodingSupportData & supportdata = cramsupport;
							
							while ( 
								rewriteReorderQueue.size() && rewriteReorderQueue.top()->id == rewriteReorderNext && 
								supportdata.getCramEncodingToken()
							)
							{
								// get block
								libmaus::bambam::parallel::FragmentAlignmentBuffer::shared_ptr_type block = rewriteReorderQueue.top();
								rewriteReorderQueue.pop();

								// get pass pointer object
								libmaus::bambam::parallel::CramPassPointerObject::shared_ptr_type passPointerObject =
									supportdata.passPointerFreeList.get();
								passPointerObject->set(block);

								// mark it as active
								{
									libmaus::parallel::ScopePosixSpinLock slock(supportdata.passPointerActiveLock);
									supportdata.passPointerActive[block->id] = passPointerObject;
								}

								int const r = cram_enque_compression_block(
									this,
									supportdata.context,
									passPointerObject->block->id,
									passPointerObject->D->begin(),
									passPointerObject->S->begin(),
									passPointerObject->L->begin(),
									passPointerObject->numblocks,
									passPointerObject->block->final,
									cram_enque_compression_work_package_function,
									sam_data_write_function,
									sam_compression_work_package_finished_function                                                                                                        					
								);

								if ( r < 0 )
								{
									libmaus::exception::LibMausException lme;
									lme.getStream() << "Failed to enque CRAM encoding package for block " << passPointerObject->block->id << "\n";
									lme.finish();
									throw lme;
								}
			
								if ( block->final )
									final = true;

								rewriteReorderNext += 1;
							}

							break;
					        }                                                                                                                                                                        
						case output_format_bam:
					        {
							while ( rewriteReorderQueue.size() && rewriteReorderQueue.top()->id == rewriteReorderNext )
							{
								// block to be compressed
								libmaus::bambam::parallel::FragmentAlignmentBuffer::shared_ptr_type block = rewriteReorderQueue.top();
								rewriteReorderQueue.pop();

								// get linear fragments
								std::vector<std::pair<uint8_t *,uint8_t *> > V;
								block->getLinearOutputFragments(libmaus::lz::BgzfConstants::getBgzfMaxBlockSize(),V);
								
								// std::cerr << "finished rewrite for block " << block->id << "\t" << block->getFill() << "\t" << V.size() << std::endl;
								
								bool const isfinal = block->final;

								{
									libmaus::parallel::ScopePosixSpinLock rlock(compressionUnfinishedLock);
									compressionUnfinished[block->id] = V.size();
								}

								{
									libmaus::parallel::ScopePosixSpinLock rlock(compressionActiveLock);
									compressionActive[block->id] = block;
								}

								{
									libmaus::parallel::ScopePosixSpinLock rlock(compressionPendingLock);
									for ( uint64_t i = 0; i < V.size(); ++i )
									{
										compressionPending.push_back(
											libmaus::bambam::parallel::GenericInputControlCompressionPending(
												block->id,
												i,
												compressionPendingAbsIdNext++,
												false, // final
												V[i]
											)
										);
									}
								}
								
								if ( isfinal )
								{
									// counter for last block
									{
										libmaus::parallel::ScopePosixSpinLock rlock(compressionUnfinishedLock);
										compressionUnfinished[block->id+1] = 1;
									}

									// enque empty last block
									{
										libmaus::parallel::ScopePosixSpinLock rlock(compressionPendingLock);
										uint8_t * np = NULL;
										compressionPending.push_back(
											libmaus::bambam::parallel::GenericInputControlCompressionPending(
												block->id+1,
												0,
												compressionPendingAbsIdNext++,
												true, /* final */
												std::pair<uint8_t *,uint8_t *>(np,np)
											)
										);
									}

									final = true;
								}
								
								rewriteReorderNext += 1;

								#if 0
								rewriteBlockFreeList.put(block);
								checkRewritePendingQueue();
								#endif
							}

							checkCompressionPending();
							
							break;
						}
						default:
						{
							libmaus::exception::LibMausException lme;
							lme.getStream() << "BlockMergeControl::checkRewriteReorderQueue: unknown output format" << std::endl;
							lme.finish();
							throw lme;
							break;
						}
					}

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
							
								libmaus::bambam::parallel::GenericInputMergeWorkPackage<heap_element_type> * package = mergeWorkPackages.getPackage();
								*package = libmaus::bambam::parallel::GenericInputMergeWorkPackage<heap_element_type>(0/*prio*/,GIMWPDid,&data,&mergeheap,buffer);
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
								// heap_element_type GICMHE();
								libmaus::parallel::ScopePosixSpinLock slock(mergeheaplock);
								mergeheap.pushset(data[streamid]->processActive.get());
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

				template<typename output_stream_type>
				void writeBamIndex(output_stream_type & out)
				{
					Pbamindexgenerator->flush(out);
				}
				
				void writeBamIndex(std::string const & filename)
				{
					if ( Pbamindexgenerator )
					{
						libmaus::aio::PosixFdOutputStream PFOS(filename);
						writeBamIndex(PFOS);
						PFOS.flush();
					}
				}
			};
		}
	}
}
#endif
