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
#if !defined(LIBMAUS2_RANK_DNARANKMEMPOSCOMPARATOR_HPP)
#define LIBMAUS2_RANK_DNARANKMEMPOSCOMPARATOR_HPP

#include <libmaus2/rank/DNARankMEM.hpp>

namespace libmaus2
{
	namespace rank
	{
		struct DNARankMEMPosComparator
		{
			bool operator()(DNARankMEM const & A, DNARankMEM const & B) const
			{
				if ( A.left != B.left )
					return A.left < B.left;
				else if ( A.right != B.right )
					return A.right < B.right;
				else
					return A.intv < B.intv;
			}
		};
	}
}
#endif
