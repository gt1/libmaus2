/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#if ! defined(LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_HPP)
#define LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_HPP

#include <climits>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/math/lowbits.hpp>
#include <libmaus2/util/PrefixSums.hpp>

namespace libmaus2
{
	namespace sorting
	{
		template<typename type>
		struct InterleavedRadixSortIdentityProjector
		{
			typedef type key_type;

			static key_type project(type const & v)
			{
				return v;
			}
		};

		/**
		 * radix sorting with bucket sorting and histogram computation in the same run
		 **/
		struct InterleavedRadixSort
		{
			#define LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_BUCKET_SHIFT 8
			#define LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS 0x100ull
			#define LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_BUCKET_MASK 0xffull


			/**
			 * radix sort. Keys are extracted from records using key_projector::project (see InterleavedRadixSortIdentityProjector for an example)
			 *
			 * the sorted sequence is stored in [ita,ite) if rounds is even and [tita,tite) if rounds is odd
			 **/
			template<typename iterator, typename key_projector >
			static void radixsortTemplate(
				iterator ita, iterator ite,
				iterator tita, iterator tite,
				unsigned int const tnumthreads,
				unsigned int const rounds = (CHAR_BIT*sizeof(typename key_projector::key_type)) / LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_BUCKET_SHIFT)
			{
				typedef typename key_projector::key_type key_type;

				uint64_t const n = ite-ita;
				uint64_t const packsize = (n + tnumthreads - 1) / tnumthreads;
				uint64_t const numthreads = (n + packsize - 1) / packsize;

				libmaus2::autoarray::AutoArray<uint64_t> Ahist((numthreads+1) * LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS, false);
				libmaus2::autoarray::AutoArray<uint64_t> Athist(numthreads*numthreads* LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS, false);
				libmaus2::autoarray::AutoArray<uint64_t> Ahist_cmp((numthreads+1) * LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS, false);

				for ( unsigned int r = 0; r < rounds; ++r )
				{
					/*
					 * compute bucket histograms for first run
					 */
					if ( r == 0 )
					{
						#if defined(_OPENMP)
						#pragma omp parallel for num_threads(numthreads) schedule(dynamic,1)
						#endif
						for ( uint64_t t = 0; t < numthreads; ++t )
						{
							uint64_t const p_low = t * packsize;
							uint64_t const p_high = std::min(p_low+packsize,n);

							iterator p_ita = ita + p_low;
							iterator p_ite = ita + p_high;

							uint64_t * const hist = Ahist.begin() + t * LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS;

							for ( uint64_t i = 0; i < LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS; ++i )
								hist[i] = 0;

							while ( p_ita != p_ite )
							{
								typename ::std::iterator_traits<iterator>::value_type const & v = *(p_ita ++);
								key_type const key = key_projector::project(v);
								hist[ (key>>(r*LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_BUCKET_SHIFT)) & LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_BUCKET_MASK ] ++;
							}
						}
					}

					#if 0
					if ( r )
						for ( uint64_t i = 0; i < numthreads * LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS; ++i )
							assert ( Ahist[i] == Ahist_cmp[i] );
					#endif

					// compute prefix sums for each bucket over blocks
					for ( uint64_t i = 0; i < LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS; ++i )
						libmaus2::util::PrefixSums::prefixSums(Ahist.begin() + i,Ahist.begin() + i + (numthreads+1) * LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS,LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS);

					// compute prefix sums over whole data set
					libmaus2::util::PrefixSums::prefixSums(Ahist.begin() + (numthreads+0) * LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS,Ahist.begin() + (numthreads+1) * LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS,1);

					for ( uint64_t i = 0; i < LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS; ++i )
					{
						uint64_t const a = Ahist[(numthreads+0)*LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS+i];
						uint64_t * p = Ahist.begin() + i;
						for ( uint64_t j = 0; j < numthreads; ++j )
						{
							*p += a;
							p += LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS;
						}
					}

					if ( r + 1 < rounds )
					{
						#if defined(_OPENMP)
						#pragma omp parallel for num_threads(numthreads) schedule(dynamic,1)
						#endif
						for ( uint64_t t = 0; t < numthreads; ++t )
						{
							uint64_t const p_low = t * packsize;
							uint64_t const p_high = std::min(p_low+packsize,n);

							iterator p_ita = ita + p_low;
							iterator p_ite = ita + p_high;

							uint64_t * const hist = Ahist.begin() + t * LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS;
							uint64_t * const thist = Athist.begin() + t /* in block */ *numthreads*LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS;

							std::fill(Athist.begin() + (t+0)*numthreads*LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS,Athist.begin() + (t+1)*numthreads*LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS,0);

							unsigned int const prim_shift = r*LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_BUCKET_SHIFT;
							unsigned int const seco_shift = (r+1)*LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_BUCKET_SHIFT;

							while ( p_ita != p_ite )
							{
								typename ::std::iterator_traits<iterator>::value_type const & v = *(p_ita++);
								key_type const key = key_projector::project(v);
								uint64_t const bucket      = (key>>prim_shift) & LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_BUCKET_MASK;
								uint64_t const next_bucket = (key>>seco_shift) & LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_BUCKET_MASK;
								uint64_t const outpos      = hist[bucket]++;
								uint64_t const outpack     = outpos / packsize;
								thist [ outpack /* out block */ * LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS + next_bucket ] ++;
								tita[outpos] = v;
							}
						}

						#if defined(_OPENMP)
						#pragma omp parallel for num_threads(numthreads) schedule(dynamic,1)
						#endif
						for ( uint64_t t = 0; t < numthreads; ++t )
						{
							// uint64_t * const hist = Ahist_cmp.begin() + t * LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS;
							uint64_t * const hist = Ahist.begin() + t * LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS;

							for ( uint64_t i = 0; i < LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS; ++i )
							{
								hist[i] = 0;
								for ( uint64_t p = 0; p < numthreads; ++p )
									hist[i] += Athist[p /* in block */ *numthreads*LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS + t /* out block */ *LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS + i /* bucket */];
							}
						}
					}
					else
					{
						#if defined(_OPENMP)
						#pragma omp parallel for num_threads(numthreads) schedule(dynamic,1)
						#endif
						for ( uint64_t t = 0; t < numthreads; ++t )
						{
							uint64_t const p_low = t * packsize;
							uint64_t const p_high = std::min(p_low+packsize,n);

							iterator p_ita = ita + p_low;
							iterator p_ite = ita + p_high;

							uint64_t * const hist = Ahist.begin() + t * LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS;

							unsigned int const prim_shift = r*LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_BUCKET_SHIFT;

							while ( p_ita != p_ite )
							{
								typename ::std::iterator_traits<iterator>::value_type const & v = *(p_ita++);
								key_type const key = key_projector::project(v);
								uint64_t const bucket      = (key>>prim_shift) & LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_BUCKET_MASK;
								uint64_t const outpos      = hist[bucket]++;
								tita[outpos] = v;
							}
						}
					}

					std::swap(ita,tita);
					std::swap(ite,tite);
				}
			}

