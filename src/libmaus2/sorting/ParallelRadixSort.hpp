/*
    libmaus2
    Copyright (C) 2009-2013 German Tischler
    Copyright (C) 2011-2013 Genome Research Limited

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


#if ! defined(PARALLELRADIXSORT_HPP)
#define PARALLELRADIXSORT_HPP

#if defined(_OPENMP)
#include <omp.h>
#endif

#include <libmaus2/autoarray/AutoArray.hpp>

namespace libmaus2
{
	namespace sorting
	{
		template<typename data_type, typename key_projector, bool single_thread = false>
		struct RadixSort32
		{
			typedef u_int32_t key_type;
			static unsigned int const histbits = 11;
			static u_int64_t const histsize = 1ull << histbits;
			static u_int64_t const histmask = histsize-1;

			static inline key_type mask0(data_type const v, key_projector const & K)
			{
				return (K(v) & histmask);
			}
			static inline key_type mask1(data_type const v, key_projector const & K)
			{
				return ((K(v)>>histbits) & histmask);
			}
			static inline key_type mask2(data_type const v, key_projector const & K)
			{
				return ((K(v)>>(2*histbits)) & histmask);
			}

			static void radixSort(::libmaus2::autoarray::AutoArray<data_type> & AA, u_int64_t n, key_projector const & K, int const inumthreads = -1)
			{
				data_type * const A = AA.get();
				#if defined(_OPENMP)
				unsigned int numthreads = single_thread ? 1 : (inumthreads <= 0 ? omp_get_max_threads() : inumthreads);
				#else
				unsigned int const numthreads = 1;
				#endif
				::libmaus2::autoarray::AutoArray < u_int64_t > Ahist0(3 * numthreads * histsize,false);

				u_int64_t * const ghist0 = Ahist0.get();
				u_int64_t * const ghist1 = ghist0 + numthreads * histsize;
				u_int64_t * const ghist2 = ghist1 + numthreads * histsize;

				#if defined(_OPENMP)
				#pragma omp parallel num_threads(numthreads)
				#endif
				{
					#if defined(_OPENMP)
					unsigned int const threadid = omp_get_thread_num();
					#else
					unsigned int const threadid = 0;
					#endif
					u_int64_t * const hist0 = ghist0 + threadid * histsize;
					u_int64_t * const hist1 = ghist1 + threadid * histsize;
					u_int64_t * const hist2 = ghist2 + threadid * histsize;
					u_int64_t const elpert = (n + (numthreads-1)) / numthreads;
					u_int64_t const low = threadid * elpert;
					u_int64_t const high = (low+elpert) < n ? (low+elpert) : n;

					// erase histogram
					for ( u_int64_t i = 0; i < histsize; ++i )
						hist0[i] = hist1[i] = hist2[i] = 0;
					// count
					for ( u_int64_t i = low; i < high; ++i )
					{
						hist0 [ mask0(A[i],K) ] ++;
						hist1 [ mask1(A[i],K) ] ++;
						hist2 [ mask2(A[i],K) ] ++;
					}

					// accumulate
					u_int64_t c0 = 0;
					u_int64_t c1 = 0;
					u_int64_t c2 = 0;
					for ( u_int64_t i = 0; i < histsize; ++i )
					{
						u_int64_t const t0 = hist0[i];
						u_int64_t const t1 = hist1[i];
						u_int64_t const t2 = hist2[i];
						hist0[i] = c0;
						hist1[i] = c1;
						hist2[i] = c2;
						c0 += t0;
						c1 += t1;
						c2 += t2;
					}
				}

				#if defined(_OPENMP)
				#pragma omp parallel num_threads(numthreads)
				#endif
				{
					#if defined(_OPENMP)
					unsigned int const threadid = omp_get_thread_num();
					#else
					unsigned int const threadid = 0;
					#endif
					u_int64_t const vpert = (histsize + (numthreads-1))/numthreads;
					u_int64_t const vlow = threadid * vpert;
					u_int64_t const vhigh = (vlow+vpert) < histsize ? (vlow+vpert):histsize;

					for ( u_int64_t v = vlow; v < vhigh; ++v )
						for ( unsigned int i = 1; i < numthreads; ++i )
						{
							ghist0 [ v ] += ghist0 [ i * histsize + v ];
							ghist1 [ v ] += ghist1 [ i * histsize + v ];
							ghist2 [ v ] += ghist2 [ i * histsize + v ];
						}



					for ( u_int64_t v = vlow; v < vhigh; ++v )
					{
						ghist0[v] -= 1;
						ghist1[v] -= 1;
						ghist2[v] -= 1;
					}
				}

				::libmaus2::autoarray::AutoArray<data_type> T(n,false);

				#if defined(_OPENMP)
				#pragma omp parallel num_threads(numthreads)
				#endif
				{
					#if defined(_OPENMP)
					unsigned int const threadid = omp_get_thread_num();
					#else
					unsigned int const threadid = 0;
					#endif
					u_int64_t const vpert = (histsize + (numthreads-1))/numthreads;
					u_int64_t const vlow = threadid * vpert;
					u_int64_t const vhigh = (vlow+vpert)<histsize ? (vlow+vpert):histsize;

					for ( u_int64_t i = 0; i < n; ++i )
					{
						key_type const k = mask0(A[i],K);

						if ( ! single_thread )
						{
							if ( k >= vlow && k < vhigh )
								T[ ++ghist0[k]  ] = A[i];
						}
						else
						{
							T[ ++ghist0[k]  ] = A[i];
						}
					}
					#if defined(_OPENMP)
					#pragma omp barrier
					#endif
					for ( u_int64_t i = 0; i < n; ++i )
					{
						key_type const k = mask1(T[i],K);

						if ( ! single_thread )
						{
							if ( k >= vlow && k < vhigh )
								A[ ++ghist1[k]  ] = T[i];
						}
						else
								A[ ++ghist1[k]  ] = T[i];
					}
					#if defined(_OPENMP)
					#pragma omp barrier
					#endif
					for ( u_int64_t i = 0; i < n; ++i )
					{
						key_type const k = mask2(A[i],K);

						if ( ! single_thread )
						{
							if ( k >= vlow && k < vhigh )
								T[ ++ghist2[k]  ] = A[i];
						}
						else
								T[ ++ghist2[k]  ] = A[i];
					}
				}

				AA = T;
			}
		};

		template<typename data_type, typename key_projector, bool single_thread = false>
		struct RadixSort64
		{
			typedef u_int64_t key_type;
			static unsigned int const histbits = 11;
			static u_int64_t const histsize = 1ull << histbits;
			static u_int64_t const histmask = histsize-1;

			static inline key_type mask0(data_type const v, key_projector const & K)
			{
				return (K(v) & histmask);
			}
			static inline key_type mask1(data_type const v, key_projector const & K)
			{
				return ((K(v)>>histbits) & histmask);
			}
			static inline key_type mask2(data_type const v, key_projector const & K)
			{
				return ((K(v)>>(2*histbits)) & histmask);
			}
			static inline key_type mask3(data_type const v, key_projector const & K)
			{
				return ((K(v)>>(3*histbits)) & histmask);
			}
			static inline key_type mask4(data_type const v, key_projector const & K)
			{
				return ((K(v)>>(4*histbits)) & histmask);
			}
			static inline key_type mask5(data_type const v, key_projector const & K)
			{
				return ((K(v)>>(5*histbits)) & histmask);
			}

			static void radixSort(
				::libmaus2::autoarray::AutoArray<data_type> & AA,
				u_int64_t n,
				key_projector const & K,
				int inumthreads = -1)
			{
				data_type * const A = AA.get();
				#if defined(_OPENMP)
				unsigned int numthreads = single_thread ? 1 : (inumthreads <= 0 ? omp_get_max_threads() : inumthreads);
				#else
				unsigned int const numthreads = 1;
				#endif
				::libmaus2::autoarray::AutoArray < u_int64_t > Ahist0(6 * numthreads * histsize,false);

				u_int64_t * const ghist0 = Ahist0.get();
				u_int64_t * const ghist1 = ghist0 + numthreads * histsize;
				u_int64_t * const ghist2 = ghist1 + numthreads * histsize;
				u_int64_t * const ghist3 = ghist2 + numthreads * histsize;
				u_int64_t * const ghist4 = ghist3 + numthreads * histsize;
				u_int64_t * const ghist5 = ghist4 + numthreads * histsize;

				#if defined(_OPENMP)
				#pragma omp parallel num_threads(numthreads)
				#endif
				{
					#if defined(_OPENMP)
					unsigned int const threadid = omp_get_thread_num();
					#else
					unsigned int const threadid = 0;
					#endif
					u_int64_t * const hist0 = ghist0 + threadid * histsize;
					u_int64_t * const hist1 = ghist1 + threadid * histsize;
					u_int64_t * const hist2 = ghist2 + threadid * histsize;
					u_int64_t * const hist3 = ghist3 + threadid * histsize;
					u_int64_t * const hist4 = ghist4 + threadid * histsize;
					u_int64_t * const hist5 = ghist5 + threadid * histsize;
					u_int64_t const elpert = (n + (numthreads-1)) / numthreads;
					u_int64_t const low = threadid * elpert;
					u_int64_t const high = (low+elpert) < n ? (low+elpert) : n;

					// erase histogram
					for ( u_int64_t i = 0; i < histsize; ++i )
						hist0[i] = hist1[i] = hist2[i] = hist3[i] = hist4[i] = hist5[i] = 0;
					// count
					for ( u_int64_t i = low; i < high; ++i )
					{
						hist0 [ mask0(A[i],K) ] ++;
						hist1 [ mask1(A[i],K) ] ++;
						hist2 [ mask2(A[i],K) ] ++;
						hist3 [ mask3(A[i],K) ] ++;
						hist4 [ mask4(A[i],K) ] ++;
						hist5 [ mask5(A[i],K) ] ++;
					}

					// accumulate
					u_int64_t c0 = 0;
					u_int64_t c1 = 0;
					u_int64_t c2 = 0;
					u_int64_t c3 = 0;
					u_int64_t c4 = 0;
					u_int64_t c5 = 0;
					for ( u_int64_t i = 0; i < histsize; ++i )
					{
						u_int64_t const t0 = hist0[i];
						u_int64_t const t1 = hist1[i];
						u_int64_t const t2 = hist2[i];
						u_int64_t const t3 = hist3[i];
						u_int64_t const t4 = hist4[i];
						u_int64_t const t5 = hist5[i];
						hist0[i] = c0;
						hist1[i] = c1;
						hist2[i] = c2;
						hist3[i] = c3;
						hist4[i] = c4;
						hist5[i] = c5;
						c0 += t0;
						c1 += t1;
						c2 += t2;
						c3 += t3;
						c4 += t4;
						c5 += t5;
					}
				}

				#if defined(_OPENMP)
				#pragma omp parallel num_threads(numthreads)
				#endif
				{
					#if defined(_OPENMP)
					unsigned int const threadid = omp_get_thread_num();
					#else
					unsigned int const threadid = 0;
					#endif
					u_int64_t const vpert = (histsize + (numthreads-1))/numthreads;
					u_int64_t const vlow = threadid * vpert;
					u_int64_t const vhigh = (vlow+vpert) < histsize ? (vlow+vpert):histsize;

					for ( u_int64_t v = vlow; v < vhigh; ++v )
						for ( unsigned int i = 1; i < numthreads; ++i )
						{
							ghist0 [ v ] += ghist0 [ i * histsize + v ];
							ghist1 [ v ] += ghist1 [ i * histsize + v ];
							ghist2 [ v ] += ghist2 [ i * histsize + v ];
							ghist3 [ v ] += ghist3 [ i * histsize + v ];
							ghist4 [ v ] += ghist4 [ i * histsize + v ];
							ghist5 [ v ] += ghist5 [ i * histsize + v ];
						}



					for ( u_int64_t v = vlow; v < vhigh; ++v )
					{
						ghist0[v] -= 1;
						ghist1[v] -= 1;
						ghist2[v] -= 1;
						ghist3[v] -= 1;
						ghist4[v] -= 1;
						ghist5[v] -= 1;
					}
				}

				::libmaus2::autoarray::AutoArray<data_type> T(n,false);

				#if defined(_OPENMP)
				#pragma omp parallel num_threads(numthreads)
				#endif
				{
					#if defined(_OPENMP)
					unsigned int const threadid = omp_get_thread_num();
					#else
					unsigned int const threadid = 0;
					#endif
					u_int64_t const vpert = (histsize + (numthreads-1))/numthreads;
					u_int64_t const vlow = threadid * vpert;
					u_int64_t const vhigh = (vlow+vpert)<histsize ? (vlow+vpert):histsize;

					for ( u_int64_t i = 0; i < n; ++i )
					{
						key_type const k = mask0(A[i],K);

						if ( ! single_thread )
						{
							if ( k >= vlow && k < vhigh )
								T[ ++ghist0[k]  ] = A[i];
						}
						else
						{
							T[ ++ghist0[k]  ] = A[i];
						}
					}
					#if defined(_OPENMP)
					#pragma omp barrier
					#endif
					for ( u_int64_t i = 0; i < n; ++i )
					{
						key_type const k = mask1(T[i],K);

						if ( ! single_thread )
						{
							if ( k >= vlow && k < vhigh )
								A[ ++ghist1[k]  ] = T[i];
						}
						else
								A[ ++ghist1[k]  ] = T[i];
					}
					#if defined(_OPENMP)
					#pragma omp barrier
					#endif
					for ( u_int64_t i = 0; i < n; ++i )
					{
						key_type const k = mask2(A[i],K);

						if ( ! single_thread )
						{
							if ( k >= vlow && k < vhigh )
								T[ ++ghist2[k]  ] = A[i];
						}
						else
								T[ ++ghist2[k]  ] = A[i];
					}
					#if defined(_OPENMP)
					#pragma omp barrier
					#endif
					for ( u_int64_t i = 0; i < n; ++i )
					{
						key_type const k = mask3(T[i],K);

						if ( ! single_thread )
						{
							if ( k >= vlow && k < vhigh )
								A[ ++ghist3[k]  ] = T[i];
						}
						else
								A[ ++ghist3[k]  ] = T[i];
					}
					#if defined(_OPENMP)
					#pragma omp barrier
					#endif
					for ( u_int64_t i = 0; i < n; ++i )
					{
						key_type const k = mask4(A[i],K);

						if ( ! single_thread )
						{
							if ( k >= vlow && k < vhigh )
								T[ ++ghist4[k]  ] = A[i];
						}
						else
								T[ ++ghist4[k]  ] = A[i];
					}
					#if defined(_OPENMP)
					#pragma omp barrier
					#endif
					for ( u_int64_t i = 0; i < n; ++i )
					{
						key_type const k = mask5(T[i],K);

						if ( ! single_thread )
						{
							if ( k >= vlow && k < vhigh )
								A[ ++ghist5[k]  ] = T[i];
						}
						else
								A[ ++ghist5[k]  ] = T[i];
					}
				}
			}

		};
	}
}
#endif
