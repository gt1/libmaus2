/*
    libmaus2
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_REFIDINTERVAL_HPP)
#define       LIBMAUS_BAMBAM_PARALLEL_REFIDINTERVAL_HPP

#include <libmaus2/types/types.hpp>
#include <ostream>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			struct RefIdInterval
			{
				// ref id
				int32_t refid;
				// low index
				uint64_t i_low;
				// high index
				uint64_t i_high;
				// low byte mark in buffer
				uint64_t b_low;
				// high byte mark in buffer
				uint64_t b_high;
				
				RefIdInterval() : refid(-1), i_low(0), i_high(0), b_low(0), b_high(0)
				{
				
				}
				RefIdInterval(
					int32_t const rrefid,
					uint64_t const ri_low,
					uint64_t const rb_low
				) : refid(rrefid), i_low(ri_low), i_high(ri_low), b_low(rb_low), b_high(rb_low) {}
			};
			
			std::ostream & operator<<(std::ostream & out, RefIdInterval const & R);
		}
	}
}
#endif
