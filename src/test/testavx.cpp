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

static void process(std::string const & q, std::vector<std::string> const & V)
{
	static uint64_t const wordsize = sizeof(LIBMAUS2_SIMD_WORD_TYPE);
	typedef int32_t gather_word_type;
	static uint64_t const gather_align_size = sizeof(gather_word_type);
	uint64_t const numbatches = (V.size() + wordsize - 1)/wordsize;
	
	static uint64_t const strings_per_gather = (sizeof(LIBMAUS2_SIMD_WORD_TYPE)/sizeof(gather_word_type));
	
	for ( uint64_t batchid = 0; batchid < numbatches; ++batchid )
	{
		uint64_t const batchlow = batchid * wordsize;
		uint64_t const batchhigh = std::min(batchlow + wordsize,static_cast<uint64_t>(V.size()));
		uint64_t const batchsize = batchhigh-batchlow;
		
		std::vector<std::string>::const_iterator itlow = V.begin()+batchlow;
		std::vector<std::string>::const_iterator ithigh = V.begin()+batchhigh;
		std::vector < std::pair<uint64_t,uint64_t> > Vlen;
	
		uint64_t maxlen = 0;
		for ( std::vector<std::string>::const_iterator it = itlow; it != ithigh; ++it )
		{
			maxlen = std::max(maxlen,static_cast<uint64_t>(it->size()));
			Vlen.push_back(
				std::pair<uint64_t,uint64_t>(it->size(),it-itlow)
			);
		}
		
		std::sort(Vlen.begin(),Vlen.end());
		uint64_t vleno = 0;

		// align maxlen to gather length
		maxlen = ((maxlen + gather_align_size - 1) / gather_align_size )*gather_align_size;

		// packing size for gather (number of strings gathered over in first level)
		static uint64_t const pack_size = sizeof(LIBMAUS2_SIMD_WORD_TYPE); //; / sizeof(gather_word_type);
		uint64_t const numpacks = (batchsize + pack_size - 1)/pack_size;

		#if 0
		std::cerr << "maxlen=" << maxlen << std::endl;
		std::cerr << "packsize=" << pack_size << std::endl;
		#endif

		uint64_t const numinputstrings = numpacks*pack_size;
		libmaus2::autoarray::AutoArray<uint8_t,libmaus2::autoarray::alloc_type_memalign_pagesize> A(maxlen * wordsize);
		libmaus2::autoarray::AutoArray<LIBMAUS2_SIMD_WORD_TYPE,libmaus2::autoarray::alloc_type_memalign_pagesize> B(numinputstrings / strings_per_gather);

		libmaus2::autoarray::AutoArray<LIBMAUS2_SIMD_WORD_TYPE,libmaus2::autoarray::alloc_type_memalign_pagesize> M0(q.size()+1);
		libmaus2::autoarray::AutoArray<LIBMAUS2_SIMD_WORD_TYPE,libmaus2::autoarray::alloc_type_memalign_pagesize> M1(q.size()+1);
		for ( uint64_t i = 0; i < M0.size(); ++i )
			M0[i] = _mm256_set1_epi8(i);

		// std::cerr << "B.size()=" << B.size() << std::endl;

		for ( std::vector<std::string>::const_iterator it = itlow; it != ithigh; ++it )
		{
			char const * s = it->c_str();
			std::copy(s,s+it->size(),A.begin() + (it-itlow) * maxlen);
		}

		uint32_t I[8] __attribute__((aligned(sizeof(LIBMAUS2_SIMD_WORD_TYPE))));
		
		#if 0	
		for ( uint64_t i = 0; i < M0.size(); ++i )
		{
			std::cerr << "M0 " << formatRegister(M0[i]) << std::endl;;
		}
		#endif

		uint8_t baseerr = 1;
		for ( uint64_t a = 0; a < maxlen / gather_align_size; ++a )
		{
			for ( uint64_t b = 0; b < numinputstrings / strings_per_gather; ++b )
			{
				//std::cerr << std::endl << std::endl << "a=" << a << " b=" << b << std::endl;

				for ( uint64_t i = 0, j = a*gather_align_size+b*strings_per_gather*maxlen; i < sizeof(I)/sizeof(I[0]); ++i, j += maxlen )
					I[i] = j;

				LIBMAUS2_SIMD_WORD_TYPE const vecI = LIBMAUS2_SIMD_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_SIMD_WORD_TYPE const *>(&I[0]));
				// std::cerr << "vecI=" << formatRegister(vecI) << std::endl;

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

				//std::cerr << "vecu original=" << formatRegister(vecu) << std::endl;

				LIBMAUS2_SIMD_WORD_TYPE const vecshufu = LIBMAUS2_SIMD_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_SIMD_WORD_TYPE const *>(&shufu[0]));

				vecu = LIBMAUS2_SIMD_AND(LIBMAUS2_SIMD_SHUFFLE(vecu,vecshufu),LIBMAUS2_SIMD_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_SIMD_WORD_TYPE const *>(&maxlowu[0])));

				//std::cerr << "vecu shuffled=" << formatRegister(vecu) << std::endl;

				LIBMAUS2_SIMD_WORD_TYPE const vecshufv = LIBMAUS2_SIMD_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_SIMD_WORD_TYPE const *>(&shufv[0]));

				//std::cerr << "vecv original=" << formatRegister(vecv) << std::endl;

				vecv = LIBMAUS2_SIMD_AND(LIBMAUS2_SIMD_SHUFFLE(vecv,vecshufv),LIBMAUS2_SIMD_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_SIMD_WORD_TYPE const *>(&maxlowu[0])));

				//std::cerr << "vecv shuffled=" << formatRegister(vecv) << std::endl;

				vecu = LIBMAUS2_SIMD_OR(vecu,LIBMAUS2_SIMD_PERMUTE(vecu,vecu,1 | (8<<4)));

				//std::cerr << formatRegister(vecu) << std::endl;

				vecv = LIBMAUS2_SIMD_OR(vecv,LIBMAUS2_SIMD_PERMUTE(vecv,vecv,8 | (2<<4)));

				//std::cerr << formatRegister(vecv) << std::endl;

				LIBMAUS2_SIMD_WORD_TYPE const w = LIBMAUS2_SIMD_PERMUTE(vecu,vecv,0 | (3<<4));

				//std::cerr << formatRegister(w) << std::endl;
				//std::cerr << formatRegister(w,V) << std::endl;
				
				LIBMAUS2_SIMD_STORE(B.begin()+b,w);
			}
			
			LIBMAUS2_SIMD_WORD_TYPE const vone = _mm256_set1_epi8(1);

			for ( uint64_t b = 0; b < numinputstrings / strings_per_gather; ++b )
			{
				uint64_t const lbaseerr = baseerr++;
				M1[0] = _mm256_set1_epi8(lbaseerr);
				
				// std::cerr << "base err " << formatRegister(M1[0]) << std::endl;
				
				for ( uint64_t i = 0; i < 4; ++i )
				{
					I[2*i+0] = i * sizeof(LIBMAUS2_SIMD_WORD_TYPE)+0 + b*8;
					I[2*i+1] = i * sizeof(LIBMAUS2_SIMD_WORD_TYPE)+4 + b*8;
				}

				LIBMAUS2_SIMD_WORD_TYPE const vecI = LIBMAUS2_SIMD_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_SIMD_WORD_TYPE const *>(&I[0]));
				// std::cerr << "vecI=" << formatRegister(vecI) << std::endl;

				LIBMAUS2_SIMD_WORD_TYPE vecg = _mm256_i32gather_epi32(reinterpret_cast<int const *>(B.begin()),vecI,1);
				//std::cerr << formatRegister(vecg,V) << std::endl;
				
				for ( uint64_t i = 0; i < q.size(); ++i )
				{
					LIBMAUS2_SIMD_WORD_TYPE const qw = _mm256_set1_epi8(q[i]);
					// std::cerr << "qw=" << formatRegister(qw) << std::endl;
					
					LIBMAUS2_SIMD_WORD_TYPE const top  = _mm256_adds_epi8(M0[i+1],vone);
					LIBMAUS2_SIMD_WORD_TYPE const left = _mm256_adds_epi8(M1[i],vone);
					LIBMAUS2_SIMD_WORD_TYPE const diag = _mm256_adds_epi8(_mm256_andnot_si256(_mm256_cmpeq_epi8(vecg,qw),vone),M0[i]);
					
					M1[i+1] = _mm256_min_epi8(_mm256_min_epi8(top,left),diag);					
				}			
				M0.swap(M1);
				
				while ( vleno < Vlen.size() && Vlen[vleno].first == lbaseerr )
				{
					__m128i const T = Vlen[vleno].second >= 16 ? _mm256_extractf128_si256 (M0[lbaseerr], 1) : _mm256_extractf128_si256 (M0[lbaseerr], 0);
					int e;
					
					switch ( Vlen[vleno].second & 0xf )
					{
						case 0: e = _mm_extract_epi8(T,0); break;
						case 1: e = _mm_extract_epi8(T,1); break;
						case 2: e = _mm_extract_epi8(T,2); break;
						case 3: e = _mm_extract_epi8(T,3); break;
						case 4: e = _mm_extract_epi8(T,4); break;
						case 5: e = _mm_extract_epi8(T,5); break;
						case 6: e = _mm_extract_epi8(T,6); break;
						case 7: e = _mm_extract_epi8(T,7); break;
						case 8: e = _mm_extract_epi8(T,8); break;
						case 9: e = _mm_extract_epi8(T,9); break;
						case 10: e = _mm_extract_epi8(T,10); break;
						case 11: e = _mm_extract_epi8(T,11); break;
						case 12: e = _mm_extract_epi8(T,12); break;
						case 13: e = _mm_extract_epi8(T,13); break;
						case 14: e = _mm_extract_epi8(T,14); break;
						case 15: e = _mm_extract_epi8(T,15); break;
					}
					
					std::cerr << "end of " << Vlen[vleno].second << " error " << e << std::endl;
					++vleno;
				}
			}
		}
	}
}

int main()
{
	std::vector<std::string> V;
	for ( uint64_t i = 0; i < 5; ++i )
		V.push_back(numString(i*11,(i+1)*11));

	V[0][5] = 7;

	std::string q = numString(0,11);
	process(q,V);

	#if 0
	uint8_t A[] __attribute__((aligned(sizeof(LIBMAUS2_SIMD_WORD_TYPE)))) = {
		0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
		16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
	};
	__m256i i = LIBMAUS2_SIMD_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_SIMD_WORD_TYPE const *>(&A[0]));
	std::cerr << formatRegister(i) << std::endl;
	#endif
}
