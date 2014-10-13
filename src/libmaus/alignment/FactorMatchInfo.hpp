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
#if ! defined(LIBMAUS_FM_FACTORMATCHINFO_HPP)
#define LIBMAUS_FM_FACTORMATCHINFO_HPP

#include <libmaus/types/types.hpp>
#include <ostream>

namespace libmaus
{
	namespace fm
	{
		struct FactorMatchInfo
		{
			uint64_t pos;
			uint64_t offset;
			uint64_t len;
			
			FactorMatchInfo() : pos(0), offset(0), len(0) {}
			FactorMatchInfo(
				uint64_t const rpos,
				uint64_t const roffset,
				uint64_t const rlen
			) : pos(rpos), offset(roffset), len(rlen)
			{
			
			}
			
			bool operator<(FactorMatchInfo const & O) const
			{
				return pos < O.pos;
			}
		};

		std::ostream & operator<<(std::ostream & out, FactorMatchInfo const & K);
	}
}
#endif
