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
#if ! defined(LIBMAUS2_LCS_NNPTRACEELEMENT_HPP)
#define LIBMAUS2_LCS_NNPTRACEELEMENT_HPP

#include <libmaus2/lcs/BaseConstants.hpp>
#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/util/shared_ptr.hpp>

namespace libmaus2
{
	namespace lcs
	{
		struct NNPTraceElement
		{
			typedef NNPTraceElement this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			libmaus2::lcs::BaseConstants::step_type step;
			int32_t slide;
			int32_t parent;

			NNPTraceElement(
				libmaus2::lcs::BaseConstants::step_type rstep = libmaus2::lcs::BaseConstants::STEP_RESET,
				int32_t rslide = 0,
				int32_t rparent = -1
			) : step(rstep), slide(rslide), parent(rparent)
			{
			}
		};

		std::ostream & operator<<(std::ostream & out, NNPTraceElement const & T);
	}
}
#endif
