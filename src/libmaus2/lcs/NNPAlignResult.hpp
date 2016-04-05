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
#if ! defined(LIBMAUS2_LCS_NNPALIGNRESULT_HPP)
#define LIBMAUS2_LCS_NNPALIGNRESULT_HPP

#include <libmaus2/types/types.hpp>
#include <ostream>

namespace libmaus2
{
	namespace lcs
	{
		struct NNPAlignResult
		{
			uint64_t abpos;
			uint64_t aepos;
			uint64_t bbpos;
			uint64_t bepos;
			uint64_t dif;

			NNPAlignResult(
				uint64_t rabpos = 0,
				uint64_t raepos = 0,
				uint64_t rbbpos = 0,
				uint64_t rbepos = 0,
				uint64_t rdif = 0
			) : abpos(rabpos), aepos(raepos), bbpos(rbbpos), bepos(rbepos), dif(rdif)
			{

			}

			double getErrorRate() const
			{
				return (static_cast<double>(dif) / static_cast<double>(aepos-abpos));
			}
		};

		std::ostream & operator<<(std::ostream & out, NNPAlignResult const & O);
	}
}
#endif
