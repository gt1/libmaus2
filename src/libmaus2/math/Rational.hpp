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
#if ! defined(LIBMAUS2_MATH_RATIONAL_HPP)
#define LIBMAUS2_MATH_RATIONAL_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/util/shared_ptr.hpp>
#include <libmaus2/math/gcd.hpp>
#include <libmaus2/exception/LibMausException.hpp>
#include <cassert>
#include <cmath>

namespace libmaus2
{
	namespace math
	{
		template<typename _N>
		struct Rational;
		template<typename _N>
		std::ostream & operator<<(std::ostream & out, Rational<_N> const & R);
		template<typename N>
		Rational<N> operator+(Rational<N> const & A, Rational<N> const & B);
		template<typename N>
		Rational<N> operator-(Rational<N> const & A, Rational<N> const & B);
		template<typename N>
		Rational<N> operator*(Rational<N> const & A, Rational<N> const & B);
		template<typename N>
		Rational<N> operator/(Rational<N> const & A, Rational<N> const & B);

		template<typename _N = int64_t>
		struct Rational
		{
			typedef _N N;
			typedef Rational<N> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			static N gcd(N a, N b)
			{
				if ( a < N() )
					a = -a;
				if ( b < N() )
					b = -b;

				while ( b != N() )
				{
					N const ob = b;
					b = a % b;
					a = ob;
				}

				return a;
			}

			static N lcm(N a, N b)
			{
				assert ( a != N() );
				assert ( b != N() );

				N const g = gcd(a,b);
				N l = (a*b)/g;

				if ( (!(l == N())) && ((l % a) == N()) && ((l % b) == N()) )
					return l;

				l = (a/g)*(b/g);

				if ( (!(l == N())) && ((l % a) == N()) && ((l % b) == N()) )
					return l;

				libmaus2::exception::LibMausException lme;
				lme.getStream() << "Rational<>::lcm: unable to compute exact result for a=" << a << " b=" << b << std::endl;
				lme.finish();
				throw lme;
			}

			static N mult(N a, N b)
			{
				N p = a * b;

				if ( (a == N()) || (b == N()) )
					return p;

				if (
					((p % a) == N())
					&&
					((p % b) == N())
					&&
					((p / a) == b)
					&&
					((p / b) == a)
				)
					return p;

				libmaus2::exception::LibMausException lme;
				lme.getStream() << "Rational<>::mult: unable to compute exact result for a=" << a << " b=" << b << std::endl;
				lme.finish();
				throw lme;
			}

			N c;
			N d;

			Rational() : c(N()), d(N(1)) {}
			Rational(N const rc) : c(rc), d(N(1)) {}
			Rational(N const rc, N const rd) : c(rc), d(rd)
			{
				if ( d == N() )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "Rational<>: zero denominator" << std::endl;
					lme.finish();
					throw lme;
				}

				if ( d < N() )
				{
					d = -d;
					c = -c;
				}

				N const g = gcd(c,d);
				c = (c/g);
				d = (d/g);

				assert ( !(d < N()) );
			}

			static Rational<N> doubleToRational(double v, unsigned int const bindig = 8*sizeof(double))
			{
				Rational<N> R(N(),N(1));
				bool neg = false;
				if ( v < 0.0 )
				{
					neg = true;
					v = -v;
				}

				double const intv = std::floor(v);
				R.c = N(static_cast<int64_t>(intv));
				v -= intv;

				for ( unsigned int i = 1; i <= bindig && v; ++i )
				{
					v *= 2.0;
					double const fl = std::floor(v);
					v -= fl;
					R += Rational(N(static_cast<int>(fl)),N(static_cast<int64_t>(1ull << i)));
				}

				if ( neg )
					R.c = -R.c;

				return R;
			}

			Rational<N> & operator=(Rational<N> const & o)
			{
				if ( ! (this == &o) )
				{
					c = o.c;
					d = o.d;
				}
				return *this;
			}

			Rational<N> & operator+=(Rational<N> const & o)
			{
				assert ( d != N() );
				assert ( o.d != N() );

				N tc = c;
				N td = d;
				N oc = o.c;
				N od = o.d;

				N sd = lcm(td,od);
				tc = mult(tc,(sd / td));
				oc = mult(oc,(sd / od));
				N sc = tc + oc;

				N const g = gcd(sc,sd);

				sc = (sc/g);
				sd = (sd/g);

				c = sc;
				d = sd;

				return *this;
			}

