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
#if ! defined(LIBMAUS2_RANDOM_GAUSSIANRANDOM_HPP)
#define LIBMAUS2_RANDOM_GAUSSIANRANDOM_HPP

#include <libmaus2/random/UniformUnitRandom.hpp>
#include <cmath>

namespace libmaus2
{
	namespace random
	{
		struct GaussianRandom
		{
			/* density function of normal distribution */
			static double gaussian(double const x, double const sigma, double const mu)
			{
				return 1.0 / sqrt(2*M_PI*sigma*sigma) * ::exp ( - (x-mu)*(x-mu) / (2.0*sigma*sigma));
			}

			/* faculty function i! */
			static double fak(int i)
			{
				double f = 1;
				for ( int j = 2; j <= i; ++j )
					f *= static_cast<double>(j);
				return f;
			}

			/* series for erf computation via series expansion */
			static double erfseries(int i)
			{
				return (2*i+1)*fak(i);
			}
			/* precomputed series for erf computation via series expansion */
			static double const series[20];
			/* length of prefix to be used when computing erf via series expansion */
			static int const seriesuse;
			/* pre computed pre factor */
			static double const pref;
			/* compute erf function via series expansion */
			static double erfViaSeries(double z)
			{
				double a = z;
				double z2 = z*z;
				for ( int i = 1; i < seriesuse; ++i )
				{
					z *= z2;
					a += series[i] * z;
				}
				return pref * a;
			}

			/* sign function */
			static double sgn(double z)
			{
				if ( z < 0 )
					return -1;
				else if ( z > 0 )
					return 1;
				else if ( std::abs(0) == 0 )
					return 0;
				else
					return z;
			}

			/* see http://de.wikipedia.org/wiki/Fehlerfunktion */
			static double erf(double z)
			{
				return
					2.0/std::sqrt(M_PI) * sgn(z) * std::sqrt(1-std::exp(-z*z)) *
					(
						std::sqrt(M_PI) / 2 + 31.0/200.0 * std::exp(-z*z) - 341.0 / 8000.0 * std::exp(-2*z*z)
					);
			}

			/* probability function of normal distribution (integral over density) */
			static double probability(double const x, double const sigma, double const mu)
			{
				if ( x >= mu )
					return 0.5*(1.0+GaussianRandom::erf((x-mu)/(std::sqrt(2*sigma*sigma)) ));
				else
					return 1 - 0.5*(1.0+GaussianRandom::erf((mu-x)/(std::sqrt(2*sigma*sigma)) ));
			}

			/*
			 * search for (approximation of) smallest value v such that probability(v,sigma,mu) >= t
			 */
			static double search(double const t, double const sigma, double const mu)
			{
				double low = mu - 10*sigma, high = mu + 10*sigma;

				while ( high - low > 1e-6 )
				{
					double const mid = (high+low)/2.0;
					double const v = probability(mid,sigma,mu);

					if ( v < t )
						low = mid;
					else
						high = mid;
				}

				return low;
			}


			/* produce random number from Gaussian distribution */
			static double random(double const sigma, double const mu)
			{
				return search(UniformUnitRandom::uniformUnitRandom(),sigma,mu);
			}
		};
	}
}
#endif
