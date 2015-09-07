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
#define LIBMAUS2_LCS_SIMD_BANDED_CLASS_NAME SimdX86BandedGlobalAlignmentY256_16
#define LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE __m256i
#define LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE uint16_t
#define LIBMAUS2_LCS_SIMD_BANDED_FIRST_FF_REST_0 libmaus2::lcs::SimdX86GlobalAlignmentConstants256::ymm_16_banded_first_ff_rest_0
#define LIBMAUS2_LCS_SIMD_BANDED_LAST_FF_REST_0 libmaus2::lcs::SimdX86GlobalAlignmentConstants256::ymm_16_banded_last_ff_rest_0
#define LIBMAUS2_LCS_SIMD_BANDED_ALL_ONE libmaus2::lcs::SimdX86GlobalAlignmentConstants256::ymm_16_banded_all_one
#define LIBMAUS2_LCS_SIMD_BANDED_SELECT_FIRST_TOBACK libmaus2::lcs::SimdX86GlobalAlignmentConstants256::ymm_16_banded_select_first_to_back

#define LIBMAUS2_LCS_SIMD_BANDED_STORE _mm256_store_si256
#define LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED _mm256_load_si256
#define LIBMAUS2_LCS_SIMD_BANDED_LOAD_UNALIGNED _mm256_loadu_si256
#define LIBMAUS2_LCS_SIMD_BANDED_ANDNOT _mm256_andnot_si256
#define LIBMAUS2_LCS_SIMD_BANDED_OR _mm256_or_si256

#define LIBMAUS2_LCS_SIMD_BANDED_CMPEQ _mm256_cmpeq_epi16
#define LIBMAUS2_LCS_SIMD_BANDED_ADD _mm256_adds_epu16
#define LIBMAUS2_LCS_SIMD_BANDED_SHUFFLE _mm256_shuffle_epi8
#define LIBMAUS2_LCS_SIMD_BANDED_MIN _mm256_min_epu16

#define LIBMAUS2_LCS_SIMD_BANDED_SHIFTOVER libmaus2::lcs::SimdX86GlobalAlignmentConstants256::ymm_16_banded_shiftover
#define LIBMAUS2_LCS_SIMD_BANDED_SHIFTOVERLEFT libmaus2::lcs::SimdX86GlobalAlignmentConstants256::ymm_16_banded_shiftoverleft

#if 0
	LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const x_select_first_toback = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const *>(&LIBMAUS2_LCS_SIMD_BANDED_SELECT_FIRST_TOBACK)); \
	LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const x_select16 = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const *>(&libmaus2::lcs::SimdX86GlobalAlignmentConstants256::ymm_16_banded_select_16));
#endif

#define LIBMAUS2_LCS_SIMD_BANDED_INIT \
	LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const x_shiftover = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const *>(&LIBMAUS2_LCS_SIMD_BANDED_SHIFTOVER)); \
	LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const x_mask16 = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const *>(&libmaus2::lcs::SimdX86GlobalAlignmentConstants256::ymm_16_banded_mask16[0])); \
	LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const x_select15 = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const *>(&libmaus2::lcs::SimdX86GlobalAlignmentConstants256::ymm_16_banded_select_15)); \
	LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const x_shiftoverleft = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const *>(&LIBMAUS2_LCS_SIMD_BANDED_SHIFTOVERLEFT)); \
	LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const x_mask15 = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const *>(&libmaus2::lcs::SimdX86GlobalAlignmentConstants256::ymm_16_banded_mask15[0])); \
	LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const x_16_to_end = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const *>(&libmaus2::lcs::SimdX86GlobalAlignmentConstants256::ymm_16_banded_16_to_end[0])); 

#define LIBMAUS2_LCS_SIMD_BANDED_SHIFTRIGHT(val) \
	_mm256_or_si256( \
		_mm256_shuffle_epi8(val,x_shiftover), \
		_mm256_shuffle_epi8(_mm256_permute2f128_si256(val,val,8),x_mask16) \
	)
#define LIBMAUS2_LCS_SIMD_BANDED_SELECTLAST(v) LIBMAUS2_LCS_SIMD_BANDED_SHUFFLE(_mm256_permute2f128_si256(v,v,1),x_select15)

#define LIBMAUS2_LCS_SIMD_BANDED_SHIFTLEFT(val) \
	_mm256_or_si256( \
		_mm256_shuffle_epi8(val,x_shiftoverleft), \
		_mm256_shuffle_epi8(_mm256_permute2f128_si256(val,val,241),x_mask15) \
	)

// move first element to last and erase rest, broken
#define LIBMAUS2_LCS_SIMD_BANDED_FIRST_TO_BACK(val) LIBMAUS2_LCS_SIMD_BANDED_SHUFFLE(_mm256_permute2f128_si256(val,val,8),x_16_to_end)
