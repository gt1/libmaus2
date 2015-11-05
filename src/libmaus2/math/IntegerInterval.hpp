/*
    libmaus2
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS2_MATH_INTEGERINTERVAL_HPP)
#define LIBMAUS2_MATH_INTEGERINTERVAL_HPP

#include <libmaus2/types/types.hpp>
#include <ostream>
#include <numeric>
#include <limits>
#include <algorithm>
#include <cassert>
#include <vector>

namespace libmaus2
{
	namespace math
	{
		template<typename _N=int64_t>
		struct IntegerInterval
		{
			typedef _N N;

			N from;
			N to;

			IntegerInterval() : from(0), to(0) {}
			IntegerInterval(N const rfrom, N const rto) : from(rfrom), to(rto) {}

			bool operator<(IntegerInterval<N> const & O) const
			{
				if ( from != O.from )
					return from < O.from;
				else
					return to < O.to;
			}

			bool isEmpty() const
			{
				return to < from;
			}

			static IntegerInterval<N> empty()
			{
				return IntegerInterval<N>(
					std::numeric_limits<N>::max(),
					std::numeric_limits<N>::min()
				);
			}

			static N absdiff(N const a, N const b)
			{
				if ( a >= b )
					return a-b;
				else
					return b-a;
			}

			static N hausdorffSlow(IntegerInterval<N> const & A, IntegerInterval<N> const & B)
			{
				if ( A.isEmpty() )
				{
					if ( B.isEmpty() )
						return N();
					else
						return std::numeric_limits<N>::max();
				}

				N maxdiff = 0;

				for ( N i = A.from; i <= A.to; ++i )
				{
					N mindiff = std::numeric_limits<N>::max();
					for ( N j = B.from; j <= B.to; ++j )
						mindiff = std::min(mindiff,absdiff(i,j));
					maxdiff = std::max(maxdiff,mindiff);
				}
				for ( N i = B.from; i <= B.to; ++i )
				{
					N mindiff = std::numeric_limits<N>::max();
					for ( N j = A.from; j <= A.to; ++j )
						mindiff = std::min(mindiff,absdiff(i,j));
					maxdiff = std::max(maxdiff,mindiff);
				}

				return maxdiff;
			}

			static N hausdorffDistance(IntegerInterval<N> const & A, IntegerInterval<N> const & B)
			{
				if ( A.isEmpty() )
				{
					if ( B.isEmpty() )
						return N();
					else
						return std::numeric_limits<N>::max();
				}

				return
					std::max(
						std::max(
							std::min(absdiff(A.from,B.from),absdiff(A.from,B.to)),
							std::min(absdiff(A.to,B.from),absdiff(A.to,B.to))
						),
						std::max(
							std::min(absdiff(B.from,A.from),absdiff(B.from,A.to)),
							std::min(absdiff(B.to,A.from),absdiff(B.to,A.to))
						)
					);
			}

			static IntegerInterval<N> intersection(IntegerInterval<N> const & A, IntegerInterval<N> const & B)
			{
				if ( A.isEmpty() || B.isEmpty() )
					return empty();
				if ( A.from > B.from )
					return intersection(B,A);

				assert ( A.from <= B.from );

				if ( A.to < B.from )
					return empty();

				return IntegerInterval<N>(B.from,std::min(B.to,A.to));
			}

			static IntegerInterval<N> span(IntegerInterval<N> const & A, IntegerInterval<N> const & B)
			{
				if ( A.isEmpty() )
					return B;
				else if ( B.isEmpty() )
					return A;
				else
				{
					if ( A.from > B.from )
						return span(B,A);
					assert ( A.from <= B.from );

					return IntegerInterval<N>(A.from, std::max(A.to,B.to));
				}
			}

			IntegerInterval<N> intersection(IntegerInterval<N> const & B) const
			{
				return intersection(*this,B);
			}

			struct IntegerIntervalComparator
			{
				bool operator()(IntegerInterval<N> const & A, IntegerInterval<N> const & B)
				{
					if ( A.from != B.from )
						return A.from < B.from;
					else
						return A.to < B.to;
				}
			};

			static std::vector < IntegerInterval<N> > mergeOverlapping(std::vector< IntegerInterval<N> > IV)
			{
				std::sort(IV.begin(),IV.end(),IntegerIntervalComparator());
				std::vector < IntegerInterval<N> > R;

				uint64_t low = 0;
				while ( low != IV.size() )
				{
					uint64_t high = low+1;
					IntegerInterval<N> merged = IV[low];

					while ( high != IV.size() && (!(merged.intersection(IV[high]).isEmpty())) )
					{
						// set new upper bound
						merged.to = std::max(merged.to,IV[high].to);
						++high;
					}

					R.push_back(merged);

					low = high;
				}

				return R;
			}

			static std::vector < IntegerInterval<N> > mergeOverlappingFuzzy(std::vector< IntegerInterval<N> > IV, N const fuzz)
			{
				std::sort(IV.begin(),IV.end(),IntegerIntervalComparator());
				std::vector < IntegerInterval<N> > R;

				uint64_t low = 0;
				while ( low != IV.size() )
				{
					uint64_t high = low+1;
					IntegerInterval<N> merged = IV[low];

					while ( high != IV.size() && (merged.to + fuzz >= IV[high].from) )
					{
						// set new upper bound
						merged.to = std::max(merged.to,IV[high].to);
						++high;
					}

					R.push_back(merged);

					low = high;
				}

				return R;
			}

			bool contains(IntegerInterval<N> const & I) const
			{
				if ( I.isEmpty() )
				{
					return true;
				}
				else if ( isEmpty() )
				{
					return false;
				}
				else
				{
					return from <= I.from && to >= I.to;
				}
			}

			bool operator==(IntegerInterval<N> const & I) const
			{
				if ( isEmpty() )
					return I.isEmpty();
				else
					return from == I.from && to == I.to;
			}

			N diameter() const
			{
				if ( from <= to )
					return (to-from+N(1));
				else
					return N();
			}
		};

		template<typename N>
		std::ostream & operator<<(std::ostream & out, IntegerInterval<N> const & II)
		{
			if ( II.from <= II.to )
				return out << '[' << II.from << "," << II.to << ']';
			else
				return out << "[empty]";
		};
	}
}
#endif
