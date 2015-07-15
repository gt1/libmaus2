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
#if ! defined(LIBMAUS2_MATH_BERNOULLINUMBER_HPP)
#define LIBMAUS2_MATH_BERNOULLINUMBER_HPP

#include <libmaus2/math/binom.hpp>
#include <libmaus2/math/Rational.hpp>
#include <vector>

namespace libmaus2
{
	namespace math
	{
		struct BernoulliNumber
		{
			/**
			 * compute n'th Bernoulli number B_n
			 **/
			static Rational<> B(unsigned int const n)
			{
				std::vector< Rational<> > R;
				R.push_back(Rational<>(1));
				R.push_back(Rational<>(-1,2));

				for ( uint64_t i = 2; i <= n; ++i )
				{
					Rational<> V;
					for ( uint64_t k = 0; k < i; ++k )
						V += Rational<>(libmaus2::math::Binom::binomialCoefficientInteger(k,i+1)) * R[k];
					V /= Rational<>(i+1);
					V = -V;
					R.push_back(V);
				}
				
				return R[n];		
			}
		};
	}
}
#endif
