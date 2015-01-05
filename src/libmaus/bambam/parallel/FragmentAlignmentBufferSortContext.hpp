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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_FRAGMENTALIGNMENTBUFFERSORTCONTEXT_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_FRAGMENTALIGNMENTBUFFERSORTCONTEXT_HPP

#include <libmaus/bambam/parallel/FragmentAlignmentBufferSortContextBaseBlockSortedInterface.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferSortContextMergePackageFinished.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferBaseSortPackage.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferSortFinishedInterface.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBuffer.hpp>
#include <libmaus/sorting/ParallelStableSort.hpp>
#include <libmaus/parallel/SimpleThreadPoolInterfaceEnqueTermInterface.hpp>
#include <libmaus/parallel/SimpleThreadPoolWorkPackageFreeList.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferMergeSortWorkPackage.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			template<typename _order_type>
			struct FragmentAlignmentBufferSortContext : 
				public FragmentAlignmentBufferSortContextBaseBlockSortedInterface,
				public FragmentAlignmentBufferSortContextMergePackageFinished
			{
				typedef _order_type order_type;
				typedef FragmentAlignmentBufferSortContext<order_type> this_type;
				typedef typename libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef typename libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
				
				typedef uint8_t ** iterator;
			
				FragmentAlignmentBuffer::shared_ptr_type buffer;
				order_type comparator;
				libmaus::sorting::ParallelStableSort::ParallelSortControl<iterator,order_type> PSC;
				libmaus::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & STPI;
	
				typename libmaus::sorting::ParallelStableSort::MergeLevels<iterator,order_type>::level_type * volatile level;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<FragmentAlignmentBufferMergeSortWorkPackage<order_type> > & mergeSortPackages;
				uint64_t const mergeSortDispatcherId;
				
				FragmentAlignmentBufferSortFinishedInterface & sortFinishedInterface;
				
				enum state_type
				{
					state_base_sort, state_merge_plan, state_merge_execute, state_copy_back, state_done
				};
				
				state_type volatile state;
				
				FragmentAlignmentBufferSortContext(
					FragmentAlignmentBuffer::shared_ptr_type rbuffer,
					uint64_t const numthreads,
					libmaus::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & rSTPI,
					libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<FragmentAlignmentBufferMergeSortWorkPackage<order_type> > & rmergeSortPackages,
					uint64_t const rmergeSortDispatcherId,
					FragmentAlignmentBufferSortFinishedInterface & rsortFinishedInterface
				)
				: 
					buffer(rbuffer), comparator(), 
					PSC(
						buffer->getPointerArray().first,
						buffer->getPointerArray().second,
						buffer->getAuxPointerArray().first,
						buffer->getAuxPointerArray().second,
						comparator,
						numthreads,
						true /* copy back */), 
					STPI(rSTPI),
					level(PSC.mergeLevels.levels.size() ? &(PSC.mergeLevels.levels[0]) : 0),
					mergeSortPackages(rmergeSortPackages),
					mergeSortDispatcherId(rmergeSortDispatcherId),
					sortFinishedInterface(rsortFinishedInterface),
					state(
						(buffer->getPointerArray().second-buffer->getPointerArray().first) ? state_base_sort : state_done
					)
				{
				}
				
				void enqueBaseSortPackages(
					libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<FragmentAlignmentBufferBaseSortPackage<order_type> > & baseSortPackages,
					uint64_t const baseSortDispatcher
				)
				{
					// enque base sort requests if there are any
					if ( PSC.baseSortRequests.baseSortRequests.size() )
					{
						for ( uint64_t i = 0; i < PSC.baseSortRequests.baseSortRequests.size(); ++i )
						{
							FragmentAlignmentBufferBaseSortPackage<order_type> * package = baseSortPackages.getPackage();
							*package = FragmentAlignmentBufferBaseSortPackage<order_type>(
								0 /* prio */,
								&(PSC.baseSortRequests.baseSortRequests[i]),
								this,
								baseSortDispatcher
							);
						
							STPI.enque(package);
						}
					}
					// empty array, notify that we are done
					else
					{
						sortFinishedInterface.putSortFinished(buffer);				
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

					typename libmaus::sorting::ParallelStableSort::MergeLevels<iterator,order_type>::level_type * looplevel = level;
					uint64_t const numreq = looplevel->mergeRequests.size();

					for ( uint64_t i = 0; i < numreq; ++i )
					{
						FragmentAlignmentBufferMergeSortWorkPackage<order_type> * package = mergeSortPackages.getPackage();
						*package = FragmentAlignmentBufferMergeSortWorkPackage<order_type>(
							0 /* prio */,
							&(looplevel->mergeRequests[i]),
							this,
							mergeSortDispatcherId
						);
						STPI.enque(package);
					}
				}
	
				virtual void baseBlockSorted()
				{
					if ( PSC.baseSortRequests.requestsFinished.increment() == PSC.baseSortRequests.baseSortRequests.size() )
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
					if ( level->requestsFinished.increment() == level->mergeRequests.size() )
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
