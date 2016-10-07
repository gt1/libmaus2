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

#include <libmaus2/lcs/AlignmentOneAgainstManyAVX2.hpp>
#include <libmaus2/autoarray/GenericAlignedAllocation.hpp>
#include <libmaus2/lcs/NP.hpp>
#include <libmaus2/aio/StreamLock.hpp>

#include <ostream>
#include <sstream>
#include <iostream>

#if defined(LIBMAUS2_HAVE_AVX2)
#include <immintrin.h>

#define LIBMAUS2_SIMD_WORD_TYPE       __m256i
#define LIBMAUS2_SIMD_STORE           _mm256_store_si256
#define LIBMAUS2_SIMD_LOAD_ALIGNED    _mm256_load_si256
#define LIBMAUS2_SIMD_LOAD_UNALIGNED  _mm256_loadu_si256
#define LIBMAUS2_SIMD_ELEMENT_TYPE    uint8_t
#define LIBMAUS2_SIMD_SHUFFLE         _mm256_shuffle_epi8
#define LIBMAUS2_SIMD_AND             _mm256_and_si256
#define LIBMAUS2_SIMD_OR              _mm256_or_si256
#define LIBMAUS2_SIMD_PERMUTE         _mm256_permute2f128_si256
#define LIBMAUS2_SIMD_SET1            _mm256_set1_epi8
#define LIBMAUS2_SIMD_ADDS            _mm256_adds_epu8
#define LIBMAUS2_SIMD_GATHER          _mm256_i32gather_epi32
#define LIBMAUS2_SIMD_ANDNOT          _mm256_andnot_si256
#define LIBMAUS2_SIMD_CMPEQ           _mm256_cmpeq_epi8
#define LIBMAUS2_SIMD_MIN             _mm256_min_epu8
#define LIBMAUS2_SIMD_EXTRACT128      _mm256_extractf128_si256
#define LIBMAUS2_SIMD_EXTRACT8        _mm_extract_epi8

// #define AVX2_DEBUG

#if defined(AVX2_DEBUG)
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

static std::string formatRegisterChar(LIBMAUS2_SIMD_WORD_TYPE const reg)
{
	std::ostringstream ostr;
	printRegisterChar(ostr,reg);
	return ostr.str();
}

static std::string formatRegisterCharNoSep(LIBMAUS2_SIMD_WORD_TYPE const reg)
{
	std::ostringstream ostr;
	printRegisterCharNoSep(ostr,reg);
	return ostr.str();
}

static std::string formatRegister(
	LIBMAUS2_SIMD_WORD_TYPE const reg,
	std::pair<uint8_t const *,uint64_t> const * MA,
	uint64_t const MAo
)
{
	std::ostringstream ostr;
	printRegisterCharNoSep(ostr,reg);
	std::string s = ostr.str();
	std::string t(std::string(2*s.size(),' '));
	for ( uint64_t i = 0; i < s.size(); ++i )
	{
		char const c = s[i];

		//std::cerr << (int)(unsigned char)c << std::endl;

		for ( uint64_t j = 0; j < MAo; ++j )
		{
			std::string const q(MA[j].first,MA[j].first + MA[j].second);

			if ( q.find(c) != std::string::npos )
			{
				if ( 'a' + j <= 'z' )
				{
					t[2*i+0] = 'a' + j;
				}
				else
				{
					t[2*i+0] = 'A' + j-('z'-'a'+1);
				}

				t[2*i+1] = '0' + q.find(c);
			}
		}
	}

	return t;
}
#endif


struct AlignmentOneAgainstManyAVX2Context
{
	typedef int32_t gather_word_type;
	// SIMD word size in bytes
	static uint64_t const wordsize = sizeof(LIBMAUS2_SIMD_WORD_TYPE);
	// size of gather word type
	static uint64_t const gather_align_size = sizeof(gather_word_type);
	//
	static uint64_t const Isize = sizeof(LIBMAUS2_SIMD_WORD_TYPE)/sizeof(gather_word_type);

	// number of strings we gather into a SIMD word in the first round
	static uint64_t const strings_per_gather = (sizeof(LIBMAUS2_SIMD_WORD_TYPE)/sizeof(gather_word_type));

	private:
	uint64_t M0size;
	libmaus2::util::Destructable::unique_ptr_type M0;
	uint64_t M1size;
	libmaus2::util::Destructable::unique_ptr_type M1;

