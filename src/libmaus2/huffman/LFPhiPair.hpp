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
#if ! defined(LIBMAUS2_HUFFMAN_LFPHIPAIR_HPP)
#define LIBMAUS2_HUFFMAN_LFPHIPAIR_HPP

#include <libmaus2/types/types.hpp>

namespace libmaus2
{
	namespace huffman
	{
		struct LFPhiPair
		{
			//uint64_t r0;
			uint64_t p0;
			uint64_t r1;
			uint64_t p1;

			LFPhiPair(
				//uint64_t rr0 = 0,
				uint64_t rp0 = 0,
				uint64_t rr1 = 0,
				uint64_t rp1 = 0
			)
			: /* r0(rr0), */ p0(rp0), r1(rr1), p1(rp1) {}

			bool operator==(LFPhiPair const & O) const
			{
				return
					// r0 == O.r0 &&
					p0 == O.p0 &&
					r1 == O.r1 &&
					p1 == O.p1;
			}
		};
	}
}
#endif
