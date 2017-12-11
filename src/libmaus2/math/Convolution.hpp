/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#if ! defined(LIBMAUS2_MATH_CONVOLUTION_HPP)
#define LIBMAUS2_MATH_CONVOLUTION_HPP

#include <vector>
#include <cassert>
#include <numeric>
#include <algorithm>
#include <complex>
#include <libmaus2/types/types.hpp>
#include <libmaus2/math/ilog.hpp>
#include <libmaus2/math/numbits.hpp>

#include <iostream>

namespace libmaus2
{
	namespace math
	{
		struct Convolution
		{
			// deinterleave array in place (i.e. turn 0,1,2,3,4,5,6,7 to 0,2,4,6,1,3,5,7)
			template<typename iterator>
			static void deinterleave(iterator A, uint64_t const n)
			{
				for ( uint64_t blocksize = 1; 2*blocksize < n; blocksize <<= 1 )
				{
					uint64_t low = 0;

					while ( low+2*blocksize < n )
					{
						uint64_t const a = low;
						uint64_t const b = a+blocksize;
						uint64_t const c = b+blocksize;
						uint64_t const r = std::min(blocksize<<1,n-c);
						uint64_t const r_0 = (r+1)/2;
						uint64_t const r_1 = r-r_0;
						uint64_t const d = c + r_0;
						uint64_t const e = d + r_1;

						std::reverse(A+b,A+c);
						std::reverse(A+c,A+d);
						std::reverse(A+b,A+d);

						low = e;
					}
				}
			}

			// reorder by index bit order reversal in place. Takes time O(n log^2 n)
			template<typename iterator>
			static void bitReverseReorder(iterator A, uint64_t const n)
			{
				assert ( (1ull << libmaus2::math::ilog(n)) == n );

				for ( uint64_t bs = n; bs; bs >>= 1 )
				{
					uint64_t const nb = n / bs;

					for ( uint64_t i = 0; i < nb; ++i )
						deinterleave(A + (i+0)*bs,bs);
				}
			}

			// compute FFT of complex input vector in place
			// input size is required to be a power of two
			template<typename iterator>
			static void fft(iterator A, uint64_t const n, bool const reverse = false)
			{
				uint64_t const slog = libmaus2::math::ilog(n);
				assert ( (1ull << slog) == n );

				bitReverseReorder(A,n);

				for ( uint64_t s = 1; s <= slog; ++s )
				{
					uint64_t const m = 1ull << s;
					uint64_t const m2 = (m >> 1);

					double const expo = (reverse ? -1 : 1) * ((2.0*M_PI) / static_cast<double>(m));
					std::complex<double> omega_m(::std::cos(expo),::std::sin(expo));

					for ( uint64_t k = 0; k < n; k += m )
					{
						std::complex<double> omega(1.0,0.0);
						for ( uint64_t j = 0; j < m2; ++j )
						{
							std::complex<double> const t = omega * A [ k + j + m2 ];
							std::complex<double> const u = A [ k + j ];
							A [ k + j ] = u + t;
							A [ k + j + m2 ] = u - t;
							omega = omega * omega_m;
						}
					}
				}
			}

			template<typename type_1, typename type_2>
			static std::vector<double> convolutionFFTRef(
				std::vector<type_1> const & RA,
				std::vector<type_2> const & RB
			)
			{
				uint64_t const na = RA.size();
				uint64_t const nb = RB.size();

				if ( na * nb == 0 )
					return std::vector<double>(0);

				uint64_t const rn = na + nb - 1;
				uint64_t const rn2 = libmaus2::math::nextTwoPow(rn);

				std::vector < std::complex<double> > VA(rn2);
				for ( uint64_t i = 0; i < na; ++i )
					VA[i] = RA[i];
				std::vector < std::complex<double> > VB(rn2);
				for ( uint64_t i = 0; i < nb; ++i )
					VB[i] = RB[i];

				fft(VA.begin(),VA.size());
				fft(VB.begin(),VB.size());

				for ( uint64_t i = 0; i < VA.size(); ++i )
					VA[i] *= VB[i];

				fft(VA.begin(),VA.size(),true);

				std::vector<double> VC(rn);
				double const div = 1.0 / rn2;
				for ( uint64_t i = 0; i < VC.size(); ++i )
					VC[i] = VA[i].real() * div;

				return VC;
			}

			static std::vector < std::vector < double > > computePowers(std::vector < double > const & P_I, uint64_t const n)
			{
				std::vector < double > C_I(1,1.0);

				std::vector < std::vector < double > > R;

				for ( uint64_t i = 0; i <= n; ++i )
				{
					R.push_back(C_I);

					if ( i == 0 )
					{
						C_I = P_I; // C_I = P_I ^ 1 for i = 1
					}
					else
					{
						C_I = convolutionFFTRef(C_I,C_I);
					}
				}

				return R;
			}

			// cache for convolution powers storing self convolutions A^k for integer k >= 0
			struct PowerCache
			{
				typedef PowerCache this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				std::vector < libmaus2::util::shared_ptr < std::vector < double > >::type > R;
				libmaus2::parallel::PosixSpinLock Rlock;
				double e;

				PowerCache() {}
				PowerCache(std::vector < double > const & P_I, double const re = std::numeric_limits<double>::min())
				: e(re)
				{
					libmaus2::util::shared_ptr < std::vector < double > >::type sptr(
						new std::vector < double >(P_I)
					);
					R.push_back(sptr);
				}

