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
#include <libmaus2/lcs/SimdX86GlobalAlignmentConstants128.hpp>

#if defined(LIBMAUS2_HAVE_GLOBAL_ALIGNMENT_X128_8)
uint8_t const libmaus2::lcs::SimdX86GlobalAlignmentConstants128::xmm_8_all_ff[sizeof(__m128i)]          __attribute__((aligned(sizeof(__m128i)))) = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
uint8_t const libmaus2::lcs::SimdX86GlobalAlignmentConstants128::xmm_8_first_ff_rest_0[sizeof(__m128i)] __attribute__((aligned(sizeof(__m128i)))) = {0xff,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
uint8_t const libmaus2::lcs::SimdX86GlobalAlignmentConstants128::xmm_8_all_one[sizeof(__m128i)]         __attribute__((aligned(sizeof(__m128i)))) = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
uint8_t const libmaus2::lcs::SimdX86GlobalAlignmentConstants128::xmm_8_shift_right[sizeof(__m128i)]     __attribute__((aligned(sizeof(__m128i)))) = {0x80,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14};
uint8_t const libmaus2::lcs::SimdX86GlobalAlignmentConstants128::xmm_8_select_last[sizeof(__m128i)]     __attribute__((aligned(sizeof(__m128i)))) = {15,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80};
#endif

#if defined(LIBMAUS2_HAVE_GLOBAL_ALIGNMENT_X128_16)
uint16_t const libmaus2::lcs::SimdX86GlobalAlignmentConstants128::xmm_16_all_ff[sizeof(__m128i)/sizeof(uint16_t)]          __attribute__((aligned(sizeof(__m128i)))) = {0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff};
uint16_t const libmaus2::lcs::SimdX86GlobalAlignmentConstants128::xmm_16_first_ff_rest_0[sizeof(__m128i)/sizeof(uint16_t)] __attribute__((aligned(sizeof(__m128i)))) = {0xffff,0,0,0,0,0,0,0};
uint16_t const libmaus2::lcs::SimdX86GlobalAlignmentConstants128::xmm_16_all_one[sizeof(__m128i)/sizeof(uint16_t)]         __attribute__((aligned(sizeof(__m128i)))) = {1,1,1,1,1,1,1,1};
uint16_t const libmaus2::lcs::SimdX86GlobalAlignmentConstants128::xmm_16_shift_right[sizeof(__m128i)/sizeof(uint16_t)]     __attribute__((aligned(sizeof(__m128i)))) = {0x8080,0x0100,0x0302,0x0504,0x0706,0x0908,0x0b0a,0x0d0c};
uint16_t const libmaus2::lcs::SimdX86GlobalAlignmentConstants128::xmm_16_select_last[sizeof(__m128i)/sizeof(uint16_t)]     __attribute__((aligned(sizeof(__m128i)))) = {0x0f0e,0x8080,0x8080,0x8080,0x8080,0x8080,0x8080,0x8080};
#endif
