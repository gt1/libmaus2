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
#if ! defined(LIBMAUS2_RANDOM_LOGNORMALRANDOM_HPP)
#define LIBMAUS2_RANDOM_LOGNORMALRANDOM_HPP

#include <libmaus2/random/UniformUnitRandom.hpp>
#include <libmaus2/random/GaussianRandom.hpp>
#include <cmath>
#include <limits>
#include <vector>
#include <utility>
#include <algorithm>
#include <numeric>

namespace libmaus2
{
	namespace random
	{
		struct LogNormalRandom
		{
			/**
			 * see https://de.wikipedia.org/wiki/Fehlerfunktion
			 *
			 * used Horner's scheme to save on multiplications
			 **/
			static double tau(double const x)
			{
				double const t = 1.0 / (1+0.5*::std::abs(x));

				double const arg =
					-x*x
					-1.26551223
					+t*(
						1.00002368+
						t*(
							0.37409196
							+t*(
								0.09678418
								+t*(
									-0.18628806
									+t*(
										0.27886807
										+t*(
											-1.13520398
											+t*(
												1.48851587
												+t*(
													-0.82215223
													+t*0.17087277
												)
											)
										)
									)
								)
							)
						)
					);

				return t * ::exp(arg);
			}

			/**
			 * see https://de.wikipedia.org/wiki/Fehlerfunktion
			 **/
			static double erf(double const x)
			{
				if ( x >= 0 )
					return 1-tau(x);
				else
					return tau(-x)-1;
			}

			/* probability function of normal distribution (integral over density) */
			static double probability(double const x, double const sigma, double const mu)
			{
				if ( x < 0 )
					return 0;
				else
				{
					double const arg = (::std::log(x)-mu) / (sigma * ::std::sqrt(2.0));
					return 0.5 * (1.0 +(erf(arg)));
				}
			}

			/*
			 * search for (approximation of) smallest value v such that probability(v,sigma,mu) >= t
			 */
			static double search(double const t, double const sigma, double const mu)
			{
				double low = 0;
				double high = 1.0;

				while (
					1.0 - probability(high,sigma,mu) > 1e-6
				)
				{
					// std::cerr << "high=" << high << " val=" << probability(high,sigma,mu) << std::endl;
					high *= 2;
				}


				while ( high - low > 1e-6 )
				{
					double const mid = low + (high-low)/2.0;
					double const v = probability(mid,sigma,mu);

					if ( v < t )
						low = mid;
					else
						high = mid;
				}

				return low;
			}

			static std::pair<double,double> estimateParameters(std::vector<double> const & V)
			{
				double mu = 0.0;
				for ( uint64_t i = 0; i < V.size(); ++i )
					mu += ::std::log(V[i]);
				if ( V.size() )
					mu /= V.size();

				double sigma = 0.0;
				for ( uint64_t i = 0; i < V.size(); ++i )
					sigma += ::std::pow(::std::log(V[i])-mu,2);
				if ( V.size() )
					sigma /= V.size();

				sigma = ::std::sqrt(sigma);

				return std::pair<double,double>(mu,sigma);
			}

			static std::pair<double,double> estimateParameters(std::vector<uint64_t> const & V)
			{
				double mu = 0.0;
				for ( uint64_t i = 1; i < V.size(); ++i )
					mu += V[i] * ::std::log(static_cast<double>(i));
				if ( V.size() )
					mu /= std::accumulate(V.begin(),V.end(),0ull);

				double sigma = 0.0;
				for ( uint64_t i = 1; i < V.size(); ++i )
					sigma += V[i] * ::std::pow(::std::log(static_cast<double>(i))-mu,2);
				if ( V.size() )
					sigma /= std::accumulate(V.begin(),V.end(),0ull);

				sigma = ::std::sqrt(sigma);

				return std::pair<double,double>(mu,sigma);
			}

			static std::pair<double,double> computeParameters(double const mu, double const sigma)
			{
				double const var = sigma*sigma;
				double const vmu = ::std::log(mu*mu / std::sqrt(var + mu*mu));
				double const vvar = ::std::log(1.0 + var / (mu*mu));
				double const vsigma = ::std::sqrt(vvar);
				return std::pair<double,double>(vmu,vsigma);
			}

			static std::pair<double,double> computeAverageAndSigma(std::vector<uint64_t> const & V)
			{
				uint64_t sum = 0;
				uint64_t cnt = 0;
				for ( uint64_t i = 0; i < V.size(); ++i )
				{
					sum += i*V[i];
					cnt += V[i];
				}
				double const E = cnt ? (static_cast<double>(sum)/cnt) : 0.0;

				double vsum = 0;
				for ( uint64_t i = 0; i < V.size(); ++i )
					vsum += (i-E)*(i-E)*V[i];
				double const VAR = cnt ? (vsum / cnt) : 0.0;

				return std::pair<double,double>(E,::std::sqrt(VAR));
			}

			static std::pair<double,double> computeParameters(std::vector<uint64_t> const & V)
			{
				std::pair<double,double> const P = computeAverageAndSigma(V);
				return computeParameters(P.first,P.second);
			}

			/* produce random number from log normal distribution */
			static double random(double const sigma, double const mu)
			{
				return search(UniformUnitRandom::uniformUnitRandom(),sigma,mu);
			}
		};
	}
}
#endif
