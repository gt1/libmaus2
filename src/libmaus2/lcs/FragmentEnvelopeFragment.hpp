/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#if ! defined(LIBMAUS2_LCS_FRAGMENTENVELOPEFRAGMENT_HPP)
#define LIBMAUS2_LCS_FRAGMENTENVELOPEFRAGMENT_HPP

#include <libmaus2/lcs/FragmentEnvelopeYScore.hpp>

namespace libmaus2
{
	namespace lcs
	{
		struct FragmentEnvelopeFragment
		{
			int64_t x;
			libmaus2::lcs::FragmentEnvelopeYScore yscore;

			FragmentEnvelopeFragment(int64_t const rx = -1, FragmentEnvelopeYScore const ryscore = FragmentEnvelopeYScore())
			: x(rx), yscore(ryscore)
			{

			}
		};

		std::ostream & operator<<(std::ostream & out, FragmentEnvelopeFragment const & O);
	}
}
#endif
