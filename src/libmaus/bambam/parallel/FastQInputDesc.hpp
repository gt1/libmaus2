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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_FASTQINPUTDESC_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_FASTQINPUTDESC_HPP

#include <libmaus/bambam/parallel/FastQInputDescBase.hpp>
#include <libmaus/bambam/parallel/FastqToBamControlInputBlockHeapComparator.hpp>
#include <libmaus/bambam/parallel/FastqToBamControlSubReadPending.hpp>
#include <libmaus/bambam/parallel/FastqToBamControlSubReadPendingHeapComparator.hpp>
#include <libmaus/bambam/parallel/DecompressedBlockAllocator.hpp>
#include <libmaus/bambam/parallel/DecompressedBlockTypeInfo.hpp>
#include <libmaus/bambam/parallel/DecompressedBlockReorderHeapComparator.hpp>
#include <queue>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			struct FastQInputDesc : public FastQInputDescBase
			{
				std::istream & in;
				libmaus::parallel::PosixSpinLock inlock;
				free_list_type blockFreeList;
				uint64_t streamid;
				libmaus::autoarray::AutoArray<uint8_t,libmaus::autoarray::alloc_type_c> stallArray;
				uint64_t volatile stallArraySize;
				uint64_t volatile blockid;
				bool volatile eof;
				uint64_t volatile blocksproduced;
				libmaus::parallel::PosixSpinLock blocksproducedlock;
				uint64_t volatile blockspassed;
				libmaus::parallel::PosixSpinLock blockspassedlock;

				std::priority_queue<
					FastQInputDescBase::input_block_type::shared_ptr_type,
					std::vector<FastQInputDescBase::input_block_type::shared_ptr_type>,
					FastqToBamControlInputBlockHeapComparator
				> readpendingqueue;
				libmaus::parallel::PosixSpinLock readpendingqueuelock;
				uint64_t volatile readpendingqueuenext;

				std::priority_queue<
					FastqToBamControlSubReadPending,
					std::vector<FastqToBamControlSubReadPending>,
					FastqToBamControlSubReadPendingHeapComparator
				> readpendingsubqueue;
				libmaus::parallel::PosixSpinLock readpendingsubqueuelock;
				uint64_t volatile readpendingsubqueuenextid;
				
				libmaus::parallel::LockedFreeList<
					libmaus::bambam::parallel::DecompressedBlock,
					libmaus::bambam::parallel::DecompressedBlockAllocator,
					libmaus::bambam::parallel::DecompressedBlockTypeInfo
				> decompfreelist;
				
				std::priority_queue<
					libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type,
					std::vector<libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type>,
					DecompressedBlockReorderHeapComparator
				> parsereorderqueue;
				libmaus::parallel::PosixSpinLock parsereorderqueuelock;
				uint64_t volatile parsereorderqueuenext;
				
				FastQInputDesc(std::istream & rin, uint64_t const numblocks, uint64_t const blocksize, uint64_t const rstreamid)
				: in(rin), blockFreeList(numblocks,libmaus::bambam::parallel::GenericInputBlockAllocator<meta_type>(blocksize)),
				  streamid(rstreamid), stallArray(), stallArraySize(0), blockid(0), eof(false), blocksproduced(0), blockspassed(0),
				  readpendingqueue(), readpendingqueuelock(), readpendingqueuenext(0),
				  readpendingsubqueuenextid(0),
				  decompfreelist(32),
				  parsereorderqueue(),
				  parsereorderqueuelock(),
				  parsereorderqueuenext(0)
				{}
				
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
					blockid += 1;
				}
				
				bool getEOF() const
				{
					return eof;
				}
				
				void setEOF(bool const reof)
				{
					eof = reof;
				}
				
				uint64_t getBlocksProduced()
				{
					libmaus::parallel::ScopePosixSpinLock slock(blocksproducedlock);
					return blocksproduced;
				}
				
				void incrementBlocksProduced()
				{
					libmaus::parallel::ScopePosixSpinLock slock(blocksproducedlock);
					blocksproduced += 1;					
				}
				
				uint64_t getBlocksPassed()
				{					
					libmaus::parallel::ScopePosixSpinLock slock(blockspassedlock);
					return blockspassed;
				}

				void incrementBlocksPassed()
				{
					libmaus::parallel::ScopePosixSpinLock slock(blockspassedlock);
					blockspassed += 1;					
				}
			};
		}
	}
}
#endif
