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
#if ! defined(LIBMAUS2_LCS_CHAINNODE_HPP)
#define LIBMAUS2_LCS_CHAINNODE_HPP

#include <libmaus2/types/types.hpp>
#include <ostream>
#include <limits>

namespace libmaus2
{
	namespace lcs
	{
		struct ChainNode
		{
			uint64_t chainid;
			uint64_t chainsubid;
			uint64_t parentsubid;
			uint64_t xleft;
			uint64_t xright;
			uint64_t yleft;
			uint64_t yright;
			uint64_t chainscore;
			uint64_t next;

			ChainNode() : chainscore(0), next(std::numeric_limits<uint64_t>::max()) {}
			ChainNode(
				uint64_t rchainid,
				uint64_t rchainsubid,
				uint64_t rparentsubid,
				uint64_t rxleft,
				uint64_t rxright,
				uint64_t ryleft,
				uint64_t ryright
			) : chainid(rchainid), chainsubid(rchainsubid), parentsubid(rparentsubid), xleft(rxleft), xright(rxright), yleft(ryleft), yright(ryright), chainscore(0), next(std::numeric_limits<uint64_t>::max()) {}

			bool operator<(ChainNode const & C) const
			{
				if ( chainid != C.chainid )
					return chainid < C.chainid;
				else
					return chainsubid < C.chainsubid;
			}
		};

		std::ostream & operator<<(std::ostream & out, ChainNode const & C);
	}
}
#endif
