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
			static iterator parallelSort(
				iterator aa,
				iterator ae,
				iterator ba,
				iterator be,
				order_type order = std::less< typename std::iterator_traits<iterator>::value_type >(),
				uint64_t const num_threads =
				#if defined(_OPENMP)
				omp_get_max_threads()
				#else
				1
				#endif
			)
			{
				uint64_t const n = ae-aa;
				uint64_t pack = (n + num_threads - 1)/num_threads;
				uint64_t const numpacks = (n + pack - 1)/pack;
				
				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(num_threads)
				#endif
				for ( int64_t t = 0; t < static_cast<int64_t>(numpacks); ++t )
				{
					uint64_t const low = t * pack;
					uint64_t const high = std::min(low+pack,n);
					std::sort(aa + low, aa + high, order);
				}
				
				iterator in = aa;
				iterator out = ba;
				
				while ( pack < n )
				{
					uint64_t const mergesize = pack<<1;
					uint64_t const mergesteps = (n + mergesize-1)/mergesize;
					
					for ( uint64_t t = 0; t < mergesteps; ++t )
					{
						uint64_t const low = t*mergesize;
						uint64_t const high = std::min(low+mergesize,n);
						
						// merge
						if ( high-low > pack )
						{
							uint64_t const l = pack;
							uint64_t const r = (high-low)-pack;
							
							parallelMerge(
								in + low,
								in + low + l,
								in + low + l,
								in + low + l + r,
								out + low,
								order,
								num_threads
							);
						}
						// incomplete packet, copy it to other array as is
						else
							std::copy(in+low,in+high,out+low);
					}
					
					std::swap(in,out);
					pack <<= 1;
				}
				
				return in;
			}
		};
	}
}
#endif