			template<typename iterator>
			static void radixsort(
				iterator ita, iterator ite,
				iterator tita, iterator tite,
				unsigned int const tnumthreads,
				unsigned int const rounds = (CHAR_BIT*sizeof(typename ::std::iterator_traits<iterator>::value_type)) / LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_BUCKET_SHIFT
			)
			{
				radixsortTemplate<iterator,InterleavedRadixSortIdentityProjector<typename ::std::iterator_traits<iterator>::value_type> >(ita,ite,tita,tite,tnumthreads,rounds);
			}

			/**
			 * radix sorting with bucket sorting and histogram computation in the same run
			 *
			 * byte access version
			 *
			 * keys are assumed to be stored in the first keylength bytes of each record
			 * for little endian these bytes are read left to right, for big endian right to left
			 *
			 * the sorted sequence is stored in [ita,ite) if rounds is even and [tita,tite) if rounds is odd
			 **/
			template<typename value_type, typename key_type >
			static void byteradixsortTemplate(
				value_type * ita, value_type * ite,
				value_type * tita, value_type * tite,
				unsigned int const tnumthreads,
				unsigned int const rounds = (CHAR_BIT*sizeof(key_type)) / LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_BUCKET_SHIFT,
				unsigned int const
					#if defined(LIBMAUS2_BYTE_ORDER_BIG_ENDIAN)
					keylength
					#endif
					= (CHAR_BIT*sizeof(key_type)) / LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_BUCKET_SHIFT
			)
			{
				uint64_t const n = ite-ita;
				uint64_t const packsize = (n + tnumthreads - 1) / tnumthreads;
				uint64_t const numthreads = (n + packsize - 1) / packsize;

				libmaus2::autoarray::AutoArray<uint64_t> Ahist((numthreads+1) * LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS, false);
				libmaus2::autoarray::AutoArray<uint64_t> Athist(numthreads*numthreads* LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS, false);
				libmaus2::autoarray::AutoArray<uint64_t> Ahist_cmp((numthreads+1) * LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS, false);

				for ( unsigned int r = 0; r < rounds; ++r )
				{
					/*
					 * compute bucket histograms for first run
					 */
					if ( r == 0 )
					{
						#if defined(LIBMAUS2_BYTE_ORDER_LITTLE_ENDIAN)
						unsigned int const key_offset_0 = r;
						#elif defined(LIBMAUS2_BYTE_ORDER_BIG_ENDIAN)
						unsigned int const key_offset_0 = keylength-r-1;
						#else
						#error "Unknown byte order"
						#endif

						#if defined(_OPENMP)
						#pragma omp parallel for num_threads(numthreads) schedule(dynamic,1)
						#endif
						for ( uint64_t t = 0; t < numthreads; ++t )
						{
							uint64_t const p_low = t * packsize;
							uint64_t const p_high = std::min(p_low+packsize,n);

							value_type * p_ita = ita + p_low;
							value_type * p_ite = ita + p_high;

							uint64_t * const hist = Ahist.begin() + t * LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS;

							for ( uint64_t i = 0; i < LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS; ++i )
								hist[i] = 0;

							while ( p_ita != p_ite )
								hist[ reinterpret_cast<uint8_t const *>(p_ita++)[key_offset_0] ] ++;
						}
					}

					#if 0
					if ( r )
						for ( uint64_t i = 0; i < numthreads * LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS; ++i )
							assert ( Ahist[i] == Ahist_cmp[i] );
					#endif

					// compute prefix sums for each bucket over blocks
					for ( uint64_t i = 0; i < LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS; ++i )
						libmaus2::util::PrefixSums::prefixSums(Ahist.begin() + i,Ahist.begin() + i + (numthreads+1) * LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS,LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS);

					// compute prefix sums over whole data set
					libmaus2::util::PrefixSums::prefixSums(Ahist.begin() + (numthreads+0) * LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS,Ahist.begin() + (numthreads+1) * LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS,1);

					for ( uint64_t i = 0; i < LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS; ++i )
					{
						uint64_t const a = Ahist[(numthreads+0)*LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS+i];
						uint64_t * p = Ahist.begin() + i;
						for ( uint64_t j = 0; j < numthreads; ++j )
						{
							*p += a;
							p += LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS;
						}
					}

					if ( r + 1 < rounds )
					{
						#if defined(LIBMAUS2_BYTE_ORDER_LITTLE_ENDIAN)
						unsigned int const key_offset_0 = r;
						#elif defined(LIBMAUS2_BYTE_ORDER_BIG_ENDIAN)
						unsigned int const key_offset_0 = keylength-r-1;
						#else
						#error "Unknown byte order"
						#endif

						#if defined(_OPENMP)
						#pragma omp parallel for num_threads(numthreads) schedule(dynamic,1)
						#endif
						for ( uint64_t t = 0; t < numthreads; ++t )
						{
							uint64_t const p_low = t * packsize;
							uint64_t const p_high = std::min(p_low+packsize,n);

							value_type * p_ita = ita + p_low;
							value_type * p_ite = ita + p_high;

							uint64_t * const hist = Ahist.begin() + t * LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS;
							uint64_t * const thist = Athist.begin() + t /* in block */ *numthreads*LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS;

							std::fill(Athist.begin() + (t+0)*numthreads*LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS,Athist.begin() + (t+1)*numthreads*LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS,0);

							while ( p_ita != p_ite )
							{
								value_type const & v = *(p_ita);
								uint8_t const * kp = reinterpret_cast<uint8_t const *>(p_ita) + key_offset_0;
								#if defined(LIBMAUS2_BYTE_ORDER_LITTLE_ENDIAN)
								uint64_t const bucket      = *(kp++);
								#elif defined(LIBMAUS2_BYTE_ORDER_BIG_ENDIAN)
								uint64_t const bucket      = *(kp--);
								#else
								#error "Unknown byte order"
								#endif
								uint64_t const next_bucket = *kp;
								uint64_t const outpos      = hist[bucket]++;
								uint64_t const outpack     = outpos / packsize;
								thist [ outpack /* out block */ * LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS + next_bucket ] ++;
								tita[outpos] = v;
								p_ita++;
							}
						}

						#if defined(_OPENMP)
						#pragma omp parallel for num_threads(numthreads) schedule(dynamic,1)
						#endif
						for ( uint64_t t = 0; t < numthreads; ++t )
						{
							// uint64_t * const hist = Ahist_cmp.begin() + t * LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS;
							uint64_t * const hist = Ahist.begin() + t * LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS;

							for ( uint64_t i = 0; i < LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS; ++i )
							{
								hist[i] = 0;
								for ( uint64_t p = 0; p < numthreads; ++p )
									hist[i] += Athist[p /* in block */ *numthreads*LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS + t /* out block */ *LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS + i /* bucket */];
							}
						}
					}
					else
					{
						#if defined(LIBMAUS2_BYTE_ORDER_LITTLE_ENDIAN)
						unsigned int const key_offset_0 = r;
						#elif defined(LIBMAUS2_BYTE_ORDER_BIG_ENDIAN)
						unsigned int const key_offset_0 = keylength-r-1;
						#else
						#error "Unknown byte order"
						#endif

						#if defined(_OPENMP)
						#pragma omp parallel for num_threads(numthreads) schedule(dynamic,1)
						#endif
						for ( uint64_t t = 0; t < numthreads; ++t )
						{
							uint64_t const p_low = t * packsize;
							uint64_t const p_high = std::min(p_low+packsize,n);

							value_type * p_ita = ita + p_low;
							value_type * p_ite = ita + p_high;

							uint64_t * const hist = Ahist.begin() + t * LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_NUM_BUCKETS;

							while ( p_ita != p_ite )
							{
								value_type const & v = *(p_ita);
								uint64_t const bucket      = reinterpret_cast<uint8_t const *>(p_ita)[key_offset_0];
								uint64_t const outpos      = hist[bucket]++;
								tita[outpos] = v;
								p_ita++;
							}
						}
					}

					std::swap(ita,tita);
					std::swap(ite,tite);
				}
			}

			template<typename value_type>
			static void byteradixsort(
				value_type * ita, value_type * ite,
				value_type * tita, value_type * tite,
				unsigned int const tnumthreads,
				unsigned int const rounds = (CHAR_BIT*sizeof(value_type)) / LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_BUCKET_SHIFT,
				unsigned int const keylength = (CHAR_BIT*sizeof(value_type)) / LIBMAUS2_SORTING_INTERLEAVEDRADIXSORT_BUCKET_SHIFT
			)
			{
				byteradixsortTemplate<value_type,value_type>(ita,ite,tita,tite,tnumthreads,rounds,keylength);
			}


		};
	}
}
#endif
