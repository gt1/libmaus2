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
#if ! defined(LIBMAUS2_BAMBAM_PILEVECTORELEMENT_HPP)
#define LIBMAUS2_BAMBAM_PILEVECTORELEMENT_HPP

#include <libmaus2/types/types.hpp>

namespace libmaus2
{
	namespace bambam
	{
		struct PileVectorElement
		{
			int32_t readid;
			int32_t refpos;
			int32_t predif;
			int32_t readpos;
			char sym;

			PileVectorElement(
				int32_t rreadid = 0,
				int32_t rrefpos = 0,
				int32_t rpredif = 0,
				int32_t rreadpos = 0,
				char rsym = 0
			)
			: readid(rreadid), refpos(rrefpos), predif(rpredif), readpos(rreadpos), sym(rsym)
			{

			}

			bool operator<(PileVectorElement const & o) const
			{
				if ( refpos != o.refpos )
					return refpos < o.refpos;
				else if ( predif != o.predif )
					return predif < o.predif;
				else if ( sym != o.sym )
					return sym < o.sym;
				else if ( readid != o.readid )
					return readid < o.readid;
				else
					return readpos < o.readpos;
			}
		};
	}
}
#endif