				std::vector < double > const & getR(uint64_t const j)
				{
					std::vector < double > const *  P = 0;

					{
						libmaus2::parallel::ScopePosixSpinLock slock(Rlock);

						while ( ! (j < R.size()) )
						{
							std::vector < double > const T = convolutionFFTRef(*(R.back()),*(R.back()));
							libmaus2::util::shared_ptr < std::vector < double > >::type sptr(new std::vector < double >(T));

							while ( sptr->size() && sptr->back() < e )
								sptr->pop_back();

							R.push_back(sptr);
						}

						assert ( j < R.size() );


						P = R [ j ].get();
					}

					return *P;
				}

				std::vector < double > operator[](uint64_t const i)
				{
					std::vector < double > A(1,1.0);

					for ( uint64_t j = 0, ti = i; ti; ++j, ti /= 2 )
						if ( ti & 1 )
						{
							A = convolutionFFTRef(A,getR(j));

							while ( A.size() && A.back() < e )
								A.pop_back();
						}

					return A;
				}
			};

			struct PowerCacheRussian
			{
				typedef PowerCacheRussian this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				uint64_t const n;
				libmaus2::autoarray::AutoArray < std::vector<double> > A;
				double const e;

				PowerCacheRussian(
					std::vector < double > const & P_I,
					unsigned int const l, unsigned int const s,
					uint64_t const numthreads,
					double const re = std::numeric_limits<double>::min()
				)
				: n(1ull << l), A(n), e(re)
				{
					PowerCache PC(P_I,e);

					assert ( 0 < n );
					A[0] = PC[0];

					if ( l )
					{
						assert ( 1 < n );
						A[1] = PC[1ull << s];

						for ( uint64_t q = 1; q < l; ++q )
						{
							uint64_t const base = 1ull << q;
							std::vector<double> const Q = PC[base << s];

							#if defined(_OPENMP)
							#pragma omp parallel for schedule(dynamic,1) num_threads(numthreads)
							#endif
							for ( uint64_t i = 0; i < base; ++i )
							{
								// std::cerr << "[V] setting " << base+i << " from " << i << std::endl;
								A[base + i] = convolutionFFTRef(A[i],Q);

								while ( A[base + i].size() && A[base + i].back() < e )
									A[base + i].pop_back();
							}
						}
					}
				}

				std::vector<double> const & operator[](uint64_t const i) const
				{
					assert ( i < n );
					return A[i];
				}
			};

			struct PowerBlockCache
			{
				typedef PowerBlockCache this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				std::vector<double> const P_I;
				unsigned int const blocksize;
				std::vector < PowerCacheRussian::shared_ptr_type > V;
				libmaus2::parallel::PosixSpinLock Vlock;
				uint64_t const numthreads;
				double const e;

				PowerBlockCache(
					std::vector<double> const & rP_I,
					unsigned int const rblocksize,
					uint64_t const rnumthreads,
					double const re = std::numeric_limits<double>::min()
				)
				: P_I(rP_I), blocksize(rblocksize), numthreads(rnumthreads), e(re) {}

				PowerCacheRussian const & getBlock(unsigned int const i)
				{
					PowerCacheRussian const * P = 0;
					{
						libmaus2::parallel::ScopePosixSpinLock slock(Vlock);

						while ( ! (i < V.size()) )
						{
							uint64_t const j = V.size();
							PowerCacheRussian::shared_ptr_type PCR(
								new PowerCacheRussian(
									P_I,
									blocksize,
									j * blocksize,
									numthreads,
									e
								)
							);

							// std::cerr << "[V] generated " << i << std::endl;

							V.push_back(PCR);
						}

						assert ( i < V.size() );

						P = V[i].get();
					}

					return *P;
				}

				std::vector<double> operator[](uint64_t const i)
				{
					uint64_t const mask = (1ull << blocksize)-1;

					std::vector < double> V = getBlock(0)[i&mask];

					for ( uint64_t q = 1;
						(i >> (blocksize *q)) != 0;
						++q
					)
					{
						V = convolutionFFTRef(V,getBlock(q)[(i >> (blocksize *q)) & mask]);

						while ( V.size() && V.back() < e )
							V.pop_back();
					}

					return V;
				}
			};

			template<typename type>
			static std::vector<type> convolution(
				std::vector<type> const & A,
				std::vector<type> const & B
			)
			{
				// max value A.size()-1 + B.size() - 1 = A.size()+B.size()-2
				std::vector<type> R((A.size() * B.size()) ? (A.size()+B.size()-1) : 0);

				for ( uint64_t i = 0; i < R.size(); ++i )
				{
					type s = 0.0;

					// j >= 0
					// i-j < B.size() -> i-j+1 <= B.size() -> i-B.size()+1 <= j -> j >= i-B.size()+1

					uint64_t const start = (i >= B.size()-1) ? (i-B.size()+1) : 0;
					uint64_t const end = std::min(i+1,static_cast<uint64_t>(A.size()));

					for ( uint64_t j = start; j < end; ++j )
					{
						assert ( j < A.size() );
						assert ( i-j < B.size() );
						s += A[j] * B[i-j];
					}

					R[i] = s;
				}

				return R;
			}

			static std::vector<double> convolutionFFTW(
				std::vector<double> const & RA,
				std::vector<double> const & RB
			);

			template<typename type_1, typename type_2>
			static std::vector<double> convolutionFFT(
				std::vector<type_1> const & RA,
				std::vector<type_2> const & RB
			)
			{
				std::vector<double> const A(RA.begin(),RA.end());
				std::vector<double> const B(RB.begin(),RB.end());

				#if defined(LIBMAUS2_HAVE_FFTW)
				return convolutionFFTW(A,B);
				#else
				// return convolution(A,B);
				return convolutionFFTRef(A,B);
				#endif
			}

			static void cleanup();
		};
	}
}
#endif
