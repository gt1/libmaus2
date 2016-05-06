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
#if ! defined(LIBMAUS2_GAMMA_FLAGGEDPARTITIONINTERVAL_HPP)
#define LIBMAUS2_GAMMA_FLAGGEDPARTITIONINTERVAL_HPP

#include <libmaus2/types/types.hpp>

namespace libmaus2
{
	namespace gamma
	{
		struct FlaggedPartitionInterval
		{
			uint64_t from;
			uint64_t to;
			bool complete;

			FlaggedPartitionInterval()
			{

			}

			FlaggedPartitionInterval(uint64_t const rfrom, uint64_t const rto, bool const rcomplete)
			: from(rfrom), to(rto), complete(rcomplete)
			{

			}

			bool operator==(FlaggedPartitionInterval const & P) const
			{
				return
					from == P.from && to == P.to && complete == P.complete;
			}
		};
	}
}
#endif
