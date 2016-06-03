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
#if ! defined(LIBMAUS2_LCS_CHAINLEFTMOSTCOMPARATOR_HPP)
#define LIBMAUS2_LCS_CHAINLEFTMOSTCOMPARATOR_HPP

#include <libmaus2/lcs/Chain.hpp>
#include <libmaus2/lcs/ChainNode.hpp>
#include <libmaus2/util/ContainerElementFreeList.hpp>

namespace libmaus2
{
	namespace lcs
	{
		struct ChainLeftMostComparator
		{
			libmaus2::util::ContainerElementFreeList<libmaus2::lcs::ChainNode> * chainnodefreelist;

			ChainLeftMostComparator()
			: chainnodefreelist(0)
			{

			}

			ChainLeftMostComparator(
				libmaus2::util::ContainerElementFreeList<libmaus2::lcs::ChainNode> * rchainnodefreelist
			) : chainnodefreelist(rchainnodefreelist)
			{

			}

			bool operator()(Chain & A, Chain & B) const
			{
				return
					A.getLeftMost(*chainnodefreelist)
					<
					B.getLeftMost(*chainnodefreelist);
			}
		};
	}
}
#endif
