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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_OVERLAPINFO_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_OVERLAPINFO_HPP

#include <libmaus2/types/types.hpp>
#include <ostream>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct OverlapInfo
			{
				int64_t aread;
				int64_t bread;
				int64_t abpos;
				int64_t aepos;
				int64_t bbpos;
				int64_t bepos;

				OverlapInfo()
				{
				}
				OverlapInfo(
					int64_t const raread,
					int64_t const rbread,
					int64_t const rabpos,
					int64_t const raepos,
					int64_t const rbbpos,
					int64_t const rbepos
				) : aread(raread), bread(rbread), abpos(rabpos), aepos(raepos), bbpos(rbbpos), bepos(rbepos) {}

				OverlapInfo inverse(uint64_t const alen, uint64_t const blen) const
				{
					return OverlapInfo(
						aread ^ 1,
						bread ^ 1,
						alen - aepos,
						alen - abpos,
						blen - bepos,
						blen - bbpos
					);
				}

				template<typename iterator>
				OverlapInfo inverse(iterator A) const
				{
					return inverse(A[aread/2],A[bread/2]);
				}
			};

			std::ostream & operator<<(std::ostream & out, OverlapInfo const & O);
		}
	}
}
#endif
