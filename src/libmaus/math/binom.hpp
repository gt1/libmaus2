/*
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
*/

#if ! defined(BINOM_HPP)
#define BINOM_HPP

#include <libmaus/types/types.hpp>
#include <libmaus/math/gcd.hpp>
#include <vector>
#include <cassert>
#include <queue>
#include <cmath>

namespace libmaus
{
	namespace math
	{
		struct Binom
		{
			static void fillBinomialVector(uint64_t const k, uint64_t const n, std::vector < uint64_t > & cnt)
			{
				if ( n-k < k )
					return fillBinomialVector(n-k,n,cnt);

				cnt.resize(0);
				std::vector < uint64_t > den;

				for ( uint64_t i = 1; i <= k; ++i )
				{
					cnt.push_back(n-(k-i));
					den.push_back(i);
				}

				for ( uint64_t i = 0; i < den.size(); ++i )
					for ( uint64_t j = 0; j < cnt.size(); ++j )
					{
						uint64_t const q = gcd(den[i],cnt[j]);
						den[i] /= q;
						cnt[j] /= q;
					}
				
				for ( uint64_t i = 0; i < den.size(); ++i )
					assert ( den[i] == 1 );
			}

			static double binomialCoefficient(uint64_t k, uint64_t n)
			{
				std::vector < uint64_t > cnt;
				fillBinomialVector(k,n,cnt);
				
				if ( k == 0 || k == n )
					return 1.0;

				std::priority_queue < double, std::vector<double>, std::greater<double> > H;
				for ( uint64_t i = 0; i < cnt.size(); ++i )
					H.push(cnt[i]);

				while ( H.size() > 1 )
				{
					double const d0 = H.top(); H.pop();
					double const d1 = H.top(); H.pop();
					H.push( d0*d1 );
				}
				
				return H.top();
			}
			
			static double slowPow(double const x, unsigned int const e)
			{
				double tx = x;
				double r = 1;
				
				for ( uint64_t i = 1; i <= e; i <<=1 )
				{
					if ( i & e )
						r *= tx;
					tx = tx*tx;
				}
				
				return r;
			}
			
			static double binomSingle(double const p, uint64_t const k, uint64_t const n)
			{
				return 
					binomialCoefficient(k,n) * 
					slowPow(p,k) *
					slowPow(1-p,n-k);
			}
			
			static double binomRow(double const p, uint64_t const k, uint64_t const n)
			{
				double r = 0;
				double const q = 1-p;
				double const tp = 1.0;
				double const tq = slowPow(q,n);
				double f = 1.0 * tp * tq;
				
				for ( uint64_t i = 0; i <= k; ++i )
				{
					r += f;
					f *= p;
					f /= q;
					f /= (i+1);
					f *= (n-i);
				}
				return r;
			}
		};
	}
}
#endif
