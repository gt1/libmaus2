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

#if ! defined(LIBMAUS_UTIL_BITLIST_HPP)
#define LIBMAUS_UTIL_BITLIST_HPP

#include <cassert>
#include <list>
#include <ostream>
#include <sys/types.h>

#include <libmaus/types/types.hpp>

namespace libmaus
{
	namespace util
	{
		struct BitList
		{
			std::list< bool > B;
			
			BitList(uint64_t words);
			uint64_t select1(uint64_t rank) const;
			uint64_t select0(uint64_t rank) const;
			uint64_t rank1(uint64_t pos) const;
			void insertBit(uint64_t pos, bool b);
			void deleteBit(uint64_t pos);
			void setBit(uint64_t pos, bool b);
		};

		std::ostream & operator<<(std::ostream & out, BitList const & B);
	}
}
#endif
