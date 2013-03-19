/**
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
**/

#if ! defined(POSIXMUTEXARRAY_HPP)
#define POSIXMUTEXARRAY_HPP

#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/parallel/PosixMutex.hpp>

namespace libmaus
{
	namespace parallel
	{
		struct PosixMutexArray
		{
			unsigned int const blockbits;
			unsigned int const hashbits;
			::libmaus::autoarray::AutoArray < ::libmaus::parallel::PosixMutex > mutexes;

			PosixMutexArray(unsigned int const rblockbits, unsigned int rhashbits)
			: blockbits(rblockbits), hashbits(rhashbits), mutexes(1ull << blockbits)
			{

			}
			
			PosixMutex & get(uint64_t v)
			{
				return mutexes [ v >> (hashbits-blockbits) ];
			}
		};
	}
}
#endif
