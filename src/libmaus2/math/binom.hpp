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

#if ! defined(BINOM_HPP)
#define BINOM_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/math/gcd.hpp>
#include <libmaus2/math/Rational.hpp>
#include <libmaus2/math/GmpInteger.hpp>
#include <libmaus2/math/GmpFloat.hpp>
#include <vector>
#include <cassert>
#include <queue>
#include <cmath>

namespace libmaus2
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

			static libmaus2::math::Rational<libmaus2::math::GmpInteger> binomialCoefficientAsRational(uint64_t k, uint64_t n)
			{
				std::vector < uint64_t > cnt;
				fillBinomialVector(k,n,cnt);

				libmaus2::math::Rational<libmaus2::math::GmpInteger> R(libmaus2::math::GmpInteger(1));

				if ( k == 0 || k == n )
					return R;

				for ( uint64_t i = 0; i < cnt.size(); ++i )
					R *= libmaus2::math::Rational<libmaus2::math::GmpInteger>(libmaus2::math::GmpInteger(cnt[i]));

				return R;
			}

			static uint64_t binomialCoefficientInteger(uint64_t k, uint64_t n)
			{
				std::vector < uint64_t > cnt;
				fillBinomialVector(k,n,cnt);
				uint64_t v = 1;
				for ( uint64_t i = 0; i < cnt.size(); ++i )
					v *= cnt[i];
				return v;
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

			static GmpFloat slowPow(GmpFloat const & x, unsigned int const e, unsigned int const prec)
			{
				GmpFloat tx = x;
				GmpFloat r(1,prec);

				for ( uint64_t i = 1; i <= e; i <<=1 )
				{
					if ( i & e )
						r *= tx;
					tx = tx*tx;
				}

				return r;
			}

			static libmaus2::math::Rational<libmaus2::math::GmpInteger> slowPow(libmaus2::math::Rational<libmaus2::math::GmpInteger> const x, unsigned int const e)
			{
				libmaus2::math::Rational<libmaus2::math::GmpInteger> tx = x;
				libmaus2::math::Rational<libmaus2::math::GmpInteger> r(libmaus2::math::GmpInteger(1));

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

			static libmaus2::math::Rational<libmaus2::math::GmpInteger> binomSingleAsRational(libmaus2::math::Rational<libmaus2::math::GmpInteger> const p, uint64_t const k, uint64_t const n)
			{
				return binomialCoefficientAsRational(k,n) * slowPow(p,k) * slowPow(libmaus2::math::Rational<libmaus2::math::GmpInteger>(libmaus2::math::GmpInteger(1))-p,n-k);
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

			static libmaus2::math::Rational<libmaus2::math::GmpInteger> binomRowAsRational(libmaus2::math::Rational<libmaus2::math::GmpInteger> const p, uint64_t const k, uint64_t const n)
			{
				libmaus2::math::Rational<libmaus2::math::GmpInteger> r(libmaus2::math::GmpInteger(0));
				libmaus2::math::Rational<libmaus2::math::GmpInteger> const q = libmaus2::math::Rational<libmaus2::math::GmpInteger>(libmaus2::math::GmpInteger(1))-p;
				libmaus2::math::Rational<libmaus2::math::GmpInteger> const tp = libmaus2::math::Rational<libmaus2::math::GmpInteger>(libmaus2::math::GmpInteger(1));
				libmaus2::math::Rational<libmaus2::math::GmpInteger> const tq = slowPow(q,n);
				libmaus2::math::Rational<libmaus2::math::GmpInteger> f = tp * tq;

				for ( uint64_t i = 0; i <= k; ++i )
				{
					r += f;
					f *= p;
					f /= q;
					f /= (libmaus2::math::Rational<libmaus2::math::GmpInteger>(libmaus2::math::GmpInteger(i))+libmaus2::math::Rational<libmaus2::math::GmpInteger>(libmaus2::math::GmpInteger(1)));
					f *= (libmaus2::math::Rational<libmaus2::math::GmpInteger>(libmaus2::math::GmpInteger(n))-libmaus2::math::Rational<libmaus2::math::GmpInteger>(libmaus2::math::GmpInteger(i)));
				}
				return r;
			}

			static double binomRowUpper(double const p, uint64_t const k, uint64_t const n)
			{
				double r = 0;
				double const q = 1-p;
				double const tp = 1.0;
				double const tq = slowPow(q,n);
				double f = 1.0 * tp * tq;

				for ( uint64_t i = 0; i < k; ++i )
				{
					f *= p;
					f /= q;
					f /= (i+1);
					f *= (n-i);
				}

				for ( uint64_t i = k; i <= n; ++i )
				{
					r += f;
					f *= p;
					f /= q;
					f /= (i+1);
					f *= (n-i);
				}

				return r;
			}

			static libmaus2::math::GmpFloat binomRowUpperGmpFloat(libmaus2::math::GmpFloat const p, uint64_t const k, uint64_t const n, unsigned int const prec)
			{
				libmaus2::math::GmpFloat r(0,prec);
				libmaus2::math::GmpFloat const q = libmaus2::math::GmpFloat(1.0,prec)-p; // q = 1-p
				libmaus2::math::GmpFloat const tp(1.0,prec); // tp = 1
				libmaus2::math::GmpFloat const tq = slowPow(q,n,prec); // tq = q^n
				libmaus2::math::GmpFloat f = tp * tq; // product of tp and tq

				for ( uint64_t i = 0; i < k; ++i )
				{
					f *= p; // multiply factor by p
					f /= q; // divide factor by q
					f /= (libmaus2::math::GmpFloat(i,prec)+libmaus2::math::GmpFloat(1,prec)); // divide by i+1
					f *= (libmaus2::math::GmpFloat(n,prec)-libmaus2::math::GmpFloat(i,prec)); // multiply by n-i
				}

				// sum up starting from k
				for ( uint64_t i = k; i <= n; ++i )
				{
					r += f; // add f to result
					f *= p; // multiply factor by p
					f /= q; // divide factor by q
					f /= (libmaus2::math::GmpFloat(i,prec)+libmaus2::math::GmpFloat(1,prec)); // divide by i+1
					f *= (libmaus2::math::GmpFloat(n,prec)-libmaus2::math::GmpFloat(i,prec)); // multiply by n-i
				}

				return r;
			}

			static std::vector < libmaus2::math::GmpFloat > binomVector(libmaus2::math::GmpFloat const p, uint64_t const n, unsigned int const prec)
			{
				//libmaus2::math::GmpFloat r(0,prec);
				libmaus2::math::GmpFloat const q = libmaus2::math::GmpFloat(1.0,prec)-p; // q = 1-p
				libmaus2::math::GmpFloat const tp(1.0,prec); // tp = 1
				libmaus2::math::GmpFloat const tq = slowPow(q,n,prec); // tq = q^n
				libmaus2::math::GmpFloat f = tp * tq; // product of tp and tq

				std::vector < libmaus2::math::GmpFloat > V(n+1);

				// sum up starting from k
				for ( uint64_t k = 0; k <= n; ++k )
				{
					V[k] = f;
					// r += f; // add f to result
					f *= p; // multiply factor by p
					f /= q; // divide factor by q
					f /= (libmaus2::math::GmpFloat(k,prec)+libmaus2::math::GmpFloat(1,prec)); // divide by k+1
					f *= (libmaus2::math::GmpFloat(n,prec)-libmaus2::math::GmpFloat(k,prec)); // multiply by n-k
				}

				return V;
			}

			/**
			 * search for maximum k s.t. binomRowUpperGmpFloat(p,n,k,prec) >= lim using binary search
			 **/
			static uint64_t binomRowUpperGmpFloatLimit(libmaus2::math::GmpFloat const p, uint64_t const n, unsigned int const prec, libmaus2::math::GmpFloat const lim)
			{
				uint64_t low = 0;
				uint64_t high = n+1;

				while ( high-low > 1 )
				{
					uint64_t const m = (high + low)>>1;
					libmaus2::math::GmpFloat const pp = binomRowUpperGmpFloat(p,m,n,prec);

					// m is valid
					if ( pp >= lim )
						low = m;
					// m is invalid
					else
						high = m;
				}

				return low;
			}

			static libmaus2::math::Rational<libmaus2::math::GmpInteger> binomRowUpperAsRational(libmaus2::math::Rational<libmaus2::math::GmpInteger> const p, uint64_t const k, uint64_t const n)
			{
				libmaus2::math::Rational<libmaus2::math::GmpInteger> r = libmaus2::math::Rational<libmaus2::math::GmpInteger>(libmaus2::math::GmpInteger(0));
				libmaus2::math::Rational<libmaus2::math::GmpInteger> const q = libmaus2::math::Rational<libmaus2::math::GmpInteger>(libmaus2::math::GmpInteger(1))-p;
				libmaus2::math::Rational<libmaus2::math::GmpInteger> const tp = libmaus2::math::Rational<libmaus2::math::GmpInteger>(libmaus2::math::GmpInteger(1));;
				libmaus2::math::Rational<libmaus2::math::GmpInteger> const tq = slowPow(q,n);
				libmaus2::math::Rational<libmaus2::math::GmpInteger> f = tp * tq;

				for ( uint64_t i = 0; i < k; ++i )
				{
					f *= p;
					f /= q;
					f /= (libmaus2::math::Rational<libmaus2::math::GmpInteger>(libmaus2::math::GmpInteger(i))+libmaus2::math::Rational<libmaus2::math::GmpInteger>(libmaus2::math::GmpInteger(1)));
					f *= (libmaus2::math::Rational<libmaus2::math::GmpInteger>(libmaus2::math::GmpInteger(n))-libmaus2::math::Rational<libmaus2::math::GmpInteger>(libmaus2::math::GmpInteger(i)));
				}

				for ( uint64_t i = k; i <= n; ++i )
				{
					r += f;
					f *= p;
					f /= q;
					f /= (libmaus2::math::Rational<libmaus2::math::GmpInteger>(libmaus2::math::GmpInteger(i))+libmaus2::math::Rational<libmaus2::math::GmpInteger>(libmaus2::math::GmpInteger(1)));
					f *= (libmaus2::math::Rational<libmaus2::math::GmpInteger>(libmaus2::math::GmpInteger(n))-libmaus2::math::Rational<libmaus2::math::GmpInteger>(libmaus2::math::GmpInteger(i)));
				}

				return r;
			}

			static bool increment(std::vector<uint64_t> & D, uint64_t top)
			{
				int64_t i = static_cast<int64_t>(D.size())-1;

				while ( i >= 0 && D[i] == top )
					--i;

				if ( i < 0 )
				{
					return false;
				}
				else
				{
					D[i++] += 1;
					while ( i < static_cast<int64_t>(D.size()) )
						D[i++] = 0;

					return true;
				}
			}

			static std::vector< libmaus2::math::GmpFloat > multiDimBinomial(double const p, uint64_t const n, unsigned int const d)
			{
				std::vector < libmaus2::math::GmpFloat > BV = libmaus2::math::Binom::binomVector(p /* prob for correct kmer */, n, 512);

				std::vector<uint64_t> D(d);
				std::vector< libmaus2::math::GmpFloat > S(d*n+1);

				do
				{
					libmaus2::math::GmpFloat mult = 1;
					uint64_t isum = 0;

					for ( uint64_t i = 0; i < D.size(); ++i )
					{
						isum += D[i];
						mult *= BV[D[i]];
					}

					S [ isum ] += mult;
				} while ( increment(D,n) );


				return S;
			}
		};
	}
}
#endif
