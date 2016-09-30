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

#include <immintrin.h>
#include <ostream>
#include <sstream>

#define LIBMAUS2_SIMD_WORD_TYPE       __m256i
#define LIBMAUS2_SIMD_STORE           _mm256_store_si256
#define LIBMAUS2_SIMD_LOAD_ALIGNED    _mm256_load_si256
#define LIBMAUS2_SIMD_LOAD_UNALIGNED  _mm256_loadu_si256
#define LIBMAUS2_SIMD_ELEMENT_TYPE    uint8_t
#define LIBMAUS2_SIMD_SHUFFLE         _mm256_shuffle_epi8
#define LIBMAUS2_SIMD_AND             _mm256_and_si256
#define LIBMAUS2_SIMD_OR              _mm256_or_si256
#define LIBMAUS2_SIMD_PERMUTE         _mm256_permute2f128_si256

static std::ostream & printRegister(std::ostream & out, LIBMAUS2_SIMD_WORD_TYPE const reg)
{
	LIBMAUS2_SIMD_ELEMENT_TYPE sp[sizeof(reg)/sizeof(LIBMAUS2_SIMD_ELEMENT_TYPE)] __attribute__((aligned(sizeof(reg))));
	LIBMAUS2_SIMD_STORE(reinterpret_cast<LIBMAUS2_SIMD_WORD_TYPE *>(&sp[0]),reg);
	for ( uint64_t i = 0; i < sizeof(reg)/sizeof(LIBMAUS2_SIMD_ELEMENT_TYPE); ++i )
		out << static_cast<int>(sp[i]) <<
			((i+1<sizeof(reg)/sizeof(LIBMAUS2_SIMD_ELEMENT_TYPE))?",":"");
	return out;
}

static std::ostream & printRegisterChar(std::ostream & out, LIBMAUS2_SIMD_WORD_TYPE const reg)
{
	LIBMAUS2_SIMD_ELEMENT_TYPE sp[sizeof(reg)/sizeof(LIBMAUS2_SIMD_ELEMENT_TYPE)] __attribute__((aligned(sizeof(reg))));
	LIBMAUS2_SIMD_STORE(reinterpret_cast<LIBMAUS2_SIMD_WORD_TYPE *>(&sp[0]),reg);
	for ( uint64_t i = 0; i < sizeof(reg)/sizeof(LIBMAUS2_SIMD_ELEMENT_TYPE); ++i )
		out << (sp[i]) <<
			((i+1<(sizeof(reg)/sizeof(LIBMAUS2_SIMD_ELEMENT_TYPE)))?",":"");
	return out;
}

static std::ostream & printRegisterCharNoSep(std::ostream & out, LIBMAUS2_SIMD_WORD_TYPE const reg)
{
	LIBMAUS2_SIMD_ELEMENT_TYPE sp[sizeof(reg)/sizeof(LIBMAUS2_SIMD_ELEMENT_TYPE)] __attribute__((aligned(sizeof(reg))));
	LIBMAUS2_SIMD_STORE(reinterpret_cast<LIBMAUS2_SIMD_WORD_TYPE *>(&sp[0]),reg);
	for ( uint64_t i = 0; i < sizeof(reg)/sizeof(LIBMAUS2_SIMD_ELEMENT_TYPE); ++i )
		out << (sp[i]);
	return out;
}

static std::string formatRegister(LIBMAUS2_SIMD_WORD_TYPE const reg)
{
	std::ostringstream ostr;
	printRegister(ostr,reg);
	return ostr.str();
}

#include <iostream>
#include <vector>

static std::string formatRegister(LIBMAUS2_SIMD_WORD_TYPE const reg, std::vector<std::string> const & V)
{
	std::ostringstream ostr;
	printRegisterCharNoSep(ostr,reg);
	std::string s = ostr.str();
	std::string t(std::string(2*s.size(),' '));
	for ( uint64_t i = 0; i < s.size(); ++i )
	{
		char const c = s[i];

		//std::cerr << (int)(unsigned char)c << std::endl;

		for ( uint64_t j = 0; j < V.size(); ++j )
			if ( V[j].find(c) != std::string::npos )
			{
				if ( 'a' + j <= 'z' )
				{
					t[2*i+0] = 'a' + j;
				}
				else
				{
					t[2*i+0] = 'A' + j-('z'-'a'+1);
				}

				t[2*i+1] = '0' + V[j].find(c);
			}
	}

	return t;
}


static std::string numString(uint64_t const from, uint64_t const to)
{
	std::string s(to-from,'\0');

	for ( uint64_t i = 0; i < s.size(); ++i )
		s[i] = from+i;

	return s;
}

#include <libmaus2/autoarray/AutoArray.hpp>

// _mm256_set1_epi8

