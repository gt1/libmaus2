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
#if !defined(LIBMAUS2_RANK_DNARANKGETPOSITION_HPP)
#define LIBMAUS2_RANK_DNARANKGETPOSITION_HPP

#include <libmaus2/rank/DNARank.hpp>
#include <libmaus2/suffixsort/bwtb3m/BwtMergeSortResult.hpp>

namespace libmaus2
{
	namespace rank
	{
		template<typename _ssa_type>
		struct DNARankGetPosition
		{
			typedef _ssa_type ssa_type;

			libmaus2::rank::DNARank const & Prank;
			ssa_type const & BSSSA;

			DNARankGetPosition(
				libmaus2::rank::DNARank const & rPrank,
				ssa_type const & rBSSSA
			) : Prank(rPrank), BSSSA(rBSSSA)
			{

			}

			uint64_t getPosition(uint64_t const r) const
			{
				std::pair<uint64_t,uint64_t> const P = Prank.simpleLFUntilMask(r, BSSSA.samplingrate-1);
				uint64_t const p = ( BSSSA[P.first / BSSSA.samplingrate] + P.second ) % Prank.size();
				return p;
			}
		};
	}
}
#endif
