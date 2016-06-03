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
#if ! defined(LIBMAUS2_LCS_CHAINSPLAYTREENODECOMPARATOR_HPP)
#define LIBMAUS2_LCS_CHAINSPLAYTREENODECOMPARATOR_HPP

#include <libmaus2/lcs/ChainSplayTreeNode.hpp>

namespace libmaus2
{
	namespace lcs
	{
		struct ChainSplayTreeNodeComparator
		{
			bool operator()(libmaus2::lcs::ChainSplayTreeNode const & A, libmaus2::lcs::ChainSplayTreeNode const & B) const
			{
				if ( A.yright != B.yright )
					return A.yright < B.yright;
				else if ( A.xright != B.xright )
					return A.xright < B.xright;
				else if ( A.chainid != B.chainid )
					return A.chainid < B.chainid;
				else
					return A.chainsubid < B.chainsubid;
			}
		};
	}
}
#endif