static void process(std::vector<std::string> const & V)
{
	uint64_t maxlen = 0;
	for ( uint64_t i = 0; i < V.size(); ++i )
		maxlen = std::max(maxlen,static_cast<uint64_t>(V[i].size()));

	typedef int32_t gather_word_type;
	static uint64_t const gather_align_size = sizeof(gather_word_type);
	// align maxlen to gather length
	maxlen = ((maxlen + gather_align_size - 1) / gather_align_size )*gather_align_size;
	// packing size for gather (number of strings gathered over in first level)
	static uint64_t const pack_size = sizeof(LIBMAUS2_SIMD_WORD_TYPE); //; / sizeof(gather_word_type);
	uint64_t const numpacks = (V.size() + pack_size - 1)/pack_size;

	std::cerr << "maxlen=" << maxlen << std::endl;
	std::cerr << "packsize=" << pack_size << std::endl;

	uint64_t const numinputstrings = numpacks*pack_size;
	libmaus2::autoarray::AutoArray<uint8_t,libmaus2::autoarray::alloc_type_memalign_pagesize> A(maxlen * numinputstrings);
	// ( A.size() + 1 ) * numstrings
	libmaus2::autoarray::AutoArray<LIBMAUS2_SIMD_WORD_TYPE,libmaus2::autoarray::alloc_type_memalign_pagesize> B(1);

	for ( uint64_t i = 0; i < V.size(); ++i )
	{
		char const * s = V[i].c_str();
		std::copy(s,s+V[i].size(),A.begin() + i * maxlen);
	}

	uint32_t I[8] __attribute__((aligned(sizeof(LIBMAUS2_SIMD_WORD_TYPE))));

	static uint64_t const strings_per_gather = (sizeof(LIBMAUS2_SIMD_WORD_TYPE)/sizeof(gather_word_type));

	for ( uint64_t a = 0; a < maxlen / gather_align_size; ++a )
		for ( uint64_t b = 0; b < numinputstrings / strings_per_gather; ++b )
		{
			std::cerr << std::endl << std::endl << "a=" << a << " b=" << b << std::endl;

			for ( uint64_t i = 0, j = a*gather_align_size+b*strings_per_gather*maxlen; i < sizeof(I)/sizeof(I[0]); ++i, j += maxlen )
				I[i] = j;

			LIBMAUS2_SIMD_WORD_TYPE const vecI = LIBMAUS2_SIMD_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_SIMD_WORD_TYPE const *>(&I[0]));
			std::cerr << "vecI=" << formatRegister(vecI) << std::endl;

			LIBMAUS2_SIMD_WORD_TYPE vecu = _mm256_i32gather_epi32(reinterpret_cast<int const *>(A.begin()),vecI,1);

			static uint8_t const shufu[] __attribute__((aligned(sizeof(LIBMAUS2_SIMD_WORD_TYPE)))) = {
				0,4,8,12,2,6,10,14,1,5,9,13,3,7,11,15,
				2,6,10,14,0,4,8,12,3,7,11,15,1,5,9,13
			};
			static uint8_t const shufv[] __attribute__((aligned(sizeof(LIBMAUS2_SIMD_WORD_TYPE)))) = {
				2,6,10,14,0,4,8,12,3,7,11,15,1,5,9,13,
				0,4,8,12,2,6,10,14,1,5,9,13,3,7,11,15
			};
			static uint8_t const maxlowu[] __attribute__((aligned(sizeof(LIBMAUS2_SIMD_WORD_TYPE)))) = {
				0xff,0xff,0xff,0xff,0,0,0,0,0xff,0xff,0xff,0xff,0,0,0,0,
				0,0,0,0,0xff,0xff,0xff,0xff,0,0,0,0,0xff,0xff,0xff,0xff
			};
			static uint8_t const maxlowv[] __attribute__((aligned(sizeof(LIBMAUS2_SIMD_WORD_TYPE)))) = {
				0,0,0,0,0xff,0xff,0xff,0xff,0,0,0,0,0xff,0xff,0xff,0xff,
				0xff,0xff,0xff,0xff,0,0,0,0,0xff,0xff,0xff,0xff,0,0,0,0
			};

			LIBMAUS2_SIMD_WORD_TYPE vecv = vecu;

			std::cerr << "vecu original=" << formatRegister(vecu) << std::endl;

			LIBMAUS2_SIMD_WORD_TYPE const vecshufu = LIBMAUS2_SIMD_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_SIMD_WORD_TYPE const *>(&shufu[0]));

			vecu = LIBMAUS2_SIMD_AND(LIBMAUS2_SIMD_SHUFFLE(vecu,vecshufu),LIBMAUS2_SIMD_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_SIMD_WORD_TYPE const *>(&maxlowu[0])));

			std::cerr << "vecu shuffled=" << formatRegister(vecu) << std::endl;

			LIBMAUS2_SIMD_WORD_TYPE const vecshufv = LIBMAUS2_SIMD_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_SIMD_WORD_TYPE const *>(&shufv[0]));

			std::cerr << "vecv original=" << formatRegister(vecv) << std::endl;


			vecv = LIBMAUS2_SIMD_AND(LIBMAUS2_SIMD_SHUFFLE(vecv,vecshufv),LIBMAUS2_SIMD_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_SIMD_WORD_TYPE const *>(&maxlowu[0])));

			std::cerr << "vecv shuffled=" << formatRegister(vecv) << std::endl;

			vecu = LIBMAUS2_SIMD_OR(vecu,LIBMAUS2_SIMD_PERMUTE(vecu,vecu,1 | (8<<4)));

			std::cerr << formatRegister(vecu) << std::endl;

			vecv = LIBMAUS2_SIMD_OR(vecv,LIBMAUS2_SIMD_PERMUTE(vecv,vecv,8 | (2<<4)));

			std::cerr << formatRegister(vecv) << std::endl;

			LIBMAUS2_SIMD_WORD_TYPE const w = LIBMAUS2_SIMD_PERMUTE(vecu,vecv,0 | (3<<4));

			std::cerr << formatRegister(w) << std::endl;
			std::cerr << formatRegister(w,V) << std::endl;
		}
}

int main()
{
	std::vector<std::string> V;
	for ( uint64_t i = 0; i < 32; ++i )
		V.push_back(numString(i*8,(i+1)*8));

	process(V);

	#if 0
	uint8_t A[] __attribute__((aligned(sizeof(LIBMAUS2_SIMD_WORD_TYPE)))) = {
		0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
		16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
	};
	__m256i i = LIBMAUS2_SIMD_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_SIMD_WORD_TYPE const *>(&A[0]));
	std::cerr << formatRegister(i) << std::endl;
	#endif
}
