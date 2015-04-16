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

#if ! defined(LIBMAUS_RANDOM_RANDOM_HPP)
#define LIBMAUS_RANDOM_RANDOM_HPP

#include <libmaus2/types/types.hpp>
#include <cstdlib>
#include <ctime>

namespace libmaus2
{
	namespace random
	{
		struct Random
		{
			static inline void setup(time_t const t = ::std::time(0))
			{
				srand(t);
			}
		
			static inline uint64_t rand64()
			{
				uint64_t v = 0;
				for ( unsigned int i = 0; i < sizeof(uint64_t); ++i )
				{
					v <<= 8;
					v |= ( rand() & 0xFF );
				}
				return v;
			}

			static inline uint32_t rand32()
			{
				uint64_t v = 0;
				for ( unsigned int i = 0; i < sizeof(uint32_t); ++i )
				{
					v <<= 8;
					v |= ( rand() & 0xFF );
				}
				return v;
			}

			static inline uint16_t rand16()
			{
				uint64_t v = 0;
				for ( unsigned int i = 0; i < sizeof(uint16_t); ++i )
				{
					v <<= 8;
					v |= ( rand() & 0xFF );
				}
				return v;
			}

			static inline uint8_t rand8()
			{
				uint64_t v = 0;
				for ( unsigned int i = 0; i < sizeof(uint8_t); ++i )
				{
					v <<= 8;
					v |= ( rand() & 0xFF );
				}
				return v;
			}
		};
	}
}
#endif
