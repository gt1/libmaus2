/*
    libmaus
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
#if ! defined(LIBMAUS_MATH_INTEGERINTERVAL_HPP)
#define LIBMAUS_MATH_INTEGERINTERVAL_HPP

#include <libmaus/types/types.hpp>
#include <numeric>
#include <limits>
#include <algorithm>
#include <cassert>

namespace libmaus
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
		};
	}
}
#endif
