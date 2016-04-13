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
#if ! defined(LIBMAUS2_RANK_DNARANKKMERCACHE_HPP)
#define LIBMAUS2_RANK_DNARANKKMERCACHE_HPP

#include <libmaus2/rank/DNARank.hpp>

namespace libmaus2
{
	namespace rank
	{
		struct DNARankKmerCache
		{
			typedef DNARankKmerCache this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			libmaus2::rank::DNARank const & rank;
			unsigned int const k;
			libmaus2::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > C;

			bool operator==(DNARankKmerCache const & O) const
			{
				bool ok = k == O.k && C.size() == O.C.size();

				for ( uint64_t i = 0; ok && i < C.size(); ++i )
					ok = ok && (C[i] == O.C[i]);

				return ok;
			}

			template<typename iterator>
			std::pair<uint64_t,uint64_t> searchNoCache(iterator it, size_t n) const
			{
				std::pair<uint64_t,uint64_t> P(0,rank.size());
				iterator it_e = it + n;
				for ( size_t i = 0; i < n; ++i )
				{
					int64_t m = (*(--it_e));
					if ( m < 4 )
						P = rank.backwardExtend(P,m);
					else
						P = std::pair<uint64_t,uint64_t>(0,0);
				}
				return P;
			}

			template<typename iterator>
			std::pair<uint64_t,uint64_t> search(iterator it, size_t n) const
			{
				if ( n < k )
				{
					return searchNoCache(it,n);
				}
				else
				{
					iterator it_c = it+n-k;
					uint64_t w = 0;
					bool valid = true;
					for ( size_t i = 0; i < k; ++i )
					{
						int64_t m = (it_c[i]);

						if ( m < 4 )
						{
							w <<= 2;
							w |= m;
						}
						else
						{
							valid = false;
						}
					}

					std::pair<uint64_t,uint64_t> P = valid ? C[w] : std::pair<uint64_t,uint64_t>(0,0);

					for ( size_t i = 0; i < n-k && (P.second-P.first); ++i )
					{
						int64_t m = (*(--it_c));

						if ( m < 4 )
							P = rank.backwardExtend(P,m);
						else
							P = std::pair<uint64_t,uint64_t>(0,0);
					}

					#if 0
					std::pair<uint64_t,uint64_t> const Q = searchNoCache(it,n);
					if ( P.first == P.second )
						assert ( Q.first == Q.second );
					else
						assert ( P == Q );
					#endif

					return P;
				}

			}

			template<typename iterator>
			std::pair<uint64_t,uint64_t> searchRc(iterator it, size_t n) const
			{
				if ( n < k )
				{
					std::pair<uint64_t,uint64_t> P(0,rank.size());
					for ( size_t i = 0; i < n; ++i )
					{
						int64_t m = (*(it++)) ^ 3;

						if ( m < 4 )
							P = rank.backwardExtend(P,m);
						else
							P = std::pair<uint64_t,uint64_t>(0,0);
					}
					return P;
				}
				else
				{
					uint64_t w = 0;
					bool valid = true;
					it += k;
					for ( size_t i = 0; i < k; ++i )
					{
						int64_t m = (*(--it)) ^ 3;

						if ( m < 4 )
						{
							w <<= 2;
							w |= m;
						}
						else
						{
							valid = false;
						}
					}

					std::pair<uint64_t,uint64_t> P = valid ? C[w] : std::pair<uint64_t,uint64_t>(0,0);

					it += k;
					for ( size_t i = 0; i < n-k && (P.second-P.first); ++i )
					{
						int64_t m = (*(it++)) ^ 3;

						if ( m < 4 )
							P = rank.backwardExtend(P,m);
						else
							P = std::pair<uint64_t,uint64_t>(0,0);
					}

					return P;
				}

			}

			static unsigned int increment(libmaus2::autoarray::AutoArray<char> & S, unsigned int const end)
			{
				unsigned int p = 0;

				// skip over 3 ('T') symbols
				while ( p < end && S[p] == 3 )
					++p;

				// if we found any non 'T' symbol
				if ( p < end )
				{
					// sanity check
					assert ( S[p] < 3 );

					// increment position p
					S[p] += 1;
					// set everything left of p to zero
					for ( unsigned int i = 0; i < p; ++i )
						S[i] = 0;
				}

				return p;
			}

