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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_TRACEBLOCK_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_TRACEBLOCK_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/math/IntegerInterval.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct TraceBlock
			{
				std::pair<int64_t,int64_t> A;
				std::pair<int64_t,int64_t> B;
				
				TraceBlock() {}
				TraceBlock(
					std::pair<int64_t,int64_t> const & rA,
					std::pair<int64_t,int64_t> const & rB		
				) : A(rA), B(rB) {}
				
				libmaus2::math::IntegerInterval<int64_t> getAInterval() const
				{
					return libmaus2::math::IntegerInterval<int64_t>(A.first,A.second-1);
				}

				libmaus2::math::IntegerInterval<int64_t> getBInterval() const
				{
					return libmaus2::math::IntegerInterval<int64_t>(B.first,B.second-1);
				}
				
				bool overlapsA(TraceBlock const & O) const
				{
					return !getAInterval().intersection(O.getAInterval()).isEmpty();
				}

				bool overlapsB(TraceBlock const & O) const
				{
					return !getBInterval().intersection(O.getBInterval()).isEmpty();
				}
			};
			
			std::ostream & operator<<(std::ostream & out, TraceBlock const & TB);
		}
	}
}
#endif
