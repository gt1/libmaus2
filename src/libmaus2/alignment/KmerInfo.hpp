/*
    libmaus
    Copyright (C) 2009-2014 German Tischler
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
*/
#if ! defined(LIBMAUS_FM_KMERINFO_HPP)
#define LIBMAUS_FM_KMERINFO_HPP

#include <libmaus/types/types.hpp>
#include <ostream>

namespace libmaus
{
	namespace alignment
	{
		struct KmerInfo
		{
			uint64_t rank;
			uint64_t pos;
			uint64_t offset;
			
			KmerInfo() : rank(0), pos(0), offset(0) {}
			KmerInfo(
				uint64_t const rrank,
				uint64_t const rpos,
				uint64_t const roffset
			) : rank(rrank), pos(rpos), offset(roffset) {}
			
			bool operator<(KmerInfo const & O) const
			{
				int64_t const offa = static_cast<int64_t>(pos) - static_cast<int64_t>(offset);
				int64_t const offb = static_cast<int64_t>(O.pos) - static_cast<int64_t>(O.offset);
				
				if ( offa != offb )
					return offa < offb;
				else
					return offset < O.offset;
			}
		};
		std::ostream & operator<<(std::ostream & out, KmerInfo const & K);
	}
}
#endif
