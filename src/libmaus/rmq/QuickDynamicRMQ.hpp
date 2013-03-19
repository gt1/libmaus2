/**
    libmaus
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
**/

#if ! defined(QUICKDYNAMICRMQ_HPP)
#define QUICKDYNAMICRMQ_HPP

#include <limits>
#include <cassert>
#include <libmaus/autoarray/AutoArray.hpp>

namespace libmaus
{
	namespace rmq
	{
		// Berkman, Vishkin Range Minimum Query class
		// setup time O(n log n)
		// space O(n log n) words
		// query time O(1)
		template<typename array_iterator>
		struct QuickDynamicRMQ
		{
			typedef QuickDynamicRMQ<array_iterator> this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type; 

			typedef typename std::iterator_traits<array_iterator>::value_type key_type;
			array_iterator const A;

			// base 2 logarithm rownded to zero (-1 means -inf)
			static inline int logbindown(uint32_t k)
			{
				int c = -1;
					
				while ( k & (~0xFFFFul) ) c += 16, k >>= 16;
				if ( k & (~0xFFul)      ) c += 8, k>>=8;
				if ( k & (~0xFul)       ) c += 4, k>>=4;
				if ( k & (~0x3ul)       ) c += 2, k>>=2;
				if ( k & (~0x1ul)       ) c += 1, k>>=1;
				if ( k                  ) c += 1, k>>=1;
					
				return c;
			}

			uint32_t const n;
			uint32_t const d;
			::libmaus::autoarray::AutoArray<uint32_t> M;

			uint32_t m(uint32_t h, uint32_t i) const { return h?M[(h-1)*n+i]:i; }
				
			QuickDynamicRMQ(array_iterator rA, uint32_t const rn)
			: A(rA), n(rn), d(static_cast<uint32_t>(logbindown(n)))
			{
				if ( n )
				{
					// allocate table
					M = ::libmaus::autoarray::AutoArray<uint32_t>(n*d);

					// first row of matrix is identity
					// fill rest of rows using dynamic programming
					uint32_t offset1 = 0;
					
					for ( uint32_t j = 1; j <= d; ++j )
					{
						uint32_t i00 = 0, i01 = (1<<(j-1)), i0e = n, i1a = offset1;
						uint32_t const i1e = i1a+n;

						for ( ; i01 != i0e; ++i00,++i01,++i1a )
							if ( A[m(j-1,i00)] <= A[m(j-1,i01)] )
								M[i1a] = m(j-1,i00);
							else
								M[i1a] = m(j-1,i01);

						for ( ; i1a != i1e; ++i00,++i1a )
								M[i1a] = m(j-1,i00);
						
						offset1 += n;
					}
				}
			}

			bool regressionTest()
			{
				for ( uint32_t i = 0; i < n; ++i )
					for ( uint32_t j = 0; j <= i; ++j )
					{
						// left is j, right is i
						key_type m = std::numeric_limits<key_type>::max();

						for ( uint32_t q = j; q <= i; ++ q )
							m = std::min(m,A[q]);
							
						if ( m != A[(*this)(j,i)] )
							return false;
					}	

				return true;
			}

			// O(1) range query
			uint32_t operator()(uint32_t l, uint32_t r) const
			{
				assert(r>=l && r<n);

				if ( r-l == 0 ) return l;

				uint32_t const h = static_cast<uint32_t>(logbindown(r-l));
			
				uint32_t const i0 = m(h,l);
				uint32_t const i1 = m(h,r - (1<<h) + 1);
				
				if ( A[i0] <= A[i1] )
					return i0;
				else
					return i1;
			}
		};
	}
}
#endif
