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
#if ! defined(LIBMAUS2_MATH_FAULHABER_HPP)
#define LIBMAUS2_MATH_FAULHABER_HPP

#include <libmaus2/math/BernoulliNumber.hpp>

namespace libmaus2
{
	namespace math
	{
		struct Faulhaber
		{
			/**
			 * compute Faulhaber polynomial coefficients for power p
			 **/
			static std::vector < Rational<> > polynomial(unsigned int const p)
			{
				Rational<> m(-1,1);
				Rational<> pre(1,1);
				std::vector< Rational<> > P(p+2,Rational<>(1,p+1));
				// no constant
				P[0] = Rational<>(0,1);
				
				for ( unsigned int j = 0; j <= p; ++j )
				{
					Rational<> coeff = pre;
					coeff *= Rational<>(libmaus2::math::Binom::binomialCoefficientInteger(j,p+1));
					coeff *= libmaus2::math::BernoulliNumber::B(j);
					P[p+1-j] *= coeff;
					
					pre *= m;
				}
				
				return P;
			}
		};
	}
}
#endif
