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
#if ! defined(LIBMAUS2_LCS_SIMDX86GLOBALALIGNMENTSCORECONSTANTS256_HPP)
#define LIBMAUS2_LCS_SIMDX86GLOBALALIGNMENTSCORECONSTANTS256_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/LibMausConfig.hpp>

#if defined(LIBMAUS2_HAVE_GLOBAL_ALIGNMENT_Y256_8) || defined(LIBMAUS2_HAVE_GLOBAL_ALIGNMENT_Y256_16)
#include <immintrin.h>
#endif

namespace libmaus2
{
	namespace lcs
	{
		struct SimdX86GlobalAlignmentScoreConstants256
		{
			#if defined(LIBMAUS2_HAVE_GLOBAL_ALIGNMENT_Y256_8)
			static uint8_t const ymm_8_all_ff[sizeof(__m256i)]          __attribute__((aligned(sizeof(__m256i))));
			static uint8_t const ymm_8_first_ff_rest_0[sizeof(__m256i)] __attribute__((aligned(sizeof(__m256i))));
			static uint8_t const ymm_8_all_one[sizeof(__m256i)]         __attribute__((aligned(sizeof(__m256i))));
			static uint8_t const ymm_8_select_15[sizeof(__m256i)]       __attribute__((aligned(sizeof(__m256i))));
			static uint8_t const ymm_8_mask16[sizeof(__m256i)]     __attribute__((aligned(sizeof(__m256i))));
			static uint8_t const ymm_8_shiftover[sizeof(__m256i)]     __attribute__((aligned(sizeof(__m256i))));
			static uint8_t const ymm_8_banded_all_ff[sizeof(__m256i)/sizeof(uint8_t)]          __attribute__((aligned(sizeof(__m256i))));
			static uint8_t const ymm_8_banded_first_ff_rest_0[sizeof(__m256i)/sizeof(uint8_t)] __attribute__((aligned(sizeof(__m256i))));
			static uint8_t const ymm_8_banded_all_one[sizeof(__m256i)/sizeof(uint8_t)]         __attribute__((aligned(sizeof(__m256i))));
			static uint8_t const ymm_8_banded_select_15[sizeof(__m256i)/sizeof(uint8_t)]       __attribute__((aligned(sizeof(__m256i))));
			static uint8_t const ymm_8_banded_mask16[sizeof(__m256i)/sizeof(uint8_t)]     __attribute__((aligned(sizeof(__m256i))));
			static uint8_t const ymm_8_banded_shiftover[sizeof(__m256i)/sizeof(uint8_t)]     __attribute__((aligned(sizeof(__m256i))));
			static uint8_t const ymm_8_banded_last_ff_rest_0[sizeof(__m256i)/sizeof(uint8_t)]       __attribute__((aligned(sizeof(__m128i))));
			static uint8_t const ymm_8_banded_select_16[sizeof(__m256i)/sizeof(uint8_t)]       __attribute__((aligned(sizeof(__m256i))));
			static uint8_t const ymm_8_banded_shiftoverleft[sizeof(__m256i)/sizeof(uint8_t)]     __attribute__((aligned(sizeof(__m256i))));
			static uint8_t const ymm_8_banded_mask15[sizeof(__m256i)/sizeof(uint8_t)]     __attribute__((aligned(sizeof(__m256i))));
			static uint8_t const ymm_8_banded_16_to_end[sizeof(__m256i)/sizeof(uint8_t)]     __attribute__((aligned(sizeof(__m256i))));
			static uint8_t const ymm_8_banded_select_first_to_back[sizeof(__m256i)/sizeof(uint8_t)]      __attribute__((aligned(sizeof(__m256i))));
			static uint8_t const ymm_8_banded_all_0[sizeof(__m256i)/sizeof(uint8_t)]          __attribute__((aligned(sizeof(__m256i))));
			static uint8_t const ymm_8_banded_all_1[sizeof(__m256i)/sizeof(uint8_t)]          __attribute__((aligned(sizeof(__m256i))));
			static uint8_t const ymm_8_banded_all_signed_max[sizeof(__m256i)/sizeof(uint8_t)]          __attribute__((aligned(sizeof(__m256i))));
			#endif
			#if defined(LIBMAUS2_HAVE_GLOBAL_ALIGNMENT_Y256_16)
			static uint16_t const ymm_16_all_ff[sizeof(__m256i)/sizeof(uint16_t)]          __attribute__((aligned(sizeof(__m256i))));
			static uint16_t const ymm_16_first_ff_rest_0[sizeof(__m256i)/sizeof(uint16_t)] __attribute__((aligned(sizeof(__m256i))));
			static uint16_t const ymm_16_all_one[sizeof(__m256i)/sizeof(uint16_t)]         __attribute__((aligned(sizeof(__m256i))));
			static uint16_t const ymm_16_select_15[sizeof(__m256i)/sizeof(uint16_t)]       __attribute__((aligned(sizeof(__m256i))));
			static uint16_t const ymm_16_mask16[sizeof(__m256i)/sizeof(uint16_t)]     __attribute__((aligned(sizeof(__m256i))));
			static uint16_t const ymm_16_shiftover[sizeof(__m256i)/sizeof(uint16_t)]     __attribute__((aligned(sizeof(__m256i))));
			static uint16_t const ymm_16_banded_all_ff[sizeof(__m256i)/sizeof(uint16_t)]          __attribute__((aligned(sizeof(__m256i))));
			static uint16_t const ymm_16_banded_first_ff_rest_0[sizeof(__m256i)/sizeof(uint16_t)] __attribute__((aligned(sizeof(__m256i))));
			static uint16_t const ymm_16_banded_all_one[sizeof(__m256i)/sizeof(uint16_t)]         __attribute__((aligned(sizeof(__m256i))));
			static uint16_t const ymm_16_banded_select_15[sizeof(__m256i)/sizeof(uint16_t)]       __attribute__((aligned(sizeof(__m256i))));
			static uint16_t const ymm_16_banded_mask16[sizeof(__m256i)/sizeof(uint16_t)]     __attribute__((aligned(sizeof(__m256i))));
			static uint16_t const ymm_16_banded_shiftover[sizeof(__m256i)/sizeof(uint16_t)]     __attribute__((aligned(sizeof(__m256i))));
			static uint16_t const ymm_16_banded_last_ff_rest_0[sizeof(__m256i)/sizeof(uint16_t)]       __attribute__((aligned(sizeof(__m128i))));
			static uint16_t const ymm_16_banded_select_16[sizeof(__m256i)/sizeof(uint16_t)]       __attribute__((aligned(sizeof(__m256i))));
			static uint16_t const ymm_16_banded_shiftoverleft[sizeof(__m256i)/sizeof(uint16_t)]     __attribute__((aligned(sizeof(__m256i))));
			static uint16_t const ymm_16_banded_mask15[sizeof(__m256i)/sizeof(uint16_t)]     __attribute__((aligned(sizeof(__m256i))));
			static uint16_t const ymm_16_banded_16_to_end[sizeof(__m256i)/sizeof(uint16_t)]     __attribute__((aligned(sizeof(__m256i))));
			static uint16_t const ymm_16_banded_select_first_to_back[sizeof(__m256i)/sizeof(uint16_t)]      __attribute__((aligned(sizeof(__m256i))));
			static uint16_t const ymm_16_banded_all_0[sizeof(__m256i)/sizeof(uint16_t)]          __attribute__((aligned(sizeof(__m256i))));
			static uint16_t const ymm_16_banded_all_1[sizeof(__m256i)/sizeof(uint16_t)]          __attribute__((aligned(sizeof(__m256i))));
			static uint16_t const ymm_16_banded_all_signed_max[sizeof(__m256i)/sizeof(uint16_t)]          __attribute__((aligned(sizeof(__m256i))));
			#endif
		};
	}
}
#endif
