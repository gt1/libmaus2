/*
    libmaus2
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

#if ! defined(LIBMAUS2_UTIL_OFFSETARRAY_HPP)
#define LIBMAUS2_UTIL_OFFSETARRAY_HPP

#include <libmaus2/types/types.hpp>

namespace libmaus2
{
	namespace util
	{
		template < typename array_a_type, typename array_b_type >
		struct OffsetArray
		{
			array_a_type const * SA;
			array_b_type const * data;
			uint64_t n;
			uint64_t offset;

			OffsetArray(array_a_type const * rSA, array_b_type const * rdata, uint64_t rn, uint64_t roffset)
			: SA(rSA), data(rdata), n(rn), offset(roffset) {}

			int64_t operator[](uint64_t i) const
			{
				uint64_t const p = SA[i]+offset;

				if ( p >= n )
					return -1;
				else
					return data[p];
			}
		};
	}
}
#endif
