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
			typedef Poisson this_type;

			double const d_lambda;
			double const e_lambda;
			double const l_lambda;

			static double fac(uint64_t const v)
			{
				return ::std::exp(
#if defined(__APPLE__)
					::lgamma(v+1)
#else
					::std::lgamma(v+1)
#endif
				);
			}

			Poisson(double const rlambda)
			: d_lambda(rlambda), e_lambda(std::exp(-d_lambda)), l_lambda(::std::log(d_lambda))
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

			uint64_t thresLow(double const thres, uint64_t const limit = 100) const
			{
				double s = 0.0;
				uint64_t i = 0;
				uint64_t const bailout = limit * d_lambda;

				for ( uint64_t i = 0; true; ++i )
				{
					double const v = poissonViaLog(i);

					if ( s + v > thres || i >= bailout )
						return i;

					s += v;
				}

				#if 0
				double f = e_lambda;
				double s = 0.0;
				std::vector<double> V;

				for ( uint64_t i = 1; true; ++i )
				{
					if ( s + f >= thres )
						return i-1;
					s += f;
					f *= (d_lambda/i);
				}
				#endif
			}

			double thresLowInv(uint64_t const thres) const
			{
				double f = e_lambda;
				double s = 0.0;

				for ( uint64_t i = 1; i <= thres+1; ++i )
				{
					s += f;
					f *= (d_lambda/i);
				}

				return s;
			}

			// compute value in log space to keep intermediate results under control
			double poissonViaLog(uint64_t const k) const
			{
				return ::std::exp(k * l_lambda - d_lambda -
#if defined(__APPLE__)
					lgamma(k+1)
#else
					::std::lgamma(k+1)
#endif
				);
			}

			double thresLowInvLog(uint64_t const thres) const
			{
				double s = 0.0;

				for ( uint64_t i = 0; i <= thres; ++i )
					s += poissonViaLog(i);

				return s;
			}

			/**
			 * get vector for arguments 0,1,2,... until sum over tail of series falls below thres
			 **/
			std::vector<double> getVectorViaLog(double const thres) const
			{
				double s = 1.0;
				std::vector<double> V;

				for ( uint64_t i = 0; s > thres; ++i )
				{
					double const f = poissonViaLog(i);
					V.push_back(f);
					s -= f;
				}

				return V;
			}

			double lowerSum(uint64_t const n) const
			{
				double s = 0.0;
				for ( uint64_t i = 0; i < n; ++i )
					s += poissonViaLog(i);
				return s;
			}

			static double gamma_q(uint64_t const n, double const lambda)
			{
				this_type PO(lambda);
				return PO.lowerSum(n);
			}
		};
	}
}
#endif
