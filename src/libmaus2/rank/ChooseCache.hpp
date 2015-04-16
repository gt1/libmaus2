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

#if ! defined(CHOOSECACHE_HPP)
#define CHOOSECACHE_HPP

#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/util/unique_ptr.hpp>

namespace libmaus2
{
	namespace rank
	{
		/**
		 * cache for choose operations (n choose k)
		 **/
		struct ChooseCache
		{
			typedef ChooseCache this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			private:
			unsigned int const n;
			::libmaus2::autoarray::AutoArray<unsigned int> C;

			static unsigned int o(unsigned int const i) { return (i*(i+1))>>1; }
			static unsigned int s(unsigned int const i) { return o(i+1); }
			
			inline static unsigned int fak(unsigned int i)
			{
				if ( i <= 1 )
					return 1;
				else
					return i*fak(i-1);
			}
			
			public:	
			/**
			 * return binomial coefficient kk from nn
			 * @param nn
			 * @param kk
			 * @return nn choose kk
			 **/
			inline unsigned int operator()(unsigned int nn, unsigned int kk) const
			{
				if ( kk <= nn )
					return C [ o(nn) + (kk) ];
				else
					return 0;
			}
			/**
			 * return sum over binomial coefficients for nn
			 * @param nn
			 * @return (n 0) + (n 1) + ... (n n)
			 **/
			inline unsigned int sum(unsigned int nn)
			{
				unsigned int s = 0;
				for ( unsigned int kk = 0; kk <= nn; ++kk )
					s += (*this)(nn,kk);
				return s;
			}

			/**
			 * construct choose operations cache for n=0,...,rn
			 * @param rn
			 **/
			ChooseCache(unsigned int rn)
			: n(rn), C( s(n) )
			{
				C[0] = 1;
				
				for ( unsigned int nn = 1; nn <= n; ++nn )
				{
					unsigned int const * S = C.get() + o(nn-1);
					unsigned int * T = C.get() + o(nn);

					*(T++) = 1;
					for ( unsigned int kk = 1; kk < nn ; ++kk, ++S, ++T )
						*T = (*S) + (*(S+1));
					*(T++) = 1;
				}
			}
			
			/**
			 * encode B of length b bits with u significant bits in enumerative code
			 * @param B number
			 * @param u number of significant bits in B
			 * @param b number of bits in B
			 * @return enumerative code for B
			 **/
			template<typename N>
			N encode(N B, unsigned int u, unsigned int const b = 8*sizeof(N)) const
			{
				unsigned int x = 0;
				unsigned int m = 1u<<(b-1);
				
				for ( unsigned int i = 0; i < b; ++i, m>>=1 )
				{
					if ( B & m )
					{
						x += (*this)(b-i-1,u);
						--u;
					}	
				}
				return x;
			}
			/**
			 * decode value v (enumerative code) with u significant bits
			 * @param v
			 * @param u number of significant bits
			 * @return decoded number
			 **/
			template<typename N>
			N decode(N v, unsigned int u) const
			{
				N q = 0;
				
				for ( unsigned int i = 0; i < 8*sizeof(v); ++i )
					if ( v >= (*this)(8*sizeof(v)-i-1,u) )
					{
						q |= (1u<<(8*sizeof(v)-i-1));
						v -= (*this)(8*sizeof(v)-i-1,u);
						--u;
					}
					
				return q;
			}
		};
	}
}
#endif
