/*
    libmaus
    Copyright (C) 2009-2013 German Tischler
    Copyright (C) 2011-2013 Genome Research Limited

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

#if ! defined(LOG2TABLE_HPP)
#define LOG2TABLE_HPP

#include <libmaus/types/types.hpp>
#include <algorithm>

namespace libmaus
{
	namespace rank
	{
		/* log2_65536_16_table [i] >= ceil ( 2^16 * log( 2^16 / i ) ) */
		// extern unsigned int const log2_65536_16_table[];
		extern unsigned int const logtable2_16_16[];

		inline uint64_t log_estimate (uint64_t const num, uint64_t const total)
		{
			/* fraction, rounding down */
			uint64_t const frac = (num << 16)/total;
			/* return log */
			return logtable2_16_16[frac];
		}

		inline uint64_t log_num_estimate(uint64_t const num, uint64_t const total)
		{
			return num * log_estimate(num,total);
		}

		inline uint64_t entropy_estimate_raw(uint64_t const num, uint64_t const total)
		{
			uint64_t const rawent = log_num_estimate(num,total) + log_num_estimate(total-num,total);
			/* return */
			return rawent;
		}

		inline uint64_t entropy_estimate_up(uint64_t const num, uint64_t const total)
		{
			if ( ! total )
				return 0;

			uint64_t const rawent = entropy_estimate_raw(num,total);
			/* divide, rounding up */	
			uint64_t const entr = (rawent + ((1u<<16)-1)) >> 16;
			/* return */
			return entr;	
		}

		inline uint64_t entropy_estimate_down(uint64_t const num, uint64_t const total)
		{
			if ( ! total )
				return 0;

			uint64_t const rawent = entropy_estimate_raw(num,total);
			/* divide, rounding up */	
			uint64_t const entr = rawent >> 16;
			/* return */
			return entr;	
		}
	}
}
#endif
