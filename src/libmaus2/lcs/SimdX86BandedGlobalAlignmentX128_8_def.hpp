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
#define LIBMAUS2_LCS_SIMD_BANDED_CLASS_NAME SimdX86BandedGlobalAlignmentX128_8
#define LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE __m128i
#define LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE uint8_t
#define LIBMAUS2_LCS_SIMD_BANDED_FIRST_FF_REST_0 libmaus2::lcs::SimdX86GlobalAlignmentConstants128::xmm_8_banded_first_ff_rest_0
#define LIBMAUS2_LCS_SIMD_BANDED_LAST_FF_REST_0 libmaus2::lcs::SimdX86GlobalAlignmentConstants128::xmm_8_banded_last_ff_rest_0
#define LIBMAUS2_LCS_SIMD_BANDED_ALL_ONE libmaus2::lcs::SimdX86GlobalAlignmentConstants128::xmm_8_banded_all_one
#define LIBMAUS2_LCS_SIMD_BANDED_SELECT_FIRST_TOBACK libmaus2::lcs::SimdX86GlobalAlignmentConstants128::xmm_8_banded_select_first_to_back

#define LIBMAUS2_LCS_SIMD_BANDED_STORE _mm_store_si128
#define LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED _mm_load_si128
#define LIBMAUS2_LCS_SIMD_BANDED_LOAD_UNALIGNED _mm_loadu_si128
#define LIBMAUS2_LCS_SIMD_BANDED_ANDNOT _mm_andnot_si128
#define LIBMAUS2_LCS_SIMD_BANDED_OR _mm_or_si128

#define LIBMAUS2_LCS_SIMD_BANDED_CMPEQ _mm_cmpeq_epi8
#define LIBMAUS2_LCS_SIMD_BANDED_ADD _mm_adds_epu8
#define LIBMAUS2_LCS_SIMD_BANDED_SHUFFLE _mm_shuffle_epi8
#define LIBMAUS2_LCS_SIMD_BANDED_MIN _mm_min_epu8

#define LIBMAUS2_LCS_SIMD_BANDED_SHIFT_RIGHT libmaus2::lcs::SimdX86GlobalAlignmentConstants128::xmm_8_banded_shift_right
#define LIBMAUS2_LCS_SIMD_BANDED_SELECT_LAST libmaus2::lcs::SimdX86GlobalAlignmentConstants128::xmm_8_banded_select_last
#define LIBMAUS2_LCS_SIMD_BANDED_SHIFT_LEFT libmaus2::lcs::SimdX86GlobalAlignmentConstants128::xmm_8_banded_shift_left

#define LIBMAUS2_LCS_SIMD_BANDED_INIT \
	LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const xslast = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const *>(&LIBMAUS2_LCS_SIMD_BANDED_SELECT_LAST)); \
	LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const x_shift_right = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const *>(&LIBMAUS2_LCS_SIMD_BANDED_SHIFT_RIGHT)); \
	LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const x_shift_left = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const *>(&LIBMAUS2_LCS_SIMD_BANDED_SHIFT_LEFT)); \
	LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const x_select_first_toback = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const *>(&LIBMAUS2_LCS_SIMD_BANDED_SELECT_FIRST_TOBACK));
// shift elements one to right
#define LIBMAUS2_LCS_SIMD_BANDED_SHIFTRIGHT(val) LIBMAUS2_LCS_SIMD_BANDED_SHUFFLE(val,x_shift_right)
// shift elements one to left
#define LIBMAUS2_LCS_SIMD_BANDED_SHIFTLEFT(val) LIBMAUS2_LCS_SIMD_BANDED_SHUFFLE(val,x_shift_left)
// move first element to last and erase rest
#define LIBMAUS2_LCS_SIMD_BANDED_FIRST_TO_BACK(val) LIBMAUS2_LCS_SIMD_BANDED_SHUFFLE(val,x_select_first_toback)
// select last element and move it to first, erase rest
#define LIBMAUS2_LCS_SIMD_BANDED_SELECTLAST(val) LIBMAUS2_LCS_SIMD_BANDED_SHUFFLE(val,xslast)
