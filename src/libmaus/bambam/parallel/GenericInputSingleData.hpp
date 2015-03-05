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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_GENERICINPUTSINGLEDATA_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_GENERICINPUTSINGLEDATA_HPP

#include <libmaus/bambam/parallel/GenericInputControlStreamInfo.hpp>
#include <libmaus/aio/InputStreamFactoryContainer.hpp>
#include <libmaus/bambam/parallel/GenericInputHeapComparator.hpp>
#include <libmaus/bambam/parallel/GenericInputControlSubBlockPendingHeapComparator.hpp>
#include <libmaus/bambam/parallel/DecompressedBlockAllocator.hpp>
#include <libmaus/bambam/parallel/DecompressedBlockTypeInfo.hpp>
#include <libmaus/bambam/parallel/DecompressedBlockHeapComparator.hpp>
#include <queue>
#include <libmaus/bambam/parallel/GenericInputSingleDataBamParseInfo.hpp>
#include <libmaus/bambam/parallel/SamParsePendingHeapComparator.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			struct GenericInputSingleDataReadBase
			{
				typedef GenericInputSingleDataReadBase this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
				
				size_t byteSize()
				{
					return
						streaminfo.byteSize() +
						sizeof(Pin) +
						sizeof(inlock) +
						sizeof(streamid) +
						sizeof(blockid) +
						sizeof(blocksize) +
						blockFreeList.byteSize() +
						sizeof(finite) +
						sizeof(dataleft) +
						sizeof(eof) +
						sizeof(eoflock) +
						stallArray.byteSize() +
						sizeof(stallArraySize) +
						sizeof(lock) +
						sizeof(nextblockid) +
						sizeof(decompressionpendingnext) +
						decompressiontotal.size() * sizeof(uint64_t) +
						meminputblockfreelist.byteSize() +
						decompressedblockfreelist.byteSize() +
						sizeof(decompressedBlockIdAcc) +
						sizeof(decompressedBlocksAcc) +
						sizeof(samHeaderComplete) +
						samHeader.byteSize() +
						samParsePendingQueue.size() * sizeof(SamParsePending) +
						sizeof(samParsePendingQueueLock) +
						sizeof(samParsePendingQueueNextAbsId);
				}
			
				public:
				// info
				libmaus::bambam::parallel::GenericInputControlStreamInfo const streaminfo;
				
				// input stream pointer
				libmaus::aio::InputStream::unique_ptr_type Pin;
				
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
				GenericInputBase::generic_input_stall_array_type stallArray;
				// number of bytes in stall array, protected by inlock
				uint64_t volatile stallArraySize;
			
				// lock
				libmaus::parallel::PosixSpinLock lock;
				
				// next block to be processed, protected by lock
				uint64_t volatile nextblockid;
				// pending queue, protected by lock
				std::priority_queue<
					GenericInputBase::generic_input_shared_block_ptr_type,
					std::vector<GenericInputBase::generic_input_shared_block_ptr_type>,
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
				
				bool volatile samHeaderComplete;
				libmaus::autoarray::AutoArray<char,libmaus::autoarray::alloc_type_c> samHeader;

				std::deque<SamParsePending> samParsePendingQueue;
				libmaus::parallel::PosixSpinLock samParsePendingQueueLock;
				uint64_t volatile samParsePendingQueueNextAbsId;
												
				GenericInputSingleDataReadBase(
					std::istream & rin,
					libmaus::bambam::parallel::GenericInputControlStreamInfo const & rstreaminfo,
					uint64_t const rstreamid,
					uint64_t const rblocksize, 
					unsigned int const numblocks,
					unsigned int const complistsize
				)
				:
				  streaminfo(rstreaminfo),
				  Pin(),
				  in(rin), 
				  streamid(rstreamid), blockid(0), blocksize(rblocksize), 
				  blockFreeList(numblocks,libmaus::bambam::parallel::GenericInputBlockAllocator<GenericInputBase::meta_type>(blocksize)),
				  finite(streaminfo.finite),
				  dataleft(finite ? (streaminfo.end-streaminfo.start) : 0),
				  eof(false),
				  stallArray(0),
				  stallArraySize(0),
				  nextblockid(0),
				  meminputblockfreelist(complistsize),
				  decompressedblockfreelist(complistsize),
				  decompressedBlockIdAcc(0),
				  decompressedBlocksAcc(0),
				  samHeaderComplete(false),
				  samHeader(0,false),
				  samParsePendingQueueNextAbsId(0)
				{
				
				}

				GenericInputSingleDataReadBase(
					libmaus::bambam::parallel::GenericInputControlStreamInfo const & rstreaminfo,
					uint64_t const rstreamid,
					uint64_t const rblocksize, 
					unsigned int const numblocks,
					unsigned int const complistsize
				)
				:
				  streaminfo(rstreaminfo),
				  Pin(streaminfo.openStream()),
				  in(*Pin), 
				  streamid(rstreamid), blockid(0), blocksize(rblocksize), 
				  blockFreeList(numblocks,libmaus::bambam::parallel::GenericInputBlockAllocator<GenericInputBase::meta_type>(blocksize)),
				  finite(streaminfo.finite),
				  dataleft(finite ? (streaminfo.end-streaminfo.start) : 0),
				  eof(false),
				  stallArray(0),
				  stallArraySize(0),
				  nextblockid(0),
				  meminputblockfreelist(complistsize),
				  decompressedblockfreelist(complistsize),
				  decompressedBlockIdAcc(0),
				  decompressedBlocksAcc(0)
				{
				
				}

				void samHeaderAdd(char const * c, size_t n)
				{
					size_t const o = samHeader.size();
					samHeader.resize(o+n);
					std::copy(c,c+n,samHeader.begin()+o);
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
		
			struct GenericInputSingleData : public GenericInputSingleDataReadBase
			{
				typedef GenericInputSingleData this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
				
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
					libmaus::bambam::parallel::GenericInputControlStreamInfo const & rstreaminfo,
					uint64_t const rstreamid,
					uint64_t const rblocksize, 
					unsigned int const numblocks,
					unsigned int const complistsize
				)
				: GenericInputSingleDataReadBase(rstreaminfo,rstreamid,rblocksize,numblocks,complistsize),
				  parsependingnext(0),
				  parsepending(),
				  parseInfo(!streaminfo.hasheader),
				  processQueue(),
				  processActive(),
				  processMissing(true),
				  processFirst(true),
				  processLock()
				{
				
				}
			};
		}
	}
}
#endif
