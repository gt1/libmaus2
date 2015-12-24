/*
    libmaus2
    Copyright (C) 2009-2015 German Tischler
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

#if ! defined(LIBMAUS2_MATH_LOWBITS_HPP)
#define LIBMAUS2_MATH_LOWBITS_HPP

#include <libmaus2/types/types.hpp>
#include <climits>

namespace libmaus2
{
	namespace math
	{
		inline uint64_t lowbits(unsigned int const b)
		{
			if ( b < 64 )
				return (static_cast<uint64_t>(1ULL)<<b)-1;
			else
				return 0xFFFFFFFFFFFFFFFFULL;
		}

		template<typename data_type>
		struct LowBits
		{
			static data_type lowbits(unsigned int const b)
			{
				unsigned int const dbits = CHAR_BIT * sizeof(data_type);

				if ( b < dbits )
				{
					return (static_cast<data_type>(1) << b) - static_cast<data_type>(1);
				}
				else
				{
					unsigned int const dbits_high = dbits/2;
					unsigned int const dbits_low = dbits - dbits_high;

					data_type const high = LowBits<data_type>::lowbits(dbits_high);
					data_type const low  = LowBits<data_type>::lowbits(dbits_low);

					return
						(high << dbits_low) | low;
				}
			}
		};
	}
}
#endif
