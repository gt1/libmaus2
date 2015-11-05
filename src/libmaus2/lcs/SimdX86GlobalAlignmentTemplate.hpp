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

namespace libmaus2
{
	namespace lcs
	{
		struct LIBMAUS2_SIMD_CLASS_NAME : public libmaus2::lcs::AlignmentTraceContainer, public libmaus2::lcs::Aligner
		{
			private:
			LIBMAUS2_SIMD_ELEMENT_TYPE * diagmem;
			size_t diagmemsize;
			LIBMAUS2_SIMD_ELEMENT_TYPE * aspace;
			size_t aspacesize;
			LIBMAUS2_SIMD_ELEMENT_TYPE * bspace;
			size_t bspacesize;

			std::pair<int64_t,int64_t> diagToSquare(std::pair<int64_t,int64_t> const P)
			{
				return std::pair<int64_t,int64_t>(P.first-P.second,P.second);
			}

			std::pair<int64_t,int64_t> squareToDiag(std::pair<int64_t,int64_t> const P)
			{
				return std::pair<int64_t,int64_t>(P.first+P.second,P.second);
			}

			void allocateMemory(
				size_t const rsize,
				size_t const sizealign,
				LIBMAUS2_SIMD_ELEMENT_TYPE * & mem,
				size_t & memsize
			);

			public:
			LIBMAUS2_SIMD_CLASS_NAME();
			~LIBMAUS2_SIMD_CLASS_NAME();

			void align(uint8_t const * a, size_t const l_a, uint8_t const * b, size_t const l_b);
			AlignmentTraceContainer const & getTraceContainer() const { return *this; }
		};
	}
}
