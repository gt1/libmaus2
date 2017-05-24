/*
    libmaus2
    Copyright (C) 2017 German Tischler

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
#if ! defined(LIBMAUS2_MATH_POISSON_HPP)
#define LIBMAUS2_MATH_POISSON_HPP

#include <libmaus2/math/ipow.hpp>
#include <vector>
#include <cmath>

/**
 * Poisson distribution
 **/
namespace libmaus2
{
	namespace random
	{
		struct Poisson
		{
			uint64_t const lambda;
			double const d_lambda;
			double const e_lambda;

			static double fac(uint64_t const v)
			{
				return ::std::exp(::lgamma(v+1));
			}

			Poisson(uint64_t const rlambda)
			: lambda(rlambda), d_lambda(lambda), e_lambda(std::exp(-d_lambda))
			{
			}

			double operator()(uint64_t const k) const
			{
				double s = e_lambda;

				for ( uint64_t i = 1; i <= k; ++i )
					s *= (d_lambda/i);

				return s;
			}

			/**
			 * get vector for arguments 0,1,2,... until sum over tail of series falls below thres
			 **/
			std::vector<double> getVector(double const thres) const
			{
				double f = e_lambda;
				double s = 1.0;
				std::vector<double> V;

				for ( uint64_t i = 1; s > thres; ++i )
				{
					V.push_back(f);
					s -= f;
					f *= (d_lambda/i);
				}

				return V;
			}
		};
	}
}
#endif
