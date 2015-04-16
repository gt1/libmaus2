/*
    libmaus2
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
#if ! defined(LIBMAUS2_BAMBAM_PARALLEL_CRAMENCODINGSUPPORTDATA_HPP)
#define LIBMAUS2_BAMBAM_PARALLEL_CRAMENCODINGSUPPORTDATA_HPP

#include <libmaus2/parallel/PosixSpinLock.hpp>
#include <libmaus2/parallel/LockedGrowingFreeList.hpp>
#include <libmaus2/bambam/parallel/CramPassPointerObject.hpp>
#include <libmaus2/bambam/parallel/CramPassPointerObjectAllocator.hpp>
#include <libmaus2/bambam/parallel/CramPassPointerObjectTypeInfo.hpp>
#include <libmaus2/bambam/parallel/CramOutputBlock.hpp>
#include <libmaus2/bambam/parallel/CramOutputBlockIdComparator.hpp>
#include <set>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			struct CramEncodingSupportData
			{
				uint64_t volatile cramtokens;
				libmaus2::parallel::PosixSpinLock cramtokenslock;

				libmaus2::parallel::LockedGrowingFreeList<
					libmaus2::bambam::parallel::CramPassPointerObject,
					libmaus2::bambam::parallel::CramPassPointerObjectAllocator,
					libmaus2::bambam::parallel::CramPassPointerObjectTypeInfo>
					passPointerFreeList;

				std::map<uint64_t,CramPassPointerObject::shared_ptr_type> passPointerActive;
				libmaus2::parallel::PosixSpinLock passPointerActiveLock;
				
				void * context;

				std::multimap<size_t,CramOutputBlock::shared_ptr_type> outputBlockFreeList;
				libmaus2::parallel::PosixSpinLock outputBlockFreeListLock;
				
				std::set<CramOutputBlock::shared_ptr_type,CramOutputBlockIdComparator> outputBlockPendingList;
				libmaus2::parallel::PosixSpinLock outputBlockPendingListLock;

				uint64_t volatile outputBlockUnfinished;
				libmaus2::parallel::PosixSpinLock outputBlockUnfinishedLock;

				std::pair<int64_t volatile,uint64_t volatile> outputWriteNext;
				libmaus2::parallel::PosixSpinLock outputWriteNextLock;
												
				bool getCramEncodingToken()
				{
					bool ok = false;
					
					cramtokenslock.lock();
					if ( cramtokens )
					{
						ok = true;
						cramtokens -= 1;
					}
					cramtokenslock.unlock();
					
					return ok;
				}
				
				void putCramEncodingToken()
				{
					cramtokenslock.lock();
					cramtokens += 1;
					cramtokenslock.unlock();
				}

				CramEncodingSupportData(size_t const numtokens)
				: 
				  cramtokens(numtokens),
				  context(0),
				  outputWriteNext()
				{
					outputWriteNext.first = -1;
					outputWriteNext.second = 0;
				}
			};
		}
	}
}
#endif
