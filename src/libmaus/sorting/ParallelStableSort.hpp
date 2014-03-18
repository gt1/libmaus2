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
				std::vector< std::pair<uint64_t,uint64_t> > const S = mergeSplitVector(aa,ae,ba,be,order,num_threads);

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
			struct ParallelStableSortContextBase
			{
				iterator const aa;
				iterator const ae;
				iterator const ba;
				iterator const be;
				order_type const order;
				uint64_t const num_threads;
				bool const copyback;
				uint64_t const n;
				uint64_t pack;
				uint64_t const numpacks;
				
				iterator in;
				iterator out;

				ParallelStableSortContextBase(
					iterator const raa,
					iterator const rae,
					iterator const rba,
					iterator const rbe,
					order_type const rorder,
					uint64_t const rnum_threads,
					bool const rcopyback
				)
				: aa(raa), ae(rae), ba(rba), be(rbe), order(rorder), num_threads(rnum_threads), copyback(rcopyback),
				  n(ae-aa), pack((n + num_threads - 1)/num_threads), numpacks((n + pack - 1)/pack),
				  in(aa), out(ba)
				{
					
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
				ParallelStableSortContextBase<iterator,order_type> context(
					raa,rae,rba,rbe,rorder,rnum_threads,rcopyback
				);
				
				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(context.num_threads)
				#endif
				for ( int64_t t = 0; t < static_cast<int64_t>(context.numpacks); ++t )
				{
					uint64_t const low = t * context.pack;
					uint64_t const high = std::min(low+context.pack,context.n);
					std::stable_sort(context.aa + low, context.aa + high, context.order);
				}
				
				while ( context.pack < context.n )
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
							
							parallelMerge(
								context.in + low,
								context.in + low + l,
								context.in + low + l,
								context.in + low + l + r,
								context.out + low,
								context.order,
								context.num_threads
							);
						}
						// incomplete packet, copy it to other array as is
						else
							std::copy(context.in+low,context.in+high,context.out+low);
					}
					
					std::swap(context.in,context.out);
					context.pack <<= 1;
				}
				
				if ( context.copyback && (context.in != context.aa) )
				{
					std::copy(context.in,context.in+context.n,context.out);
					std::swap(context.in,context.out);
					assert ( context.in == context.aa );
				}
								
				return context.in;
			}
		};
	}
}
#endif
