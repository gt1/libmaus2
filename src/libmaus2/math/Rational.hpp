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
				if ( a < 0 )
					a = -a;
				if ( b < 0 )
					b = -b;
					
				while ( b )
				{
					N const ob = b;
					b = a % b;
					a = ob;
				}
				
				return a;
			}

			static N lcm(N a, N b)
			{
				return (a*b)/gcd(a,b);
			}

			N c;
			N d;
			
			Rational() : c(0), d(1) {}
			Rational(N const rc) : c(rc), d(1) {}
			Rational(N const rc, N const rd) : c(rc), d(rd) 
			{
				if ( d == N() )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "Rational<>: zero denominator" << std::endl;
					lme.finish();
					throw lme;
				}

				if ( d < 0 )
				{
					d = -d;
					c = -c;
				}
				
				N const g = gcd(c,d);
				c /= g;
				d /= g;
				
				assert ( d > 0 );
			}
			
			Rational<N> & operator=(Rational<N> const & o)
			{
				if ( this != &o )
				{
					c = o.c;
					d = o.d;
				}
				return *this;
			}
			
			Rational<N> & operator+=(Rational<N> const & o)
			{
				N tc = c;
				N td = d;
				N oc = o.c;
				N od = o.d;
				
				N sd = lcm(td,od);
				tc *= (sd / td);
				oc *= (sd / od);
				N sc = tc + oc;
				
				N const g = gcd(sc,sd);
				
				sc /= g;
				sd /= g;
				
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
				tc *= (sd / td);
				oc *= (sd / od);
				N sc = tc - oc;
				
				N const g = gcd(sc,sd);
				
				sc /= g;
				sd /= g;
				
				c = sc;
				d = sd;
				
				return *this;
			}
			
			Rational<N> & operator*=(Rational<N> const & o)
			{
				N oc = o.c;
				N od = o.d;
				
				N const g_c_od = gcd(c,od);
				c /= g_c_od;
				od /= g_c_od;
				
				N const g_oc_d = gcd(oc,d);
				oc /= g_oc_d;
				d /= g_oc_d;
				
				c *= oc;
				d *= od;
				
				assert ( gcd(c,d) == 1 );
								
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
				c /= g_c_od;
				od /= g_c_od;
				
				N const g_oc_d = gcd(oc,d);
				oc /= g_oc_d;
				d /= g_oc_d;
				
				c *= oc;
				d *= od;

				assert ( gcd(c,d) == 1 );
				
				return *this;
			}
			
			operator double() const
			{
				return static_cast<double>(c) / static_cast<double>(d);
			}
			
			bool operator<(Rational<N> const & O) const
			{
				return (*this - O).c < N();
			}
			bool operator<=(Rational<N> const & O) const
			{
				return (*this - O).c <= N();
			}
			bool operator>(Rational<N> const & O) const
			{
				return (*this - O).c > N();
			}
			bool operator>=(Rational<N> const & O) const
			{
				return (*this - O).c >= N();
			}
			bool operator==(Rational<N> const & O) const
			{
				return (*this - O).c == N();
			}
			bool operator!=(Rational<N> const & O) const
			{
				return (*this - O).c != N();
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
