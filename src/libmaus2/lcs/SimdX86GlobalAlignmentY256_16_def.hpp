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
#define LIBMAUS2_SIMD_CLASS_NAME      SimdX86GlobalAlignmentY256_16
#define LIBMAUS2_SIMD_STORE           _mm256_store_si256
#define LIBMAUS2_SIMD_LOAD_ALIGNED    _mm256_load_si256
#define LIBMAUS2_SIMD_LOAD_UNALIGNED  _mm256_loadu_si256
#define LIBMAUS2_SIMD_ANDNOT          _mm256_andnot_si256
#define LIBMAUS2_SIMD_OR              _mm256_or_si256
#define LIBMAUS2_SIMD_CMPEQ           _mm256_cmpeq_epi16
#define LIBMAUS2_SIMD_ADD             _mm256_adds_epu16
#define LIBMAUS2_SIMD_SHUFFLE         _mm256_shuffle_epi8
#define LIBMAUS2_SIMD_MIN             _mm256_min_epu16
#define LIBMAUS2_SIMD_ALL_FF          libmaus2::lcs::SimdX86GlobalAlignmentConstants256::ymm_16_all_ff
#define LIBMAUS2_SIMD_FIRST_FF_REST_0 libmaus2::lcs::SimdX86GlobalAlignmentConstants256::ymm_16_first_ff_rest_0
#define LIBMAUS2_SIMD_ALL_ONE         libmaus2::lcs::SimdX86GlobalAlignmentConstants256::ymm_16_all_one
#define LIBMAUS2_SIMD_WORD_TYPE       __m256i
#define LIBMAUS2_SIMD_ELEMENT_TYPE    uint16_t
#define LIBMAUS2_SIMD_INIT \
	LIBMAUS2_SIMD_WORD_TYPE const x_shiftover = LIBMAUS2_SIMD_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_SIMD_WORD_TYPE const *>(&libmaus2::lcs::SimdX86GlobalAlignmentConstants256::ymm_16_shiftover[0])); \
	LIBMAUS2_SIMD_WORD_TYPE const x_mask16 = LIBMAUS2_SIMD_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_SIMD_WORD_TYPE const *>(&libmaus2::lcs::SimdX86GlobalAlignmentConstants256::ymm_16_mask16[0])); \
	LIBMAUS2_SIMD_WORD_TYPE const x_select15 = LIBMAUS2_SIMD_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_SIMD_WORD_TYPE const *>(&libmaus2::lcs::SimdX86GlobalAlignmentConstants256::ymm_16_select_15));
#define LIBMAUS2_SIMD_SHIFTRIGHT(val) \
	_mm256_or_si256( \
		_mm256_shuffle_epi8(val,x_shiftover), \
		_mm256_shuffle_epi8(_mm256_permute2f128_si256(val,val,8),x_mask16) \
	)
#define LIBMAUS2_SIMD_SELECTLAST(v) LIBMAUS2_SIMD_SHUFFLE(_mm256_permute2f128_si256(v,v,1),x_select15)

