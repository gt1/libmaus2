/*
    libmaus2
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
#if ! defined(LIBMAUS2_SORTING_PARALLELSTABLESORT_HPP)
#define LIBMAUS2_SORTING_PARALLELSTABLESORT_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/sorting/MergeStepBinSearchResult.hpp>
#include <libmaus2/parallel/SynchronousCounter.hpp>
#include <libmaus2/parallel/LockedCounter.hpp>
#include <algorithm>

#if defined(_OPENMP)
#include <omp.h>
#endif

namespace libmaus2
{
	namespace sorting
	{
		/**
		 * parallel stable merge sort using additional space
		 **/
		struct ParallelStableSort : public libmaus2::sorting::MergeStepBinSearchResult
		{
			/**
			 * sort context
			 **/
			template<typename iterator, typename order_type>
			struct ParallelStableSortContextBase
			{
				// input array start
				iterator aa;
				// input array end
				iterator ae;
				// temp array start
				iterator ba;
				// temp array end
				iterator be;
				// order
				order_type const * order;
				// number of sort threads
				uint64_t num_threads;
				// copy back on finish (if data ends up on the temp array)
				bool copyback;
				// size of input
				uint64_t n;
				// packet size
				uint64_t pack;
				// number of packets
				uint64_t numpacks;
				
				iterator in;
				iterator out;

				ParallelStableSortContextBase() {}
				ParallelStableSortContextBase(ParallelStableSortContextBase const & o)
				: aa(o.aa), ae(o.ae), ba(o.ba), be(o.be), order(o.order),
				  num_threads(o.num_threads), copyback(o.copyback), n(o.n),
				  pack(o.pack), numpacks(o.numpacks), in(o.in), out(o.out)
				{
				
				}
				ParallelStableSortContextBase(
					iterator raa,
					iterator rae,
					iterator rba,
					iterator rbe,
					order_type const & rorder,
					uint64_t rnum_threads,
					bool rcopyback
				)
				: 
					// input
					aa(raa), ae(rae), 
					// temp
					ba(rba), be(rbe), 
					// comparator
					order(&rorder), 
					// number of threads
					num_threads(rnum_threads), 
					// copy back requested?
					copyback(rcopyback),
					// number of input elements
					n(ae-aa), 
					// packet size
					pack((n + num_threads - 1)/num_threads), 
					// number of packets
					numpacks( pack ? ((n+pack-1)/pack) : 0),
					in(aa), out(ba)
				{
					
				}
			};

			/**
			 * sort request interface
			 **/
			struct SortRequest
			{
				virtual ~SortRequest() {}
				virtual void dispatch() = 0;
			};
			
			/**
			 * merge request
			 **/
			template<typename iterator, typename order_type>
			struct MergeRequest : public SortRequest
			{
				iterator aa;
				iterator ab;
				iterator ba;
				iterator bb;
				iterator oa;
				order_type const * order;
				
				MergeRequest() : aa(), ab(), ba(), bb(), oa(), order(0) {}
				MergeRequest(
					iterator raa,
					iterator rab,
					iterator rba,
					iterator rbb,
					iterator roa,
					order_type const * rorder
				) : aa(raa), ab(rab), ba(rba), bb(rbb), oa(roa), order(rorder) {}
				
				void dispatch()
				{
					if ( ba != bb )
						std::merge(aa,ab,ba,bb,oa,*order);
					else
						std::copy(aa,ab,oa);
				}
			};
		
			/**
			 * create multi threaded plan for merging
			 **/
			template<typename iterator, typename order_type>
			static void parallelMergePlan(
				iterator aa,
				iterator ae,
				iterator ba,
				iterator be,
				iterator oa,
				order_type const & order,
				uint64_t const num_threads,
				std::vector<MergeRequest<iterator,order_type> > & requests
			)
			{
				std::vector< std::pair<uint64_t,uint64_t> > const S = 
					mergeSplitVector(aa,ae,ba,be,order,num_threads);

				for ( int64_t i = 0; i < static_cast<int64_t>(num_threads); ++i )
				{
					requests.push_back(MergeRequest<iterator,order_type>(
						aa + S[i+0].first,
						aa + S[i+1].first,
						ba + S[i+0].second,
						ba + S[i+1].second,
						oa + S[i].first + S[i].second,
						&order));
				}
			}

			/**
			 * parallel OMP based merging
			 **/
			template<typename iterator, typename order_type>
			static void parallelMerge(
				iterator aa,
				iterator ae,
				iterator ba,
				iterator be,
				iterator oa,
				order_type order = std::less< typename std::iterator_traits<iterator>::value_type >(),
				uint64_t const num_threads =
				#if defined(_OPENMP)
				omp_get_max_threads()
				#else
				1
				#endif
			)
			{
				std::vector< std::pair<uint64_t,uint64_t> > const S = 
					mergeSplitVector(aa,ae,ba,be,order,num_threads);

				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(num_threads)
				#endif
				for ( int64_t i = 0; i < static_cast<int64_t>(num_threads); ++i )
				{
					std::merge(
						aa + S[i+0].first,
						aa + S[i+1].first,
						ba + S[i+0].second,
						ba + S[i+1].second,
						oa + S[i].first + S[i].second,
						order);
				}
			}
			
			/**
			 * one level of merging
			 **/
			template<typename iterator, typename order_type>
			struct MergeLevel
			{
				ParallelStableSortContextBase<iterator,order_type> context;
				std::vector<MergeRequest<iterator,order_type> > mergeRequests;
				libmaus2::parallel::LockedCounter requestsFinished;
				
				MergeLevel<iterator,order_type> * next;
				
				MergeLevel() : requestsFinished(0), next(0) {}
				MergeLevel(
					ParallelStableSortContextBase<iterator,order_type> const & rcontext
				) : context(rcontext), requestsFinished(0), next(0)
				{
				
				}
			
				/**
				 * create plan for merging
				 **/
				void dispatch()
				{
					uint64_t const mergesize = context.pack<<1;
					uint64_t const mergesteps = (context.n + mergesize-1)/mergesize;
					
					for ( uint64_t t = 0; t < mergesteps; ++t )
					{
						uint64_t const low =  std::min(t*mergesize  ,context.n);
						uint64_t const high = std::min(low+mergesize,context.n);
						
						// merge
						if ( high-low > context.pack )
						{
							uint64_t const l = context.pack;
							uint64_t const r = (high-low)-context.pack;

							parallelMergePlan(
								context.in + low,
								context.in + low + l,
								context.in + low + l,
								context.in + low + l + r,
								context.out + low,
								*(context.order),
								context.num_threads,
								mergeRequests
							);						
						}
						else
						{
							mergeRequests.push_back(
								MergeRequest<iterator,order_type>(
									context.in+low,context.in+high,
									context.in+high,context.in+high,
									context.out+low,
									context.order
								)
							);
						}
					}

				}
				
				/**
				 * execute plan for merging (parallel through openmp)
				 **/
				void subdispatch()
				{
					#if defined(_OPENMP)
					unsigned int const num_threads = context.num_threads;
					#pragma omp parallel for num_threads(num_threads)
					#endif
					for ( int64_t t = 0; t < static_cast<int64_t>(mergeRequests.size()); ++t )
						mergeRequests[t].dispatch();				
				}
			};

			/**
			 * set of merge levels for sorting
			 **/
			template<typename iterator, typename order_type>
			struct MergeLevels
			{
				typedef MergeLevel<iterator,order_type> level_type;
				std::vector<level_type> levels;
				libmaus2::parallel::SynchronousCounter<uint64_t> levelsFinished;
				
				void setNextLevelPointers()
				{
					for ( uint64_t i = 1; i < levels.size(); ++i )
						levels[i-1].next = &(levels[i]);				
				}
				
				MergeLevels() {}
				MergeLevels(ParallelStableSortContextBase<iterator,order_type> & context) 
				{
					while ( context.pack < context.n )
					{
						levels.push_back(MergeLevel<iterator,order_type>(context));
						std::swap(context.in,context.out);
						context.pack <<= 1;
					}
				
					setNextLevelPointers();
				}
				
				void dispatch()
				{
					for ( level_type * cur = levels.size() ? &levels[0] : 0; cur; cur = cur->next )
					{
						cur->dispatch();
						cur->subdispatch();					
					}
				}
			};

			template<typename iterator, typename order_type>
			struct BaseSortRequest : public SortRequest
			{
				iterator low;
				iterator high;
				order_type const * order;
				
				BaseSortRequest() : low(), high(), order(0) {}
				BaseSortRequest(iterator rlow, iterator rhigh, order_type const & rorder)
				: low(rlow), high(rhigh), order(&rorder) {}
				
				void dispatch()
				{
					std::stable_sort(low, high, *order);				
				}
			};
						
			template<typename iterator, typename order_type>
			struct BaseSortRequestSet
			{
				typedef BaseSortRequest<iterator,order_type> request_type;
				
				ParallelStableSortContextBase<iterator,order_type> * context;
				std::vector<request_type> baseSortRequests;
				libmaus2::parallel::LockedCounter requestsFinished;
				
				BaseSortRequestSet() : requestsFinished(0) {}
				BaseSortRequestSet(ParallelStableSortContextBase<iterator,order_type> & rcontext)
				: context(&rcontext), baseSortRequests(context->numpacks), requestsFinished(0)
				{
					for ( int64_t t = 0; t < static_cast<int64_t>(context->numpacks); ++t )
					{
						uint64_t const low  = std::min(t * context->pack,context->n);
						uint64_t const high = std::min(low+context->pack,context->n);
						baseSortRequests[t] = request_type(
							context->aa + low, context->aa + high, *context->order
						);
					}	
				}

				void dispatch()
				{
					#if defined(_OPENMP)
					unsigned int const num_threads = context->num_threads;
					#pragma omp parallel for num_threads(num_threads)
					#endif
					for ( int64_t t = 0; t < static_cast<int64_t>(context->numpacks); ++t )
						baseSortRequests[t].dispatch();
				}
			};

			template<typename _iterator, typename _order_type>
			struct ParallelSortControlState
			{
				typedef _iterator iterator;
				typedef _order_type order_type;
				typedef typename MergeLevels<iterator,order_type>::level_type level_type;

				enum parallel_sort_state
				{
					sort_state_base_sort,
					sort_state_plan_merge,
					sort_state_execute_merge,
					sort_state_copy_back,
					sort_state_done
				};

				parallel_sort_state state;
				BaseSortRequestSet<iterator,order_type> *basesortreqs;
				level_type * level;
				bool needCopyBack;
				iterator copyBackFrom;
				iterator copyBackTo;
				uint64_t copyBackN;
				
				ParallelSortControlState() : state(sort_state_base_sort), level(0), needCopyBack(false), copyBackFrom(iterator()), copyBackTo(iterator()), copyBackN(0)
				{
				
				}
				
				ParallelSortControlState(
					BaseSortRequestSet<iterator,order_type> * rbasesortreqs,
					level_type * rlevel,
					bool const rneedCopyBack,
					iterator rcopyBackFrom,
					iterator rcopyBackTo,
					uint64_t rcopyBackN
				) : state(sort_state_base_sort), basesortreqs(rbasesortreqs), level(rlevel), needCopyBack(rneedCopyBack), copyBackFrom(rcopyBackFrom), copyBackTo(rcopyBackTo), copyBackN(rcopyBackN) 
				{
				
				}
				
				bool serialStep()
				{
					switch ( state )
					{
						case sort_state_base_sort:
							for ( uint64_t i = 0; i < basesortreqs->baseSortRequests.size(); ++i )
								basesortreqs->baseSortRequests[i].dispatch();
							
							if ( level )
								state = sort_state_plan_merge;
							else
								state = sort_state_copy_back;	
							
							break;
						case sort_state_plan_merge:
							level->dispatch();
							state = sort_state_execute_merge;

							break;
						case sort_state_execute_merge:
							for ( uint64_t i = 0; i < level->mergeRequests.size(); ++i )
								level->mergeRequests[i].dispatch();
							
							level = level->next;
							
							if ( level )
								state = sort_state_plan_merge;
							else
								state = sort_state_copy_back;
							
							break;
						case sort_state_copy_back:
						
							if ( needCopyBack )
								std::copy(copyBackFrom,copyBackFrom+copyBackN,copyBackTo);			
						
							state = sort_state_done;
						
							break;
					}
					
					return state == sort_state_done;
				}
			};
						
			template<typename _iterator, typename _order_type>
			struct ParallelSortControl
			{
				typedef _iterator iterator;
				typedef _order_type order_type;
				typedef ParallelSortControl<iterator,order_type> this_type;
				typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
				
				ParallelStableSortContextBase<iterator,order_type> context;
				BaseSortRequestSet<iterator,order_type> baseSortRequests;
				MergeLevels<iterator,order_type> mergeLevels;
				bool const needCopyBack;

				ParallelSortControl(		
					iterator const raa,
					iterator const rae,
					iterator const rba,
					iterator const rbe,
					order_type const & rorder,
					uint64_t const rnum_threads =
					#if defined(_OPENMP)
					omp_get_max_threads()
					#else
						1
					#endif
					,
					bool const rcopyback = true
				)
				:
					context(raa,rae,rba,rbe,rorder,rnum_threads,rcopyback),
					baseSortRequests(context),
					mergeLevels(context),
					needCopyBack(context.copyback && (context.in != context.aa))
				{
				}

				ParallelSortControlState<iterator,order_type> getSortState()
				{
					return ParallelSortControlState<iterator,order_type>(
						&baseSortRequests,
						mergeLevels.levels.size() ? &(mergeLevels.levels[0]) : 0,
						needCopyBack,
						context.in,
						context.out,
						context.n
					);
				}
				
				void dispatch()
				{
					baseSortRequests.dispatch();
					mergeLevels.dispatch();

					if ( needCopyBack )
					{
						std::copy(context.in,context.in+context.n,context.out);
						std::swap(context.in,context.out);
						assert ( context.in == context.aa );
					}
				}
			};

			template<typename iterator, typename order_type>
			static iterator parallelSort(
				iterator const raa,
				iterator const rae,
				iterator const rba,
				iterator const rbe,
				order_type const rorder = std::less< typename std::iterator_traits<iterator>::value_type >(),
				uint64_t const rnum_threads =
				#if defined(_OPENMP)
				omp_get_max_threads()
				#else
				1
				#endif
				,
				bool const rcopyback = true
			)
			{
				ParallelSortControl<iterator,order_type> 
					control(raa,rae,rba,rbe,rorder,rnum_threads,rcopyback);
				control.dispatch();
				return control.context.in;
			}
		};
	}
}
#endif
