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
#if ! defined(LIBMAUS2_HUFFMAN_LFRANKLCP_HPP)
#define LIBMAUS2_HUFFMAN_LFRANKLCP_HPP

#include <libmaus2/types/types.hpp>

namespace libmaus2
{
	namespace huffman
	{
		struct LFRankLCP
		{
			uint64_t r;
			uint64_t lcp;

			LFRankLCP(
				uint64_t rr = 0,
				uint64_t rlcp = 0
			)
			: r(rr), lcp(rlcp) {}

			bool operator==(LFRankLCP const & O) const
			{
				return r == O.r && lcp == O.lcp;
			}
		};
	}
}
#endif
