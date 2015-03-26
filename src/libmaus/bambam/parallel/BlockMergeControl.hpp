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

#include <libmaus/bambam/parallel/BamBlockIndexingWorkPackageDispatcher.hpp>

#include <libmaus/bambam/parallel/AlignmentBufferAllocator.hpp>
#include <libmaus/bambam/parallel/AlignmentBufferTypeInfo.hpp>
#include <libmaus/bambam/parallel/GenericInputMergeWorkRequest.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferAllocator.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferTypeInfo.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferHeapComparator.hpp>
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
#include <libmaus/bambam/parallel/FileChecksumBlockWorkPackageDispatcher.hpp>
			
			
namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{						
			struct BlockMergeControl : 
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
				public libmaus::bambam::parallel::GenericInputControlBlockWritePackageBlockWrittenInterface,
				public ChecksumsInterfaceGetInterface,
				public ChecksumsInterfacePutInterface,
				public FileChecksumBlockFinishedInterface,
				public FileChecksumBlockWorkPackageReturnInterface,
				public BamBlockIndexingWorkPackageReturnInterface,
				public BamBlockIndexingBlockFinishedInterface
			{
				typedef BlockMergeControl this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

				libmaus::parallel::SimpleThreadPool & STP;
				
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
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<libmaus::bambam::parallel::GenericInputMergeWorkPackage> mergeWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<libmaus::bambam::parallel::GenericInputControlReorderWorkPackage> rewriteWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<libmaus::bambam::parallel::GenericInputControlBlockCompressionWorkPackage> compressWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<libmaus::bambam::parallel::GenericInputControlBlockWritePackage> writeWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList< libmaus::bambam::parallel::FileChecksumBlockWorkPackage > checksumWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList< libmaus::bambam::parallel::BamBlockIndexingWorkPackage > indexingWorkPackages;

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

				uint64_t const FCBWPDid;
				libmaus::bambam::parallel::FileChecksumBlockWorkPackageDispatcher FCBWPD;
				
				uint64_t const BBIWPDid;
				BamBlockIndexingWorkPackageDispatcher BBIWPD;
				
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
				
				libmaus::bambam::BamIndexGenerator bamindexgenerator;
				
				libmaus::digest::DigestInterface * filechecksum;

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
					libmaus::digest::DigestInterface * rfilechecksum
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
				  GICRWPDid(STP.getNextDispatcherId()), GICRWPD(*this,*this,*this,*this,BV),
				  GICBCWPDid(STP.getNextDispatcherId()), GICBCWPD(*this,*this,*this,*this),
				  GICBWPDid(STP.getNextDispatcherId()), GICBWPD(*this,*this),
				  FCBWPDid(STP.getNextDispatcherId()), FCBWPD(*this,*this),
				  BBIWPDid(STP.getNextDispatcherId()), BBIWPD(*this,*this),
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
				  bamindexgenerator(tempfileprefix+"_index",0 /* verbose */,false /* validate */,false /* debug */),
				  filechecksum(rfilechecksum)
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
					STP.registerDispatcher(FCBWPDid,&FCBWPD);
					STP.registerDispatcher(BBIWPDid,&BBIWPD);

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
				
				void bamBlockIndexingWorkPackageReturn(BamBlockIndexingWorkPackage * package) { indexingWorkPackages.returnPackage(package); }
				
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
							*package = BamBlockIndexingWorkPackage(0,BBIWPDid,GICCP,&bamindexgenerator);
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

				void checkRewriteReorderQueue()
				{
					bool final = false;
					
					libmaus::parallel::ScopePosixSpinLock slock(rewriteReorderQueueLock);
				
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

				template<typename output_stream_type>
				void writeBamIndex(output_stream_type & out)
				{
					bamindexgenerator.flush(out);
				}
				
				void writeBamIndex(std::string const & filename)
				{
					libmaus::aio::PosixFdOutputStream PFOS(filename);
					writeBamIndex(PFOS);
					PFOS.flush();
				}
			};
		}
	}
}
#endif