			DNARankKmerCache(libmaus2::rank::DNARank const & rrank, unsigned int const rk, uint64_t numthreads = 1)
			: rank(rrank), k(rk), C( 1ull << (2*k))
			{
				if ( ! numthreads )
					numthreads = 1;

				unsigned int threaddigits = 0;
				uint64_t threadparts = 1;

				while ( threadparts < numthreads && threaddigits < k )
				{
					threadparts <<= 2;
					threaddigits += 1;
				}

				// mask vector
				libmaus2::autoarray::AutoArray<uint64_t> M(k);
				for ( unsigned int i = 0; i < k; ++i )
					M[i] = libmaus2::math::lowbits( (k-i-1)*2 );

				libmaus2::autoarray::AutoArray < libmaus2::autoarray::AutoArray<char>::unique_ptr_type > AS(numthreads);
				libmaus2::autoarray::AutoArray < libmaus2::autoarray::AutoArray< std::pair<uint64_t,uint64_t> >::unique_ptr_type > AP(numthreads);

				for ( uint64_t i = 0; i < numthreads; ++i )
				{
					libmaus2::autoarray::AutoArray<char>::unique_ptr_type TS(new libmaus2::autoarray::AutoArray<char>(k));
					AS[i] = UNIQUE_PTR_MOVE(TS);
					libmaus2::autoarray::AutoArray< std::pair<uint64_t,uint64_t> >::unique_ptr_type TA(new libmaus2::autoarray::AutoArray< std::pair<uint64_t,uint64_t> >(k+1));
					AP[i] = UNIQUE_PTR_MOVE(TA);
					(*AP[i])[k] = std::pair<uint64_t,uint64_t>(0,rank.size());
				}

				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads)
				#endif
				for ( uint64_t z = 0; z < threadparts; ++z )
				{
					#if defined(_OPENMP)
					uint64_t const tid = omp_get_thread_num();
					#else
					uint64_t const tid = 0;
					#endif

					// symbol vector
					libmaus2::autoarray::AutoArray<char> & S = *AS[tid];
					std::fill(S.begin(),S.end(),0);

					uint64_t zz = z;
					for ( uint64_t i = 0; i < threaddigits; ++i )
					{
						S[k-i-1] = zz & 3;
						zz >>= 2;
					}

					// interval vector
					libmaus2::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > & P = *AP[tid];

					// set whole range for empty word
					P[k] = std::pair<uint64_t,uint64_t>(0,rank.size());
					// extend for current word in S
					for ( unsigned int i = 0; i < k; ++i )
						P[k-i-1] = rank.backwardExtend(P[k-i],S[k-i-1]);

					uint64_t v = 0;
					for ( unsigned int i = 0; i < k; ++i )
					{
						v <<= 2;
						v |= S[i];
					}

					bool running = true;
					while ( running )
					{
						#if 0
						uint64_t w = 0;
						for ( unsigned int i = 0; i < k; ++i )
						{
							w <<= 2;
							w |= S[i];
						}

						assert ( v == w );
						#endif

						// set cache value
						C[v] = P[0];

						unsigned int const p = increment(S,k-threaddigits);

						#if 0
						std::cerr << v << "\t";
						for ( unsigned int i = 0; i < k; ++i )
							std::cerr.put(libmaus2::fastx::remapChar(S[i]));
						std::cerr << std::endl;
						#endif

						if ( p < k-threaddigits )
						{
							for ( unsigned int i = 0; i <= p; ++i )
								P[p-i] = rank.backwardExtend(P[p-i+1],S[p-i]);

							#if 0
							std::pair<uint64_t,uint64_t> ref(0,rank.size());
							for ( unsigned int i = 0; i < k; ++i )
								ref = rank.backwardExtend(ref,S[k-i-1]);

							assert ( P[0] == ref );
							#endif

							v &= M[p];
							v |= static_cast<uint64_t>(S[p]) << ((k-p-1)*2);
						}
						else
						{
							running = false;
						}
					}
				}
			}
		};
	}
}
#endif
