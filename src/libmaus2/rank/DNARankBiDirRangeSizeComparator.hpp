/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#if ! defined(LIBMAUS2_RANK_DNARANKBIDIRRANGESIZECOMPARATOR_HPP)
#define LIBMAUS2_RANK_DNARANKBIDIRRANGESIZECOMPARATOR_HPP

#include <libmaus2/rank/DNARankBiDirRange.hpp>

namespace libmaus2
{
	namespace rank
	{
		struct DNARankBiDirRangeSizeComparator
		{
			bool operator()(
				libmaus2::rank::DNARankBiDirRange const & A,
				libmaus2::rank::DNARankBiDirRange const & B
			) const
			{
				return A.size > B.size;
			}
		};
	}
}
#endif