	uint64_t EM0size;
	libmaus2::util::Destructable::unique_ptr_type EM0;
	uint64_t EM1size;
	libmaus2::util::Destructable::unique_ptr_type EM1;

	uint64_t Asize;
	libmaus2::util::Destructable::unique_ptr_type A;

	libmaus2::util::Destructable::unique_ptr_type B;

	// LIBMAUS2_SIMD_WORD_TYPE B[sizeof(LIBMAUS2_SIMD_WORD_TYPE) / strings_per_gather] __attribute__((aligned(sizeof(LIBMAUS2_SIMD_WORD_TYPE))));

	libmaus2::util::Destructable::unique_ptr_type I;

	public:
	AlignmentOneAgainstManyAVX2Context()
	: M0size(0), M1size(0), EM0size(0), EM1size(0), Asize(0),
	  B(libmaus2::autoarray::GenericAlignedAllocation::allocateU(
	  	sizeof(LIBMAUS2_SIMD_WORD_TYPE) * (sizeof(LIBMAUS2_SIMD_WORD_TYPE) / strings_per_gather),
	  	sizeof(LIBMAUS2_SIMD_WORD_TYPE),
	  	sizeof(LIBMAUS2_SIMD_WORD_TYPE))),
	  I(libmaus2::autoarray::GenericAlignedAllocation::allocateU(
	  	sizeof(LIBMAUS2_SIMD_WORD_TYPE),
	  	sizeof(LIBMAUS2_SIMD_WORD_TYPE),
	  	sizeof(LIBMAUS2_SIMD_WORD_TYPE)))
	{

	}

	LIBMAUS2_SIMD_WORD_TYPE * getM0()
	{
		return reinterpret_cast<LIBMAUS2_SIMD_WORD_TYPE *>(M0->getObject());
	}

	LIBMAUS2_SIMD_WORD_TYPE * getM1()
	{
		return reinterpret_cast<LIBMAUS2_SIMD_WORD_TYPE *>(M1->getObject());
	}

	LIBMAUS2_SIMD_WORD_TYPE * getEM0()
	{
		return reinterpret_cast<LIBMAUS2_SIMD_WORD_TYPE *>(EM0->getObject());
	}

	LIBMAUS2_SIMD_WORD_TYPE * getEM1()
	{
		return reinterpret_cast<LIBMAUS2_SIMD_WORD_TYPE *>(EM1->getObject());
	}

	uint8_t * getA()
	{
		return reinterpret_cast<uint8_t *>(A->getObject());
	}

	uint32_t * getI()
	{
		return reinterpret_cast<uint32_t *>(I->getObject());
	}

	LIBMAUS2_SIMD_WORD_TYPE * getB()
	{
		return reinterpret_cast<LIBMAUS2_SIMD_WORD_TYPE *>(B->getObject());
	}

	void ensureSizeM0(uint64_t const n)
	{
		if ( n > M0size )
		{
			libmaus2::util::Destructable::unique_ptr_type TM0(
				libmaus2::autoarray::GenericAlignedAllocation::allocateU(
					n * sizeof(LIBMAUS2_SIMD_WORD_TYPE),
					sizeof(LIBMAUS2_SIMD_WORD_TYPE),
					sizeof(LIBMAUS2_SIMD_WORD_TYPE)
				)
			);
			M0 = UNIQUE_PTR_MOVE(TM0);
			M0size = n;
		}
	}

	void ensureSizeM1(uint64_t const n)
	{
		if ( n > M1size )
		{
			libmaus2::util::Destructable::unique_ptr_type TM1(
				libmaus2::autoarray::GenericAlignedAllocation::allocateU(
					n * sizeof(LIBMAUS2_SIMD_WORD_TYPE),
					sizeof(LIBMAUS2_SIMD_WORD_TYPE),
					sizeof(LIBMAUS2_SIMD_WORD_TYPE)
				)
			);
			M1 = UNIQUE_PTR_MOVE(TM1);
			M1size = n;
		}
	}