			Rational<N> & operator-=(Rational<N> const & o)
			{
				N tc = c;
				N td = d;
				N oc = o.c;
				N od = o.d;

				N sd = lcm(td,od);
				tc = mult(tc,(sd / td));
				oc = mult(oc,(sd / od));
				N sc = tc - oc;

				N const g = gcd(sc,sd);

				sc = sc/g;
				sd = sd/g;

				c = sc;
				d = sd;

				return *this;
			}

			Rational<N> & operator*=(Rational<N> const & o)
			{
				N oc = o.c;
				N od = o.d;

				N const g_c_od = gcd(c,od);
				c = c/g_c_od;
				od = od/g_c_od;

				N const g_oc_d = gcd(oc,d);
				oc = oc/g_oc_d;
				d = d/g_oc_d;

				c = mult(c,oc);
				d = mult(d,od);

				assert ( gcd(c,d) == N(1) );

				return *this;
			}

			Rational<N> & operator/=(Rational<N> const & o)
			{
				if ( o.c == N() )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "Rational<>::operator/=(): division by zero" << std::endl;
					lme.finish();
					throw lme;
				}

				N oc = o.d;
				N od = o.c;

				N const g_c_od = gcd(c,od);
				c = c/g_c_od;
				od = od/g_c_od;

				N const g_oc_d = gcd(oc,d);
				oc = oc/g_oc_d;
				d = d/g_oc_d;

				c = mult(c,oc);
				d = mult(d,od);

				assert ( gcd(c,d) == N(1) );

				return *this;
			}

			operator double() const
			{
				return static_cast<double>(c) / static_cast<double>(d);
			}

			double toDouble(unsigned int const bindig = 64) const
			{
				Rational<N> R = *this;

				if ( R.c == N() )
					return 0;

				bool neg = R.c < N();

				if ( neg )
					R.c = -R.c;

				N intv = R.c / R.d;
				R.c -= R.d * intv;

				double v = 0.0;
				v += static_cast<double>(intv);

				assert ( R.c < R.d );

				double a = 0.5;
				for ( unsigned int d = 1; d <= bindig; ++d )
				{
					R.c *= N(2);

					if ( R.c >= R.d )
					{
						R.c -= R.d;
						assert ( R.c < R.d );
						v += a;
					}

					a *= 0.5;
				}

				if ( neg )
					v = -v;

				return v;
			}

			bool operator<(Rational<N> const & O) const
			{
				return (*this - O).c < N();
			}
			bool operator<=(Rational<N> const & O) const
			{
				return
					(*this < O)
					||
					(*this == O);
			}
			bool operator>(Rational<N> const & O) const
			{
				return N() < (*this - O).c;
			}
			bool operator>=(Rational<N> const & O) const
			{
				return
					(*this > O)
					||
					(*this == O);
			}
			bool operator==(Rational<N> const & O) const
			{
				return (*this - O).c == N();
			}
			bool operator!=(Rational<N> const & O) const
			{
				return !(*this == O);
			}

			Rational<N> operator-() const
			{
				return Rational<N>(-c,d);
			}
		};

		template<typename N>
		Rational<N> operator+(Rational<N> const & A, Rational<N> const & B)
		{
			Rational<N> R = A;
			R += B;
			return R;
		}

		template<typename N>
		Rational<N> operator-(Rational<N> const & A, Rational<N> const & B)
		{
			Rational<N> R = A;
			R -= B;
			return R;
		}

		template<typename N>
		Rational<N> operator*(Rational<N> const & A, Rational<N> const & B)
		{
			Rational<N> R = A;
			R *= B;
			return R;
		}

		template<typename N>
		Rational<N> operator/(Rational<N> const & A, Rational<N> const & B)
		{
			Rational<N> R = A;
			R /= B;
			return R;
		}


		template<typename _N>
		std::ostream & operator<<(std::ostream & out, Rational<_N> const & R)
		{
			return out << R.c << "/" << R.d;
		}
	}
}
#endif
