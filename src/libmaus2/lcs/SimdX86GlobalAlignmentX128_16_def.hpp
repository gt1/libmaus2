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
#define LIBMAUS2_SIMD_CLASS_NAME SimdX86GlobalAlignmentX128_16
#define LIBMAUS2_SIMD_STORE _mm_store_si128
#define LIBMAUS2_SIMD_LOAD_ALIGNED _mm_load_si128
#define LIBMAUS2_SIMD_LOAD_UNALIGNED _mm_loadu_si128
#define LIBMAUS2_SIMD_ANDNOT _mm_andnot_si128
#define LIBMAUS2_SIMD_OR _mm_or_si128

#define LIBMAUS2_SIMD_CMPEQ _mm_cmpeq_epi16
#define LIBMAUS2_SIMD_ADD _mm_adds_epu16
#define LIBMAUS2_SIMD_SHUFFLE _mm_shuffle_epi8
#define LIBMAUS2_SIMD_MIN _mm_min_epu16

#define LIBMAUS2_SIMD_ALL_FF libmaus2::lcs::SimdX86GlobalAlignmentConstants128::xmm_16_all_ff
#define LIBMAUS2_SIMD_FIRST_FF_REST_0 libmaus2::lcs::SimdX86GlobalAlignmentConstants128::xmm_16_first_ff_rest_0
#define LIBMAUS2_SIMD_ALL_ONE libmaus2::lcs::SimdX86GlobalAlignmentConstants128::xmm_16_all_one
#define LIBMAUS2_SIMD_SHIFT_RIGHT libmaus2::lcs::SimdX86GlobalAlignmentConstants128::xmm_16_shift_right
#define LIBMAUS2_SIMD_SELECT_LAST libmaus2::lcs::SimdX86GlobalAlignmentConstants128::xmm_16_select_last
#define LIBMAUS2_SIMD_WORD_TYPE __m128i
#define LIBMAUS2_SIMD_ELEMENT_TYPE uint16_t
#define LIBMAUS2_SIMD_INIT \
	LIBMAUS2_SIMD_WORD_TYPE const xslast = LIBMAUS2_SIMD_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_SIMD_WORD_TYPE const *>(&LIBMAUS2_SIMD_SELECT_LAST)); \
	LIBMAUS2_SIMD_WORD_TYPE const x_shift_right = LIBMAUS2_SIMD_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_SIMD_WORD_TYPE const *>(&LIBMAUS2_SIMD_SHIFT_RIGHT));
#define LIBMAUS2_SIMD_SHIFTRIGHT(val) LIBMAUS2_SIMD_SHUFFLE(val,x_shift_right)
#define LIBMAUS2_SIMD_SELECTLAST(val) LIBMAUS2_SIMD_SHUFFLE(val,xslast)
