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
#if ! defined(LIBMAUS2_LCS_CHAINNODEINFO_HPP)
#define LIBMAUS2_LCS_CHAINNODEINFO_HPP

#include <libmaus2/types/types.hpp>
#include <ostream>

namespace libmaus2
{
	namespace lcs
	{

		struct ChainNodeInfo
		{
			uint64_t parentsubid;
			uint64_t xleft;
			uint64_t xright;
			uint64_t yleft;
			uint64_t yright;
			uint64_t chainscore;

			ChainNodeInfo() {}
			ChainNodeInfo(
				uint64_t rparentsubid,
				uint64_t rxleft,
				uint64_t rxright,
				uint64_t ryleft,
				uint64_t ryright
			) : parentsubid(rparentsubid), xleft(rxleft), xright(rxright), yleft(ryleft), yright(ryright), chainscore(0) {}

			int64_t diag() const
			{
				return
					static_cast<int64_t>(xleft) - static_cast<int64_t>(yleft);
			}

			uint64_t antiLow() const
			{
				return xleft + yleft;
			}

			uint64_t antiHigh() const
			{
				return xright + yright;
			}

			bool operator==(ChainNodeInfo const & O) const
			{
				return
					parentsubid == O.parentsubid &&
					xleft == O.xleft &&
					xright == O.xright &&
					yleft == O.yleft &&
					yright == O.yright &&
					chainscore == O.chainscore;
			}
		};

		std::ostream & operator<<(std::ostream & out, ChainNodeInfo const & C);
	}
}
#endif
