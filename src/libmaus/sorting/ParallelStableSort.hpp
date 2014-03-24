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
#if ! defined(LIBMAUS_SORTING_PARALLELSTABLESORT_HPP)
#define LIBMAUS_SORTING_PARALLELSTABLESORT_HPP

#include <libmaus/types/types.hpp>
#include <libmaus/sorting/MergeStepBinSearchResult.hpp>
#include <libmaus/parallel/SynchronousCounter.hpp>
#include <algorithm>

#if defined(_OPENMP)
#include <omp.h>
#endif

namespace libmaus
{
	namespace sorting
	{
		/**
		 * parallel stable merge sort using additional space
		 **/
		struct ParallelStableSort : public libmaus::sorting::MergeStepBinSearchResult
		{
			template<typename iterator, typename order_type>
			struct ParallelStableSortContextBase
			{
				iterator aa;
				iterator ae;
				iterator ba;
				iterator be;
				order_type const * order;
				uint64_t num_threads;
				bool copyback;
				uint64_t n;
				uint64_t pack;
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
				: aa(raa), ae(rae), ba(rba), be(rbe), order(&rorder), num_threads(rnum_threads), copyback(rcopyback),
				  n(ae-aa), pack((n + num_threads - 1)/num_threads), numpacks((n + pack - 1)/pack),
				  in(aa), out(ba)
				{
					
				}
			};

			struct SortRequest
			{
				virtual ~SortRequest() {}
				virtual void dispatch() = 0;
			};
		
			
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
			
			
			template<typename iterator, typename order_type>
			struct MergeLevel
			{
				ParallelStableSortContextBase<iterator,order_type> context;
				std::vector<MergeRequest<iterator,order_type> > mergeRequests;
				libmaus::parallel::SynchronousCounter<uint64_t> requestsFinished;
				
				MergeLevel<iterator,order_type> * next;
				
				MergeLevel() : requestsFinished(0), next(0) {}
				MergeLevel(
					ParallelStableSortContextBase<iterator,order_type> const & rcontext
				) : context(rcontext), requestsFinished(0), next(0)
				{
				
				}
			
				void dispatch()
				{
					uint64_t const mergesize = context.pack<<1;
					uint64_t const mergesteps = (context.n + mergesize-1)/mergesize;
					
					for ( uint64_t t = 0; t < mergesteps; ++t )
					{
						uint64_t const low = t*mergesize;
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
				
				void subdispatch()
				{
					#if defined(_OPENMP)
					#pragma omp parallel for num_threads(context.num_threads)
					#endif
					for ( int64_t t = 0; t < mergeRequests.size(); ++t )
						mergeRequests[t].dispatch();				
				}
			};

			template<typename iterator, typename order_type>
			struct MergeLevels
			{
				typedef MergeLevel<iterator,order_type> level_type;
				std::vector<level_type> levels;
				libmaus::parallel::SynchronousCounter<uint64_t> levelsFinished;
				
				MergeLevels() {}
				MergeLevels(ParallelStableSortContextBase<iterator,order_type> & context) 
				{
					while ( context.pack < context.n )
					{
						levels.push_back(MergeLevel<iterator,order_type>(context));
						std::swap(context.in,context.out);
						context.pack <<= 1;
					}
				
					for ( uint64_t i = 1; i < levels.size(); ++i )
						levels[i-1].next = &(levels[i]);
				}
				
				void dispatch()
				{
					for ( uint64_t i = 0; i < levels.size(); ++i )
					{
						levels[i].dispatch();
						levels[i].subdispatch();
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
				libmaus::parallel::SynchronousCounter<uint64_t> requestsFinished;
				
				BaseSortRequestSet() : requestsFinished(0) {}
				BaseSortRequestSet(ParallelStableSortContextBase<iterator,order_type> & rcontext)
				: context(&rcontext), baseSortRequests(context->numpacks), requestsFinished(0)
				{
					for ( int64_t t = 0; t < static_cast<int64_t>(context->numpacks); ++t )
					{
						uint64_t const low = t * context->pack;
						uint64_t const high = std::min(low+context->pack,context->n);
						baseSortRequests[t] = request_type(
							context->aa + low, context->aa + high, *context->order
						);
					}	
				}

				void dispatch()
				{
					#if defined(_OPENMP)
					#pragma omp parallel for num_threads(context->num_threads)
					#endif
					for ( int64_t t = 0; t < static_cast<int64_t>(context->numpacks); ++t )
						baseSortRequests[t].dispatch();
				}
			};
			
			
			template<typename _iterator, typename _order_type>
			struct ParallelSortControl
			{
				typedef _iterator iterator;
				typedef _order_type order_type;
				typedef ParallelSortControl<iterator,order_type> this_type;
				typedef typename libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef typename libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
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
