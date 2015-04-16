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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_ALIGNMENTREWRITEPOSSORTCONTEXT_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_ALIGNMENTREWRITEPOSSORTCONTEXT_HPP

#include <libmaus/bambam/parallel/AlignmentRewritePosSortContextBaseBlockSortedInterface.hpp>
#include <libmaus/bambam/parallel/AlignmentRewritePosSortContextMergePackageFinished.hpp>
#include <libmaus/bambam/parallel/AlignmentRewriteBuffer.hpp>
#include <libmaus/bambam/parallel/AlignmentRewritePosMergeSortPackage.hpp>
#include <libmaus/bambam/parallel/AlignmentRewritePosSortBaseSortPackage.hpp>
#include <libmaus/bambam/parallel/SortFinishedInterface.hpp>
#include <libmaus/sorting/ParallelStableSort.hpp>
#include <libmaus/parallel/SimpleThreadPoolInterfaceEnqueTermInterface.hpp>
#include <libmaus/parallel/SimpleThreadPoolWorkPackageFreeList.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			template<typename _order_type>
			struct AlignmentRewritePosSortContext : 
				public AlignmentRewritePosSortContextBaseBlockSortedInterface,
				public AlignmentRewritePosSortContextMergePackageFinished
			{
				typedef _order_type order_type;
				typedef AlignmentRewritePosSortContext<order_type> this_type;
				typedef typename libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef typename libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
				
				typedef AlignmentRewriteBuffer::pointer_type * iterator;
			
				AlignmentRewriteBuffer * const buffer;
				order_type comparator;
				libmaus::sorting::ParallelStableSort::ParallelSortControl<iterator,order_type> PSC;
				libmaus::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & STPI;
	
				typename libmaus::sorting::ParallelStableSort::MergeLevels<iterator,order_type>::level_type * volatile level;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<AlignmentRewritePosMergeSortPackage<order_type> > & mergeSortPackages;
				uint64_t const mergeSortDispatcherId;
				
				SortFinishedInterface & sortFinishedInterface;
				
				enum state_type
				{
					state_base_sort, state_merge_plan, state_merge_execute, state_copy_back, state_done
				};
				
				state_type volatile state;
				
				AlignmentRewritePosSortContext(
					AlignmentRewriteBuffer * rbuffer,
					uint64_t const numthreads,
					libmaus::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & rSTPI,
					libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<AlignmentRewritePosMergeSortPackage<order_type> > & rmergeSortPackages,
					uint64_t const rmergeSortDispatcherId,
					SortFinishedInterface & rsortFinishedInterface
				)
				: buffer(rbuffer), comparator(buffer), PSC(				
					buffer->getPointerArray().first,
					buffer->getPointerArray().first + buffer->fill(),
					buffer->getAuxPointerArray().first,
					buffer->getAuxPointerArray().first + buffer->fill(),
					comparator,
					numthreads,
					true /* copy back */), 
					STPI(rSTPI),
					level(PSC.mergeLevels.levels.size() ? &(PSC.mergeLevels.levels[0]) : 0),
					mergeSortPackages(rmergeSortPackages),
					mergeSortDispatcherId(rmergeSortDispatcherId),
					sortFinishedInterface(rsortFinishedInterface),
					state((buffer->fill() > 1) ? state_base_sort : state_done)
				{
				
				}
				
				void enqueBaseSortPackages(
					libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<AlignmentRewritePosSortBaseSortPackage<order_type> > & baseSortPackages,
					uint64_t const baseSortDispatcher
				)
				{
					for ( uint64_t i = 0; i < PSC.baseSortRequests.baseSortRequests.size(); ++i )
					{
						AlignmentRewritePosSortBaseSortPackage<order_type> * package = baseSortPackages.getPackage();
						*package = AlignmentRewritePosSortBaseSortPackage<order_type>(
							0 /* prio */,
							&(PSC.baseSortRequests.baseSortRequests[i]),
							this,
							baseSortDispatcher
						);
						
						STPI.enque(package);
					}
				}
								
				/* make sure result is in place of original data */
				void copyBack()
				{
					state = state_copy_back;
					
					if ( PSC.needCopyBack )
					{
						std::copy(PSC.context.in,PSC.context.in+PSC.context.n,PSC.context.out);
					}
					
					state = state_done;
	
					sortFinishedInterface.putSortFinished(buffer);
				}
				
				void planMerge()
				{
					state = state_merge_plan;
					assert ( level );
					level->dispatch();
					executeMerge();
				}
				
				void executeMerge()
				{
					state = state_merge_execute;
					
					assert ( level );
					for ( uint64_t i = 0; i < level->mergeRequests.size(); ++i )
					{
						AlignmentRewritePosMergeSortPackage<order_type> * package = mergeSortPackages.getPackage();
						*package = AlignmentRewritePosMergeSortPackage<order_type>(
							0 /* prio */,
							&(level->mergeRequests[i]),
							this,
							mergeSortDispatcherId
						);
						STPI.enque(package);
					}
				}
	
				virtual void baseBlockSorted()
				{
					if ( (PSC.baseSortRequests.requestsFinished.increment()) == PSC.baseSortRequests.baseSortRequests.size() )
					{
						if ( PSC.mergeLevels.levels.size() )
						{
							planMerge();
						}
						else
						{
							copyBack();
						}
					}
				}
	
				virtual void mergePackageFinished()
				{
					if ( ((level->requestsFinished)).increment() == level->mergeRequests.size() )
					{
						level = level->next;
						
						if ( level )
							planMerge();
						else
							copyBack();
					}
				}
			};
		}
	}
}
#endif