	void ensureSizeEM0(uint64_t const n)
	{
		if ( n > EM0size )
		{
			libmaus2::util::Destructable::unique_ptr_type TEM0(
				libmaus2::autoarray::GenericAlignedAllocation::allocateU(
					n * sizeof(LIBMAUS2_SIMD_WORD_TYPE),
					sizeof(LIBMAUS2_SIMD_WORD_TYPE),
					sizeof(LIBMAUS2_SIMD_WORD_TYPE)
				)
			);
			EM0 = UNIQUE_PTR_MOVE(TEM0);
			EM0size = n;
		}
	}

	void ensureSizeEM1(uint64_t const n)
	{
		if ( n > EM1size )
		{
			libmaus2::util::Destructable::unique_ptr_type TEM1(
				libmaus2::autoarray::GenericAlignedAllocation::allocateU(
					n * sizeof(LIBMAUS2_SIMD_WORD_TYPE),
					sizeof(LIBMAUS2_SIMD_WORD_TYPE),
					sizeof(LIBMAUS2_SIMD_WORD_TYPE)
				)
			);
			EM1 = UNIQUE_PTR_MOVE(TEM1);
			EM1size = n;
		}
	}

	void ensureSizeA(uint64_t const n)
	{
		if ( n > Asize )
		{
			libmaus2::util::Destructable::unique_ptr_type TA(
				libmaus2::autoarray::GenericAlignedAllocation::allocateU(
					n,
					sizeof(LIBMAUS2_SIMD_WORD_TYPE),
					1
				)
			);
			A = UNIQUE_PTR_MOVE(TA);
			Asize = n;
		}
	}
};

void destructAlignmentOneAgainstManyAVX2Context(void * object)
{
	AlignmentOneAgainstManyAVX2Context * p = reinterpret_cast<AlignmentOneAgainstManyAVX2Context *>(object);
	delete p;
}
#endif

libmaus2::lcs::AlignmentOneAgainstManyAVX2::AlignmentOneAgainstManyAVX2()
{
	#if defined(LIBMAUS2_HAVE_AVX2)
	AlignmentOneAgainstManyAVX2Context * ccontext = NULL;

	try
	{
		ccontext = new AlignmentOneAgainstManyAVX2Context;
		libmaus2::util::Destructable::unique_ptr_type tcontext(
			libmaus2::util::Destructable::construct(
				ccontext,destructAlignmentOneAgainstManyAVX2Context
			)
		);
		ccontext = NULL;
		context = UNIQUE_PTR_MOVE(tcontext);
	}
	catch(...)
	{
		delete ccontext;
		throw;
	}
	#endif
}


