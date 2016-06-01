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
#include <libmaus2/autoarray/AutoArray.hpp>

#if defined(_OPENMP)
#include <omp.h>
#endif

namespace libmaus2
{
	namespace rank
	{
		/**
		 * rank class storing explicit results round robin
		 **/
		template<typename _value_type, typename _rank_type>
		struct FastRank
		{
			typedef _value_type value_type;
			typedef _rank_type rank_type;
			typedef FastRank<value_type,rank_type> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			uint64_t n;
			// array for input sequence
			libmaus2::autoarray::AutoArray<value_type> AV;
			// input sequence pointer
			value_type * V;
			// rank array
			libmaus2::autoarray::AutoArray<rank_type> R;
			// maximum symbol
			int64_t maxsym;
			// next power of two following maxsym+1
			uint64_t mod;
			// mod-1
			uint64_t mask;
			// inverse bit mask for mask (top bits not used for storing maxsym+1)
			uint64_t invmask;

			uint64_t rank(int64_t const sym, uint64_t const i) const
			{
				if ( expect_false(sym > maxsym) )
					return 0;

				uint64_t j, r;
				if ( expect_false(static_cast<int64_t>(i) < sym) )
				{
					j = 0;
					r = (V[0]==sym);
				}
				else
				{
					j = ((i-sym)&invmask)+sym;
					r = R[j];
				}

				while ( j != i )
					r += (V[++j] == sym);

				return r;
			}

