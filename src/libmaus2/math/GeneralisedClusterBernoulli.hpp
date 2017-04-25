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
#if !defined(LIBMAUS2_MATH_GENERALISEDCLUSTERBERNOULLI_HPP)
#define LIBMAUS2_MATH_GENERALISEDCLUSTERBERNOULLI_HPP

#include <vector>
#include <stack>
#include <libmaus2/math/binom.hpp>

namespace libmaus2
{
	namespace math
	{
		/**
		 * generalasised Bernoulli class with event classes
		 * each event class has a success probability and a number of times it can occur at most
		 **/
		struct GeneralisedClusterBernoulli
		{
			// cluster centers
			std::vector<double> const & KM;
			// instances in each cluster class
			std::vector<uint64_t> const & KCNT;
			// suffix sums over KCNT
			std::vector<uint64_t> S;
			// Bernoulli cache for each cluster centre
			std::vector < std::vector<double> > D;
			// suffix sums over eval
			std::vector < double > E;

			// search for largest index s.t. suffix sum is at least p
			int64_t search(double const p) const
			{
				int64_t i = E.size()-1;

				while ( i >= 0 )
					if ( E[i] >= p )
						return i;
					else
						--i;

				return 0;
			}

			/**
			 * rKM: cluster centre probabilities
			 * rKCNT: count assigned to cluster centre
			 **/
			GeneralisedClusterBernoulli(std::vector<double> const & rKM, std::vector<uint64_t> const & rKCNT)
			: KM(rKM), KCNT(rKCNT), S(KCNT.size())
			{
				uint64_t s = 0;
				for ( uint64_t ii = 0; ii < KCNT.size(); ++ii )
				{
					// i steps from back to front
					uint64_t const i = KCNT.size() - ii - 1;
					// add count
					s += KCNT[i];
					// i and above have count s
					S[i] = s;
				}

				// binom single value vectors
				for ( uint64_t i = 0; i < KM.size(); ++i )
					D.push_back(libmaus2::math::Binom::binomVectorDouble(KM[i],KCNT[i]));

				// eval cache
				if ( KCNT.size() )
					for ( uint64_t i = 0; i <= S[0]; ++i )
						E.push_back(eval(i));

				// compute suffix sums
				double su = 0;
				for ( uint64_t ii = 0; ii < E.size(); ++ii )
				{
					uint64_t const i = E.size()-ii-1;
					su += E[i];
					E[i] = su;
				}
			}

			// evaluation stack element
			struct StackElement
			{
				uint64_t i;
				uint64_t c;
				double v;

				StackElement() {}
				StackElement(uint64_t const ri, uint64_t const rc, double const rv) : i(ri), c(rc), v(rv) {}
			};

			// compute result for exactly c successful trials
			double eval(uint64_t const c) const
			{
				assert ( (!c) || (S.size() && c <= S[0]) );
				std::stack<StackElement> ST;
				ST.push(StackElement(0 /* next index on S */,c /* successful trials left */,1.0 /* result */));
				double s = 0.0;

				while ( ! ST.empty() )
				{
					StackElement const T = ST.top();
					ST.pop();

					if ( T.i == KCNT.size() )
					{
						assert ( T.c == 0 );
						s += T.v;
					}
					else
					{
						assert ( T.c <= S[T.i] );

						uint64_t const r = (T.i + 1 < KCNT.size()) ? S[T.i+1] : 0;

						for ( uint64_t i = ((T.c >= r) ? (T.c - r) : 0); i <= KCNT[T.i] && i <= T.c; ++i )
							ST.push(StackElement(T.i+1,T.c-i,T.v * D.at(T.i).at(i)));
					}
				}

				return s;
			}
		};
	}
}
#endif