void libmaus2::lcs::AlignmentOneAgainstManyAVX2::process(
	uint8_t const * qa,
	uint8_t const * qe,
	std::pair<uint8_t const *,uint64_t> const * MA,
	uint64_t const MAo,
	libmaus2::autoarray::AutoArray<uint64_t> & E
)
{
	E.ensureSize(MAo);

	#if defined(LIBMAUS2_HAVE_AVX2)
	AlignmentOneAgainstManyAVX2Context * ccontext = reinterpret_cast<AlignmentOneAgainstManyAVX2Context *>(context->getObject());

	uint64_t const qsize = qe-qa;
	uint64_t const Msize = qsize+1;
	ccontext->ensureSizeM0(Msize);
	ccontext->ensureSizeM1(Msize);
	LIBMAUS2_SIMD_WORD_TYPE * M0 = ccontext->getM0();
	LIBMAUS2_SIMD_WORD_TYPE * M1 = ccontext->getM1();
	uint32_t * const I = ccontext->getI();
	LIBMAUS2_SIMD_WORD_TYPE * const B = ccontext->getB();

	#if 0
	ccontext->ensureSizeEM0(Msize);
	ccontext->ensureSizeEM1(Msize);
	LIBMAUS2_SIMD_WORD_TYPE * EM0 = ccontext->getEM0();
	LIBMAUS2_SIMD_WORD_TYPE * EM1 = ccontext->getEM1();
	#endif

	// batch size for processing strings in V
	uint64_t const numbatches = (MAo + AlignmentOneAgainstManyAVX2Context::wordsize - 1)/AlignmentOneAgainstManyAVX2Context::wordsize;

	for ( uint64_t batchid = 0; batchid < numbatches; ++batchid )
	{
		// batch low pointer
		uint64_t const batchlow = batchid * AlignmentOneAgainstManyAVX2Context::wordsize;
		// batch high pointer
		uint64_t const batchhigh = std::min(batchlow + AlignmentOneAgainstManyAVX2Context::wordsize,MAo);
		// size of this batch
		uint64_t const batchsize = batchhigh-batchlow;

		// iterator low
		std::pair<uint8_t const *,uint64_t> const * itlow = MA + batchlow;
		// iterator high
		std::pair<uint8_t const *,uint64_t> const * ithigh = MA + batchhigh;
		// string length,string id pairs
		// std::vector < std::pair<uint64_t,uint64_t> > Vlen;
		libmaus2::autoarray::AutoArray < std::pair<uint64_t,uint64_t> > Vlen;
		uint64_t vleni = 0;

		// compute maximum string length and (length,id) pairs
		uint64_t maxlen = 0;
		for ( std::pair<uint8_t const *,uint64_t> const * it = itlow; it != ithigh; ++it )
		{
			maxlen = std::max(maxlen,static_cast<uint64_t>(it->second));
			Vlen.push(vleni,std::pair<uint64_t,uint64_t>(it->second,it-itlow));
		}

		// sort (length,id) pairs by (length,id)
		std::sort(Vlen.begin(),Vlen.begin()+vleni);
		// scan pointer for Vlen
		uint64_t vleno = 0;

		// align maxlen to gather length by rounding it up to the next multiple of AlignmentOneAgainstManyAVX2Context::gather_align_size
		maxlen = ((maxlen + AlignmentOneAgainstManyAVX2Context::gather_align_size - 1) / AlignmentOneAgainstManyAVX2Context::gather_align_size )*AlignmentOneAgainstManyAVX2Context::gather_align_size;

		// words per string
		uint64_t const textwordsperstring = (maxlen + AlignmentOneAgainstManyAVX2Context::wordsize - 1)/AlignmentOneAgainstManyAVX2Context::wordsize;
		// bytes per string
		uint64_t const textbytesperstring = textwordsperstring * AlignmentOneAgainstManyAVX2Context::wordsize;
		// text bytes (string length * number of string)
		uint64_t const textbytes = textbytesperstring * AlignmentOneAgainstManyAVX2Context::wordsize;

		ccontext->ensureSizeA(textbytes);
		uint8_t * const A = ccontext->getA();

		// copy strings
		for ( std::pair<uint8_t const *,uint64_t> const * it = itlow; it != ithigh; ++it )
		{
			std::copy(it->first,it->first + it->second,A + (it-itlow) * maxlen);
			// std::cerr << "copied " << std::string(A + (it-itlow) * maxlen, A + (it-itlow) * maxlen+it->second) << std::endl;
		}

		uint64_t const gather_loops = (batchsize + AlignmentOneAgainstManyAVX2Context::strings_per_gather - 1)/AlignmentOneAgainstManyAVX2Context::strings_per_gather;

		// std::cerr << "gather_loops=" << gather_loops << std::endl;

		// packing size for gather (number of strings gathered over in first level)
		// static uint64_t const pack_size = sizeof(LIBMAUS2_SIMD_WORD_TYPE); //; / sizeof(gather_word_type);
		// uint64_t const numpacks = (batchsize + pack_size - 1)/pack_size;

		// assert ( numpacks == 1 );

		#if 0
		std::cerr << "maxlen=" << maxlen << std::endl;
		std::cerr << "packsize=" << pack_size << std::endl;
		#endif

		// uint64_t const numinputstrings = numpacks*pack_size;

		// first row of alignment matrix for all strings in batch
		for ( uint64_t i = 0; i < Msize; ++i )
		{
			M0[i] = LIBMAUS2_SIMD_SET1(i);
			// std::cerr << "M0[" << i << "]=" << formatRegister(M0[i]) << std::endl;
		}

		uint8_t baseerr = 1;
		// iterate of string gather intervals (blocks of 4 bytes)
		for ( uint64_t a = 0; a < maxlen / AlignmentOneAgainstManyAVX2Context::gather_align_size; ++a )
		{
			// std::cerr << "a=" << a << std::endl;

			for ( uint64_t b = 0; b < gather_loops; ++b )
			{
				//std::cerr << std::endl << std::endl << "a=" << a << " b=" << b << std::endl;

				for (
					uint64_t i = 0,
					j =
						a*AlignmentOneAgainstManyAVX2Context::gather_align_size+
						b*AlignmentOneAgainstManyAVX2Context::strings_per_gather*maxlen;
						i < AlignmentOneAgainstManyAVX2Context::Isize;
						++i, j += maxlen
				)
					I[i] = j;

				LIBMAUS2_SIMD_WORD_TYPE const vecI = LIBMAUS2_SIMD_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_SIMD_WORD_TYPE const *>(I));
				//std::cerr << "vecI=" << formatRegister(vecI) << std::endl;

				LIBMAUS2_SIMD_WORD_TYPE vecu = LIBMAUS2_SIMD_GATHER(reinterpret_cast<int const *>(A),vecI,1);

				//std::cerr << formatRegisterChar(vecu) << std::endl;

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

				#if 0
				static uint8_t const maxlowv[] __attribute__((aligned(sizeof(LIBMAUS2_SIMD_WORD_TYPE)))) = {
					0,0,0,0,0xff,0xff,0xff,0xff,0,0,0,0,0xff,0xff,0xff,0xff,
					0xff,0xff,0xff,0xff,0,0,0,0,0xff,0xff,0xff,0xff,0,0,0,0
				};
				#endif

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

				// std::cerr << formatRegisterCharNoSep(w) << std::endl;
				//std::cerr << formatRegister(w,V) << std::endl;

				#if 0
				for ( uint64_t i = 0; i < 32; ++i )
				{
					uint64_t const stringid = batchlow + b * AlignmentOneAgainstManyAVX2Context::strings_per_gather + (i % 8);
					uint64_t const offset = i / 8 + a * AlignmentOneAgainstManyAVX2Context::gather_align_size;

					// std::cerr << "i=" << i << " stringid=" << stringid << " offset=" << offset << std::endl;

					if ( stringid < MAo && offset < MA[stringid].second )
						assert ( formatRegisterCharNoSep(w)[i] == MA[stringid].first[offset] );
				}
				#endif

				LIBMAUS2_SIMD_STORE(B+b,w);
			}

			LIBMAUS2_SIMD_WORD_TYPE const vone = LIBMAUS2_SIMD_SET1(1);

			for ( uint64_t b = 0; b < sizeof(LIBMAUS2_SIMD_WORD_TYPE) / AlignmentOneAgainstManyAVX2Context::strings_per_gather; ++b )
			{
				uint64_t const lbaseerr = baseerr++;
				M1[0] = LIBMAUS2_SIMD_SET1(lbaseerr);

				// std::cerr << "base err " << formatRegister(M1[0]) << std::endl;

				for ( uint64_t i = 0; i < 4; ++i )
				{
					I[2*i+0] = i * sizeof(LIBMAUS2_SIMD_WORD_TYPE)+0 + b*8;
					I[2*i+1] = i * sizeof(LIBMAUS2_SIMD_WORD_TYPE)+4 + b*8;
				}

				LIBMAUS2_SIMD_WORD_TYPE const vecI = LIBMAUS2_SIMD_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_SIMD_WORD_TYPE const *>(&(I[0])));
				// std::cerr << "vecI=" << formatRegister(vecI) << std::endl;

				LIBMAUS2_SIMD_WORD_TYPE vecg = LIBMAUS2_SIMD_GATHER(reinterpret_cast<int const *>(B),vecI,1);
				// std::cerr << formatRegisterCharNoSep(vecg) << std::endl;

				#if 0
				for ( uint64_t i = 0; i < 32; ++i )
				{
					uint64_t const stringid = batchlow + i;
					uint64_t const offset = a * AlignmentOneAgainstManyAVX2Context::gather_align_size + b;

					if ( stringid < MAo && offset < MA[i].second )
						assert (
							formatRegisterCharNoSep(vecg)[i]
							==
							MA[stringid].first[offset]
						);
				}
				#endif

				for ( uint64_t i = 0; i < qsize; ++i )
				{
					LIBMAUS2_SIMD_WORD_TYPE const qw = LIBMAUS2_SIMD_SET1(qa[i]);
					// std::cerr << "qw=" << formatRegister(qw) << std::endl;

					LIBMAUS2_SIMD_WORD_TYPE const top  = LIBMAUS2_SIMD_ADDS(M0[i+1],vone);
					LIBMAUS2_SIMD_WORD_TYPE const left = LIBMAUS2_SIMD_ADDS(M1[i],vone);
					LIBMAUS2_SIMD_WORD_TYPE const diag = LIBMAUS2_SIMD_ADDS(LIBMAUS2_SIMD_ANDNOT(LIBMAUS2_SIMD_CMPEQ(vecg,qw),vone),M0[i]);

					M1[i+1] = LIBMAUS2_SIMD_MIN(LIBMAUS2_SIMD_MIN(top,left),diag);
				}
				std::swap(M0,M1);

				#if 0
				for ( uint64_t i = 0; i < qsize+1; ++i )
				{
					std::cerr
						<< "*" << i << ":"
						<< ((i==lbaseerr)?"*":"")
						<< formatRegister(M0[i]) << std::endl;
				}
				#endif

				while ( vleno < vleni && Vlen[vleno].first == lbaseerr )
				{
					uint64_t const stringid = Vlen[vleno].second;

					__m128i const T =
						stringid >= 16
							?
							LIBMAUS2_SIMD_EXTRACT128 (M0[qsize], 1)
							:
							LIBMAUS2_SIMD_EXTRACT128 (M0[qsize], 0)
						;
					uint8_t e;

					switch ( Vlen[vleno].second & 0xf )
					{
						case 0: e = LIBMAUS2_SIMD_EXTRACT8(T,0); break;
						case 1: e = LIBMAUS2_SIMD_EXTRACT8(T,1); break;
						case 2: e = LIBMAUS2_SIMD_EXTRACT8(T,2); break;
						case 3: e = LIBMAUS2_SIMD_EXTRACT8(T,3); break;
						case 4: e = LIBMAUS2_SIMD_EXTRACT8(T,4); break;
						case 5: e = LIBMAUS2_SIMD_EXTRACT8(T,5); break;
						case 6: e = LIBMAUS2_SIMD_EXTRACT8(T,6); break;
						case 7: e = LIBMAUS2_SIMD_EXTRACT8(T,7); break;
						case 8: e = LIBMAUS2_SIMD_EXTRACT8(T,8); break;
						case 9: e = LIBMAUS2_SIMD_EXTRACT8(T,9); break;
						case 10: e = LIBMAUS2_SIMD_EXTRACT8(T,10); break;
						case 11: e = LIBMAUS2_SIMD_EXTRACT8(T,11); break;
						case 12: e = LIBMAUS2_SIMD_EXTRACT8(T,12); break;
						case 13: e = LIBMAUS2_SIMD_EXTRACT8(T,13); break;
						case 14: e = LIBMAUS2_SIMD_EXTRACT8(T,14); break;
						case 15: e = LIBMAUS2_SIMD_EXTRACT8(T,15); break;
					}

					// std::cerr << "end of " << Vlen[vleno].second << " error " << e << std::endl;
					E [ batchlow + Vlen[vleno].second ] = e;
					++vleno;
				}
			}
		}
	}

	#if defined(ALIGNMENT_ONE_AGAINST_MANY_AVX2_DEBUG)
	libmaus2::lcs::NP np;
	for ( uint64_t i = 0; i < MAo; ++i )
	{
		np.align(qa,qe-qa,MA[i].first,MA[i].second);

		bool const ok = E[i] == np.getTraceContainer().getAlignmentStatistics().getEditDistance();
		if ( !ok )
		{
			if ( ! ok )
			{
				libmaus2::aio::StreamLock::cerrlock.lock();
				std::cerr << "Failure for " << MAo << " entry " << i << " SIMD " << E[i] << " NP " << np.getTraceContainer().getAlignmentStatistics().getEditDistance() << std::endl;
				std::cerr << "std::string const q=\"" << std::string(qa,qe) << "\";" << std::endl;
				for ( uint64_t j = 0; j < MAo; ++j )
					std::cerr << "V.push_back(std::string(\"" << std::string(MA[j].first,MA[j].first+MA[j].second) << "\"));" << std::endl;
				libmaus2::aio::StreamLock::cerrlock.unlock();
			}
			assert ( ok );
		}
	}
	#endif

	#else
	for ( uint64_t i = 0; i < MAo; ++i )
	{
		np.align(qa,qe-qa,MA[i].first,MA[i].second);
		E [ i ] = np.getTraceContainer().getAlignmentStatistics().getEditDistance();
	}
	#endif
}