			template<typename iterator>
			FastRank(
				iterator it,
				uint64_t const rn,
				int64_t rmaxsym = std::numeric_limits<int64_t>::min()
			)
			: n(rn), AV(n,false), V(AV.begin())
			{
				#if defined(_OPENMP)
				uint64_t const numthreads = omp_get_max_threads();
				#else
				uint64_t const numthreads = 1;
				#endif

				libmaus2::parallel::OMPLock lock;

				if ( rmaxsym == std::numeric_limits<int64_t>::min() )
				{
					uint64_t nperpack = (n+numthreads-1)/numthreads;
					int64_t  packs = (n + nperpack - 1)/nperpack;

					#if defined(_OPENMP)
					#pragma omp parallel for num_threads(numthreads)
					#endif
					for ( int64_t p = 0; p < packs; ++p )
					{
						lock.lock();
						int64_t lmax = std::numeric_limits<int64_t>::min();
						lock.unlock();

						uint64_t const low = p * nperpack;
						uint64_t const high = std::min(low+nperpack,n);

						iterator itc = it + low;
						iterator ite = it + high;
						value_type * O = V + low;

						for ( ; itc != ite; ++itc )
						{
							lmax = std::max(
								lmax,static_cast<int64_t>(*itc)
								);
							*(O++) = *itc;
						}

						lock.lock();
						rmaxsym = std::max(rmaxsym,lmax);
						lock.unlock();
					}

					maxsym = rmaxsym;

					if ( n > 0 && maxsym < 0 )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "FastRank: maximum symbol code is smaller than 0" << std::endl;
						lme.finish();
						throw lme;
					}

					#if 0
					// std::cerr << "maxsym=" << maxsym << std::endl;
					for ( uint64_t i = 0; i < n; ++i )
						assert ( V[i] == it[i] );
					#endif

					mod = libmaus2::math::nextTwoPow(maxsym+1);
					mask = mod-1;
					invmask = ~mask;

					uint64_t const div = libmaus2::math::lcm(numthreads,mod);
					nperpack = (n+div-1)/div;
					packs = (n + nperpack - 1)/nperpack;

					// std::cerr << "mod=" << mod << " mask=" << std::hex << mask << " invmask=" << invmask << std::dec << std::endl;

					libmaus2::autoarray::AutoArray<uint64_t> Alhist(mod * packs,false);

					#if defined(_OPENMP)
					#pragma omp parallel for num_threads(numthreads)
					#endif
					for ( int64_t p = 0; p < packs; ++p )
					{
						uint64_t * const lhist = Alhist.begin() + p * mod;
						std::fill(lhist,lhist+mod,0ull);

						uint64_t const low = p * nperpack;
						uint64_t const high = std::min(low+nperpack,n);

						for ( value_type * i = V + low; i != V + high; ++i )
							lhist[*i]++;
					}

					// compute prefix sums
					for ( uint64_t i = 0; i < mod; ++i )
					{
						uint64_t acc = 0;

						for ( int64_t j = 0; j < packs; ++j )
						{
							uint64_t const t = Alhist[
								j * mod + i
							];
							Alhist[j * mod + i] = acc;
							acc += t;
						}
					}

					#if 0
					for ( int64_t p = 0; p < packs; ++p )
					{
						uint64_t * const lhist = Alhist.begin() + p * mod;

						uint64_t const low = p * nperpack;

						std::map<value_type,uint64_t> M;
						for ( uint64_t i = 0; i < low; ++i )
							M [ it[i] ]++;

						for ( uint64_t i = 0; i < mod; ++i )
							assert ( lhist[i] == M[i] );
					}
					#endif

					R = libmaus2::autoarray::AutoArray<rank_type>(n,false);

					#if defined(_OPENMP)
					#pragma omp parallel for num_threads(numthreads)
					#endif
					for ( int64_t p = 0; p < packs; ++p )
					{
						uint64_t * const lhist = Alhist.begin() + p * mod;

						uint64_t const low = p * nperpack;
						uint64_t const high = std::min(low+nperpack,n);

						for ( uint64_t i = low ; i < high; ++i )
						{
							value_type const sym = V[i];
							lhist[sym]++;
							R[i] = lhist[i & mask];
						}
					}

				}
				else // if ( rmaxsym >= 0 )
				{
					maxsym = rmaxsym;
					mod = libmaus2::math::nextTwoPow(maxsym+1);
					mask = mod-1;
					invmask = ~mask;

					uint64_t const div = libmaus2::math::lcm(numthreads,mod);
					uint64_t const nperpack = (n+div-1)/div;
					int64_t const packs = (n + nperpack - 1)/nperpack;

					// block/thread local histograms
					libmaus2::autoarray::AutoArray<uint64_t> Alhist(mod * packs,false);

					// copy data and compute histograms
					#if defined(_OPENMP)
					#pragma omp parallel for num_threads(numthreads)
					#endif
					for ( int64_t p = 0; p < packs; ++p )
					{
						uint64_t * const lhist = Alhist.begin() + p * mod;
						std::fill(lhist,lhist+mod,0ull);

						uint64_t const low = p * nperpack;
						uint64_t const high = std::min(low+nperpack,n);

						iterator itc = it + low;
						iterator ite = it + high;
						value_type * O = V + low;

						for ( ; itc != ite; ++itc )
						{
							value_type const sym = *itc;
							*(O++) = sym;
							lhist[sym]++;
						}
					}

					// compute prefix sums
					for ( uint64_t i = 0; i < mod; ++i )
					{
						uint64_t acc = 0;

						for ( int64_t j = 0; j < packs; ++j )
						{
							uint64_t const t = Alhist[
								j * mod + i
							];
							Alhist[j * mod + i] = acc;
							acc += t;
						}
					}

					// compute R array
					R = libmaus2::autoarray::AutoArray<rank_type>(n,false);

					#if defined(_OPENMP)
					#pragma omp parallel for num_threads(numthreads)
					#endif
					for ( int64_t p = 0; p < packs; ++p )
					{
						uint64_t * const lhist = Alhist.begin() + p * mod;

						uint64_t const low = p * nperpack;
						uint64_t const high = std::min(low+nperpack,n);

						for ( uint64_t i = low ; i < high; ++i )
						{
							value_type const sym = V[i];
							lhist[sym]++;
							R[i] = lhist[i & mask];
						}
					}
				}

				#if 0
				std::map<value_type,rank_type> M;
				for ( uint64_t i = 0; i < n; ++i )
				{
					M[it[i]]++;
					assert ( R[i] == M[i & mask] );
				}
				#endif
			}
		};
	}
}
