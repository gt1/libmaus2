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
#include <unistd.h> // for getpagesize

#include <cassert>
#include <cstdlib>

#include <algorithm> // reverse
#include <iostream>
#include <iomanip>
#include <limits>
#include <map>
#include <sstream>
#include <utility>
#include <vector>

#include <immintrin.h> // all of the above

#include <libmaus2/lcs/AlignmentTraceContainer.hpp>
#include <libmaus2/lcs/Aligner.hpp>
#include <libmaus2/types/types.hpp>
#include <libmaus2/lcs/ScoreBandedAligner.hpp>

namespace libmaus2
{
	namespace lcs
	{
		struct LIBMAUS2_LCS_SIMD_BANDED_CLASS_NAME : public libmaus2::lcs::AlignmentTraceContainer, public libmaus2::lcs::ScoreBandedAligner
		{
			private:
			LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE * diagmem;
			size_t diagmemsize;
			LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE * negdiagmem;
			size_t negdiagmemsize;

			LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE * difdiagmem;
			size_t difdiagmemsize;
			
			LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE * textmem;
			size_t textmemsize;
			LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE * querymem;
			size_t querymemsize;

			static void allocateMemory(size_t const rsize, size_t const sizealign, LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE * & mem, size_t & memsize);

			public:
			LIBMAUS2_LCS_SIMD_BANDED_CLASS_NAME();
			~LIBMAUS2_LCS_SIMD_BANDED_CLASS_NAME();

			std::pair<uint64_t,uint64_t> align(
				uint8_t const * a,
				size_t const l_a,
				uint8_t const * b,
				size_t const l_b,
				size_t const d
			);

			static std::pair<int64_t,int64_t> squareToDiag(std::pair<int64_t,int64_t> const P, size_t const d)
			{
				int64_t const di = P.first + P.second;
				return std::pair<int64_t,int64_t>(di,P.second - di / 2 + d);
			}

			static std::pair<int64_t,int64_t> diagToSquare(std::pair<int64_t,int64_t> const D, size_t const d)
			{
				std::pair<int64_t,int64_t> P;

				int64_t const di = D.first;
				
				P.second = D.second + (di / 2) - d;
				P.first = D.first - P.second;

				return P;
			}

			AlignmentTraceContainer const & getTraceContainer() const;
		};
	}
}
