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
#if ! defined(LIBMAUS2_RANK_DNARANKMEM_HPP)
#define LIBMAUS2_RANK_DNARANKMEM_HPP

#include <libmaus2/rank/DNARankBiDirRange.hpp>

namespace libmaus2
{
	namespace rank
	{
		struct DNARankMEM
		{
			libmaus2::rank::DNARankBiDirRange intv;
			uint64_t left;
			uint64_t right;

			DNARankMEM()
			{

			}

			uint64_t diam() const
			{
				return right-left;
			}

			DNARankMEM(libmaus2::rank::DNARankBiDirRange const & rintv, uint64_t const rleft, uint64_t const rright)
			: intv(rintv), left(rleft), right(rright) {}

			bool operator<(DNARankMEM const & O) const
			{
				if ( intv < O.intv )
					return true;
				else if ( O.intv < intv )
					return false;
				else if ( left != O.left )
					return left < O.left;
				else
					return right < O.right;
			}

			bool operator==(DNARankMEM const & O) const
			{
				if ( *this < O )
					return false;
				else if ( O < *this )
					return false;
				else
					return true;
			}

			bool operator!=(DNARankMEM const & O) const
			{
				return !operator==(O);
			}
		};
		
		std::ostream & operator<<(std::ostream & out, libmaus2::rank::DNARankMEM const & D);
	}
}
#endif
