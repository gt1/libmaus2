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
#if ! defined(LIBMAUS2_LCS_BANDEDALIGNER_HPP)
#define LIBMAUS2_LCS_BANDEDALIGNER_HPP

#include <libmaus2/lcs/AlignmentTraceContainer.hpp>

namespace libmaus2
{
	namespace lcs
	{
		struct BandedAligner
		{
			typedef BandedAligner this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			virtual ~BandedAligner() {}
			virtual void align(
				uint8_t const * a,
				size_t const l_a,
				uint8_t const * b,
				size_t const l_b,
				size_t const k
			) = 0;
			virtual AlignmentTraceContainer const & getTraceContainer() const = 0;
		};
	}
}
#endif
