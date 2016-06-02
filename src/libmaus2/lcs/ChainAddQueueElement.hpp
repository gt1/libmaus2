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
#if ! defined(LIBMAUS2_LCS_CHAINADDQUEUEELEMENT_HPP)
#define LIBMAUS2_LCS_CHAINADDQUEUEELEMENT_HPP

#include <libmaus2/types/types.hpp>
#include <ostream>

namespace libmaus2
{
	namespace lcs
	{

		struct ChainAddQueueElement
		{
			uint64_t xleft;
			uint64_t xright;
			uint64_t yleft;
			uint64_t chainid;
			uint64_t chainsubid;

			ChainAddQueueElement() {}
			ChainAddQueueElement(uint64_t const rxleft, uint64_t const rxright, uint64_t const ryleft, uint64_t const rchainid, uint64_t const rchainsubid)
			: xleft(rxleft), xright(rxright), yleft(ryleft), chainid(rchainid), chainsubid(rchainsubid) {}
		};

		std::ostream & operator<<(std::ostream & out, ChainAddQueueElement const & A);
	}
}
#endif
