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
#if ! defined(LIBMAUS2_UTIL_PREFIXSUMS_HPP)
#define LIBMAUS2_UTIL_PREFIXSUMS_HPP

#include <iterator>

namespace libmaus2
{
	namespace util
	{
		struct PrefixSums
		{
			template<typename iterator>
			static typename ::std::iterator_traits<iterator>::value_type prefixSums(iterator ita, iterator ite, typename ::std::iterator_traits<iterator>:: difference_type const d = 1)
			{
				typename ::std::iterator_traits<iterator>::value_type s = typename ::std::iterator_traits<iterator>::value_type();

				while ( ita != ite )
				{
					typename ::std::iterator_traits<iterator>::value_type const t = *ita;
					*ita = s;
					ita += d;
					s += t;
				}

				return s;
			}

			template<typename iterator>
			static typename ::std::iterator_traits<iterator>::value_type parallelPrefixSums(iterator ita, iterator ite, uint64_t const numthreads)
			{
				typedef typename ::std::iterator_traits<iterator>::value_type value_type;

				uint64_t const n = ite-ita;
				uint64_t const blocksize = (n + numthreads - 1)/numthreads;
				uint64_t const numblocks = (n + blocksize - 1)/blocksize;
				std::vector<value_type> B(numblocks+1);

				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads)
				#endif
				for ( uint64_t t = 0; t < numblocks; ++t )
				{
					uint64_t const low = t * blocksize;
					uint64_t const high = std::min(low+blocksize,n);

					value_type s = value_type();

					iterator l_ita = ita + low;
					iterator l_ite = ita + high;

					while ( l_ita != l_ite )
					{
						value_type const t = *l_ita;
						*(l_ita++) = s;
						s += t;
					}

					B[t] = s;
				}

				prefixSums(B.begin(),B.end());

				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads)
				#endif
				for ( uint64_t t = 0; t < numblocks; ++t )
				{
					uint64_t const low = t * blocksize;
					uint64_t const high = std::min(low+blocksize,n);

					value_type const a = B[t];

					iterator l_ita = ita + low;
					iterator l_ite = ita + high;

					while ( l_ita != l_ite )
					{
						*(l_ita++) += a;
					}
				}

				return B[numblocks];
			}
		};
	}
}
#endif
