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
#if !defined(LIBMAUS2_MATH_GENERALISEDBERNOULLI_HPP)
#define LIBMAUS2_MATH_GENERALISEDBERNOULLI_HPP

#include <vector>
#include <stack>
#include <cassert>
#include <libmaus2/types/types.hpp>

namespace libmaus2
{
	namespace math
	{
		/**
		 * generalised Bernoulli. Instead of each trial having the same probability this
		 * class allows each one to have a different success/failure probability
		 **/
		struct GeneralisedBernoulli
		{
			// trail success probability vector
			std::vector<double> const & PV;

			// constructor by probability vector
			GeneralisedBernoulli(std::vector<double> const & rPV) : PV(rPV) {}

			// evaluation support element
			struct StackElement
			{
				uint64_t i;
				uint64_t c0;
				uint64_t c1;
				double v;

				StackElement() {}
				StackElement(uint64_t const ri, uint64_t const rc0, uint64_t const rc1, double const rv)
				: i(ri), c0(rc0), c1(rc1), v(rv)
				{
				}
			};

			// compute probability to see exactly c successful trials
			double operator()(uint64_t const c) const
			{
				if ( c > PV.size() )
					return 0.0;

				std::stack<StackElement> S;
				S.push(StackElement(0,PV.size()-c /* c0 */,c /* c1 */,1.0));
				double s = 0.0;

				while ( ! S.empty() )
				{
					StackElement const T = S.top();
					S.pop();

					if ( T.i == PV.size() )
					{
						assert ( T.c0 + T.c1 == 0 );
						s += T.v;
					}
					else
					{
						if ( T.c1 )
						{
							S.push(
								StackElement(
									T.i+1,
									T.c0,
									T.c1-1,
									T.v * PV[T.i]
								)
							);
						}
						if ( T.c0 )
						{
							S.push(
								StackElement(
									T.i+1,
									T.c0-1,
									T.c1,
									T.v * (1.0-PV[T.i])
								)
							);
						}
					}
				}

				return s;
			}
		};
	}
}
#endif
