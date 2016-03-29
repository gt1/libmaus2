/**
    libmaus2

    Copyright (C) 2009-2016 German Tischler
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
**/
#if ! defined(LIBMAUS2_SUFFIXSORT_BWTB3M_MERGESTRATEGYMERGEGAPREQUESTQUERYOBJECT_HPP)
#define LIBMAUS2_SUFFIXSORT_BWTB3M_MERGESTRATEGYMERGEGAPREQUESTQUERYOBJECT_HPP

#include <libmaus2/types/types.hpp>

namespace libmaus2
{
	namespace suffixsort
	{
		namespace bwtb3m
		{
			// forward declaration
			struct MergeStrategyMergeGapRequest;

			struct MergeStrategyMergeGapRequestQueryObject
			{
				uint64_t p;
				uint64_t r;
				MergeStrategyMergeGapRequest * o;

				MergeStrategyMergeGapRequestQueryObject()
				: p(0), r(0), o(0)
				{

				}

				MergeStrategyMergeGapRequestQueryObject(uint64_t const rp)
				: p(rp), r(0), o(0)
				{

				}

				MergeStrategyMergeGapRequestQueryObject(uint64_t const rp, uint64_t const rr, MergeStrategyMergeGapRequest * ro)
				: p(rp), r(rr), o(ro)
				{

				}

				bool operator<(MergeStrategyMergeGapRequestQueryObject const & o) const
				{
					return p < o.p;
				}
			};
		}
	}
}
#endif
