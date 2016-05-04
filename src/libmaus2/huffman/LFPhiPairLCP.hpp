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
#if ! defined(LIBMAUS2_HUFFMAN_LFPHIPAIRLCP_HPP)
#define LIBMAUS2_HUFFMAN_LFPHIPAIRLCP_HPP

#include <libmaus2/types/types.hpp>

namespace libmaus2
{
	namespace huffman
	{
		struct LFPhiPairLCP
		{
			uint64_t r1;
			uint64_t p1;
			uint64_t lcp;

			LFPhiPairLCP(
				uint64_t rr1 = 0,
				uint64_t rp1 = 0,
				uint64_t rlcp = 0
			)
			: r1(rr1), p1(rp1), lcp(rlcp) {}

			bool operator==(LFPhiPairLCP const & O) const
			{
				return
					r1 == O.r1 &&
					p1 == O.p1 &&
					lcp == O.lcp;
			}
		};
	}
}
#endif
