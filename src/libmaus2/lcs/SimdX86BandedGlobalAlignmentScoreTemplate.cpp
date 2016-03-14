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
#include <iomanip>
#include <libmaus2/autoarray/AutoArray.hpp>

#if 0
static std::ostream & printRegister(std::ostream & out, LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const reg)
{
	LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE sp[sizeof(reg)/sizeof(LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE)] __attribute__((aligned(sizeof(reg))));
	LIBMAUS2_LCS_SIMD_BANDED_STORE(reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE *>(&sp[0]),reg);
	for ( uint64_t i = 0; i < sizeof(reg)/sizeof(LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE); ++i )
		out << static_cast<LIBMAUS2_LCS_SIMD_BANDED_SIGNED_ELEMENT_TYPE>(sp[i]) <<
			((i+1<sizeof(reg)/sizeof(LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE))?",":"");
	return out;
}

static std::ostream & printRegisterChar(std::ostream & out, LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const reg)
{
	LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE sp[sizeof(reg)/sizeof(LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE)] __attribute__((aligned(sizeof(reg))));
	LIBMAUS2_LCS_SIMD_BANDED_STORE(reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE *>(&sp[0]),reg);
	for ( uint64_t i = 0; i < sizeof(reg)/sizeof(LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE); ++i )
		out << std::setw(2) << (sp[i]) << std::setw(0) <<
			((i+1<(sizeof(reg)/sizeof(LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE)))?",":"");
	return out;
}
#endif

#if 0
static std::string formatRegister(LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const reg)
{
	std::ostringstream ostr;
	printRegister(ostr,reg);
	return ostr.str();
}

static std::string formatRegisterChar(LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const reg)
{
	std::ostringstream ostr;
	printRegisterChar(ostr,reg);
	return ostr.str();
}
#endif

libmaus2::lcs::LIBMAUS2_LCS_SIMD_BANDED_CLASS_NAME::LIBMAUS2_LCS_SIMD_BANDED_CLASS_NAME() : diagmem(0), diagmemsize(0), negdiagmem(0), negdiagmemsize(0), difdiagmem(0), difdiagmemsize(0), 
	textmem(0), textmemsize(0), querymem(0), querymemsize(0)
{

}

static void alignedFree(void * mem)
{
	if ( mem )
	{
		#if defined(LIBMAUS2_HAVE_POSIX_MEMALIGN)
		::free(mem);
		#else
		::libmaus2::autoarray::AlignedAllocation<unsigned char,libmaus2::autoarray::alloc_type_memalign_pagesize>::freeAligned(reinterpret_cast<unsigned char *>(mem));
		#endif
	}
}

libmaus2::lcs::LIBMAUS2_LCS_SIMD_BANDED_CLASS_NAME::~LIBMAUS2_LCS_SIMD_BANDED_CLASS_NAME()
{
	if ( negdiagmem )
	{
		alignedFree(negdiagmem);
		negdiagmem = 0;
		negdiagmemsize = 0;
	}
	if ( difdiagmem )
	{
		alignedFree(difdiagmem);
		difdiagmem = 0;
		difdiagmemsize = 0;
	}
	if ( diagmem )
	{
		alignedFree(diagmem);
		diagmem = 0;
		diagmemsize = 0;
	}
	if ( textmem )
	{
		alignedFree(textmem);
		textmem = 0;
		textmemsize = 0;
	}
	if ( querymem )
	{
		alignedFree(querymem);
		querymem = 0;
		querymemsize = 0;
	}
}

void libmaus2::lcs::LIBMAUS2_LCS_SIMD_BANDED_CLASS_NAME::allocateMemory(
	size_t const rsize,
	size_t const sizealign,
	LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE * & mem,
	size_t & memsize
)
{
	if ( mem )
	{
		alignedFree(mem);
		mem = 0;
		memsize = 0;
	}

	size_t const nsize = ((rsize + sizealign-1)/sizealign)*sizealign;

	if ( nsize > memsize )
	{
		#if defined(LIBMAUS2_HAVE_POSIX_MEMALIGN)
		if ( posix_memalign(reinterpret_cast<void **>(&mem),getpagesize(),nsize) != 0 )
		{
			libmaus2::exception::LibMausException lme;
			lme.getStream() << "posix_memalign failed to allocate " << nsize << " bytes of memory." << std::endl;
			lme.finish();
			throw lme;
		}
		else
		{
			memsize = nsize;
		}
		#else
		if ( ! (mem = reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE *>(libmaus2::autoarray::AlignedAllocation<unsigned char,libmaus2::autoarray::alloc_type_memalign_pagesize>::alignedAllocate(nsize,getpagesize()))) )
		{
			libmaus2::exception::LibMausException lme;
			lme.getStream() << "libmaus2::autoarray::AlignedAllocation<unsigned char,libmaus2::autoarray::alloc_type_memalign_pagesize>::alignedAllocate failed to allocate " << nsize << " bytes of memory." << std::endl;
			lme.finish();
			throw lme;
		}
		else
		{
			memsize = nsize;
		}
		#endif
	}	
}

std::pair<uint64_t,uint64_t> libmaus2::lcs::LIBMAUS2_LCS_SIMD_BANDED_CLASS_NAME::align(uint8_t const * a, size_t const l_a, uint8_t const * b, size_t const l_b, size_t const d)
{
	// elements per SIMD word
	static size_t const elements_per_word = sizeof(LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE)/sizeof(LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE);
	// length of odd anti diagonal in elements
	size_t const odddiaglen = (d+1)<<1;
	// length of even anti diagonal in elements
	size_t const evendiaglen = (d<<1)+1;
	// length of allocated anti diagonal in elements
	size_t const allocdiaglen = std::max(odddiaglen,evendiaglen);
	// SIMD words per anti diagonal
	size_t const words_necessary = (allocdiaglen + elements_per_word - 1)/elements_per_word;
	// bytes per anti diagonal
	size_t const bytes_per_diag = words_necessary * sizeof(LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE);

	// number of diagonals computed
	size_t const compdiag = 2*std::min(l_a,l_b) + d;
	// number of pre set diagonals
	size_t const prediag = 2;
	// number of allocated diagonals
	size_t const allocdiag = prediag + compdiag;

	size_t const textelements = compdiag/2 + words_necessary * elements_per_word;  // (d+1)+(compdiag/2)+(d+1);
	size_t const textelementsalloc = ((textelements + elements_per_word - 1) / elements_per_word) * elements_per_word;
	size_t const textelementsallocbytes = textelementsalloc*sizeof(LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE);

	size_t const queryelements = textelements;
	size_t const queryelementsalloc = ((queryelements + elements_per_word - 1) / elements_per_word) * elements_per_word;
	size_t const queryelementsallocbytes = queryelementsalloc*sizeof(LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE);

	allocateMemory(allocdiag * bytes_per_diag, sizeof(LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE), diagmem, diagmemsize);
	allocateMemory(allocdiag * bytes_per_diag, sizeof(LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE), negdiagmem, negdiagmemsize);
	allocateMemory(allocdiag * bytes_per_diag, sizeof(LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE), difdiagmem, difdiagmemsize);
	allocateMemory(textelementsallocbytes, sizeof(LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE), textmem, textmemsize);
	allocateMemory(queryelementsallocbytes, sizeof(LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE), querymem, querymemsize);

	#if 0
	std::fill(diagmem,diagmem + words_necessary * elements_per_word * allocdiag, 0x00);
	std::fill(textmem,textmem + textelementsalloc, 0x00 );
	std::fill(querymem,querymem + queryelementsalloc, 0x00 );
	#endif

	LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE const asub = std::numeric_limits<LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE>::max()-1;
	LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE const bsub = std::numeric_limits<LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE>::max()-2;

	LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE *       textmemc = textmem;
	LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE * const textmeme = textmem + textelements;
	for ( uint64_t i = 0; i < (d+1); ++i )
		*(textmemc++) = asub;
	for ( uint64_t i = 0; i < l_a && textmemc != textmeme; ++i )
		*(textmemc++) = a[i];
	while ( textmemc != textmeme )
		*(textmemc++) = asub;

	LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE *bcopy = querymem + queryelements;
	size_t const bpad = words_necessary * elements_per_word - (d+1);
	for ( size_t i = 0; i < bpad; ++i )
		*(--bcopy) = bsub;
	size_t maxqcopy = queryelements - bpad;
	size_t qcopy = std::min(l_b,maxqcopy);
	for ( size_t i = 0; i < qcopy; ++i )
		*(--bcopy) = b[i];
	while ( bcopy != querymem )
		*(--bcopy) = bsub;

	// initialise two diagonals
	std::fill(diagmem   ,diagmem    + 2 * words_necessary * elements_per_word, 0); // std::numeric_limits<LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE>::max());
	// set (0,0) to 1
	diagmem [ words_necessary * elements_per_word + d ] = 1;

	// initialise two diagonals
	std::fill(negdiagmem,negdiagmem + 2 * words_necessary * elements_per_word, std::numeric_limits<LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE>::max() >> 1);
	// set (0,0) to 0
	negdiagmem [ words_necessary * elements_per_word + d ] = 0;

	// initialise two diagonals
	std::fill(difdiagmem,difdiagmem + 2 * words_necessary * elements_per_word, std::numeric_limits<LIBMAUS2_LCS_SIMD_BANDED_SIGNED_ELEMENT_TYPE>::min());
	// set (0,0) to 0
	difdiagmem [ words_necessary * elements_per_word + d ] = 0;

	// two diagonals back
	LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE * pos_diag_1 = reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE *>(diagmem);
	// one diagonal back
	LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE * pos_diag_0 = pos_diag_1 + words_necessary;
	// next diagonal to be computed
	LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE * pos_diag_n = pos_diag_0 + words_necessary;

	LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const all0 = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const *>(&LIBMAUS2_LCS_SIMD_BANDED_ALL_0[0]));
	LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const all1 = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const *>(&LIBMAUS2_LCS_SIMD_BANDED_ALL_1[0]));

	// two diagonals back
	LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE * edit_diag_1 = reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE *>(negdiagmem);
	// one diagonal back
	LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE * edit_diag_0 = edit_diag_1 + words_necessary;
	// next diagonal to be computed
	LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE * edit_diag_n = edit_diag_0 + words_necessary;

	// two diagonals back
	LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE * dif_diag_1 = reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE *>(difdiagmem);
	// one diagonal back
	LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE * dif_diag_0 = dif_diag_1 + words_necessary;
	// next diagonal to be computed
	LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE * dif_diag_n = dif_diag_0 + words_necessary;

	LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const edit_ff0 = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const *>(&LIBMAUS2_LCS_SIMD_BANDED_FIRST_FF_REST_0[0]));
	LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const edit_zff = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const *>(&LIBMAUS2_LCS_SIMD_BANDED_LAST_FF_REST_0[0]));

	LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE all_signed_max = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const *>(&LIBMAUS2_LCS_SIMD_BANDED_ALL_SIGNED_MAX[0]));

	// LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE v_all_one = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const *>(&LIBMAUS2_LCS_SIMD_BANDED_ALL_ONE[0]));

	LIBMAUS2_LCS_SIMD_BANDED_INIT
	

	for ( int64_t di = 1; di <= static_cast<int64_t>(compdiag); ++di )
	{
		assert ( pos_diag_1 == reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE *>(diagmem) + (di-1)*words_necessary );
		#if 0
		assert ( neg_diag_1 == reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE *>(negdiagmem) + (di-1)*words_necessary );
		#endif

		// text
		LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE const * t = textmem + (di>>1);
		LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE const * q = querymem + queryelements - bpad - (d) - ((di+1)>>1);
		// text words
		LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const * wt = reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const *>(t);
		LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const * wq = reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const *>(q);

		assert ( t + words_necessary * elements_per_word <= textmem + textelementsalloc );
		assert ( q + words_necessary * elements_per_word <= querymem + queryelementsalloc );

		// uneven row
		if ( (di & 1) == 1 )
		{
			// everything fits in a single word
			if ( words_necessary == 1 )
			{
				/**
				 * score
				 **/
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE pos_prev_0 = all0;
				
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE v_pos_diag_1 = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(pos_diag_1++);
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE v_pos_diag_0 = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(pos_diag_0++);

				// shift to right and insert 0 at front
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE vpos_diag_0_shift = LIBMAUS2_LCS_SIMD_BANDED_OR(LIBMAUS2_LCS_SIMD_BANDED_SHIFTRIGHT(v_pos_diag_0),pos_prev_0);

				// load text
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_t = LIBMAUS2_LCS_SIMD_BANDED_LOAD_UNALIGNED(wt++);

				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_q = LIBMAUS2_LCS_SIMD_BANDED_LOAD_UNALIGNED(wq++);

				// compare (1 iff symbols are equal)
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_eq = LIBMAUS2_LCS_SIMD_BANDED_AND(LIBMAUS2_LCS_SIMD_BANDED_CMPEQ(v_t,v_q),all1);

				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_max =
					LIBMAUS2_LCS_SIMD_BANDED_MAX(
						LIBMAUS2_LCS_SIMD_BANDED_MAX(v_pos_diag_0,vpos_diag_0_shift),
						LIBMAUS2_LCS_SIMD_BANDED_ADD(v_pos_diag_1,v_eq)
					);

				LIBMAUS2_LCS_SIMD_BANDED_STORE(pos_diag_n++,v_max);

				/**
				 * edit distance
				 **/
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE edit_prev_0 = edit_ff0;

				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE v_edit_diag_1 = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(edit_diag_1++);
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE v_edit_diag_0 = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(edit_diag_0++);

				// shift to right and insert ff at front
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE vedit_diag_0_shift = LIBMAUS2_LCS_SIMD_BANDED_OR(LIBMAUS2_LCS_SIMD_BANDED_SHIFTRIGHT(v_edit_diag_0),edit_prev_0);

				// compare (1 iff symbols are not equal)
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_neq = LIBMAUS2_LCS_SIMD_BANDED_ANDNOT(LIBMAUS2_LCS_SIMD_BANDED_CMPEQ(v_t,v_q),all1);

				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_edit_min =
					LIBMAUS2_LCS_SIMD_BANDED_MIN(
						LIBMAUS2_LCS_SIMD_BANDED_MIN(
							LIBMAUS2_LCS_SIMD_BANDED_ADD(v_edit_diag_0,all1),
							LIBMAUS2_LCS_SIMD_BANDED_ADD(vedit_diag_0_shift,all1)
						),
						LIBMAUS2_LCS_SIMD_BANDED_ADD(v_edit_diag_1,v_neq)
					);

				LIBMAUS2_LCS_SIMD_BANDED_STORE(edit_diag_n++,v_edit_min);

				LIBMAUS2_LCS_SIMD_BANDED_STORE(dif_diag_n++,LIBMAUS2_LCS_SIMD_BANDED_SUBS(LIBMAUS2_LCS_SIMD_BANDED_SUBS(v_max,all1),LIBMAUS2_LCS_SIMD_BANDED_AND(v_edit_min,all_signed_max)));				
			}
			// more than one word necessary
			else if ( words_necessary > 1 )
			{
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE pos_prev = all0;
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE edit_prev = edit_ff0;

				for ( size_t z = 0; z < words_necessary; ++z )
				{
					// two diags back
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_pos_diag_1 = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(pos_diag_1++);
					// load text
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_t = LIBMAUS2_LCS_SIMD_BANDED_LOAD_UNALIGNED(wt++);
					// load query
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_q = LIBMAUS2_LCS_SIMD_BANDED_LOAD_UNALIGNED(wq++);
					// compare (1 iff symbols are equal)
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_eq = LIBMAUS2_LCS_SIMD_BANDED_AND(LIBMAUS2_LCS_SIMD_BANDED_CMPEQ(v_t,v_q),all1);

					// next one back
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_pos_diag_0 = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(pos_diag_0++);

					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const pos_left = v_pos_diag_0;
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const pos_top =
						LIBMAUS2_LCS_SIMD_BANDED_OR(
							LIBMAUS2_LCS_SIMD_BANDED_SHIFTRIGHT(v_pos_diag_0),
							pos_prev
						);

					// compute maximum over three
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_max =
						LIBMAUS2_LCS_SIMD_BANDED_MAX(
							LIBMAUS2_LCS_SIMD_BANDED_MAX(pos_left,pos_top),
							LIBMAUS2_LCS_SIMD_BANDED_ADD(v_pos_diag_1,v_eq)
						);

					// store
					LIBMAUS2_LCS_SIMD_BANDED_STORE(pos_diag_n++,v_max);

					pos_prev = LIBMAUS2_LCS_SIMD_BANDED_SELECTLAST(v_pos_diag_0);
					
					/**
					 * edit distance
					 **/

					// two diags back
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_edit_diag_1 = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(edit_diag_1++);
					// compare (1 iff symbols are not equal)
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_neq = LIBMAUS2_LCS_SIMD_BANDED_ANDNOT(LIBMAUS2_LCS_SIMD_BANDED_CMPEQ(v_t,v_q),all1);

					// next one back
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_edit_diag_0 = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(edit_diag_0++);

					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const edit_left = v_edit_diag_0;
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const edit_top =
						LIBMAUS2_LCS_SIMD_BANDED_OR(
							LIBMAUS2_LCS_SIMD_BANDED_SHIFTRIGHT(v_edit_diag_0),
							edit_prev
						);

					// compute minimum over three
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_edit_min =
						LIBMAUS2_LCS_SIMD_BANDED_MIN(
							LIBMAUS2_LCS_SIMD_BANDED_MIN(
								LIBMAUS2_LCS_SIMD_BANDED_ADD(edit_left,all1),
								LIBMAUS2_LCS_SIMD_BANDED_ADD(edit_top ,all1)
							),
							LIBMAUS2_LCS_SIMD_BANDED_ADD(v_edit_diag_1,v_neq)
						);

					// store
					LIBMAUS2_LCS_SIMD_BANDED_STORE(edit_diag_n++,v_edit_min);

					edit_prev = LIBMAUS2_LCS_SIMD_BANDED_SELECTLAST(v_edit_diag_0);

					LIBMAUS2_LCS_SIMD_BANDED_STORE(dif_diag_n++,LIBMAUS2_LCS_SIMD_BANDED_SUBS(LIBMAUS2_LCS_SIMD_BANDED_SUBS(v_max,all1),LIBMAUS2_LCS_SIMD_BANDED_AND(v_edit_min,all_signed_max)));
				}
			}
		}
		// even row
		else
		{
			// everything fits in a single machine word
			if ( words_necessary == 1 )
			{
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE v_pos_diag_1 = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(pos_diag_1++);
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE v_pos_diag_0 = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(pos_diag_0++);
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE v_pos_diag_0_shift = LIBMAUS2_LCS_SIMD_BANDED_OR(LIBMAUS2_LCS_SIMD_BANDED_SHIFTLEFT(v_pos_diag_0),all0);

				// load text
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_t = LIBMAUS2_LCS_SIMD_BANDED_LOAD_UNALIGNED(wt++);
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_q = LIBMAUS2_LCS_SIMD_BANDED_LOAD_UNALIGNED(wq++);

				// compare (1 iff symbols are equal)
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_eq = LIBMAUS2_LCS_SIMD_BANDED_AND(LIBMAUS2_LCS_SIMD_BANDED_CMPEQ(v_t,v_q),all1);

				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_max =
					LIBMAUS2_LCS_SIMD_BANDED_MAX(
						LIBMAUS2_LCS_SIMD_BANDED_MAX(v_pos_diag_0,v_pos_diag_0_shift),
						LIBMAUS2_LCS_SIMD_BANDED_ADD(v_pos_diag_1,v_eq)
					);

				LIBMAUS2_LCS_SIMD_BANDED_STORE(pos_diag_n++,v_max);

				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE v_edit_diag_1 = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(edit_diag_1++);
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE v_edit_diag_0 = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(edit_diag_0++);
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE v_edit_diag_0_shift = LIBMAUS2_LCS_SIMD_BANDED_OR(LIBMAUS2_LCS_SIMD_BANDED_SHIFTLEFT(v_edit_diag_0),edit_zff);

				// compare (1 iff symbols are not equal)
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_neq = LIBMAUS2_LCS_SIMD_BANDED_ANDNOT(LIBMAUS2_LCS_SIMD_BANDED_CMPEQ(v_t,v_q),all1);

				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_edit_min =
					LIBMAUS2_LCS_SIMD_BANDED_MIN(
						LIBMAUS2_LCS_SIMD_BANDED_MIN(
							LIBMAUS2_LCS_SIMD_BANDED_ADD(v_edit_diag_0,all1),
							LIBMAUS2_LCS_SIMD_BANDED_ADD(v_edit_diag_0_shift,all1)
						),
						LIBMAUS2_LCS_SIMD_BANDED_ADD(v_edit_diag_1,v_neq)
					);

				LIBMAUS2_LCS_SIMD_BANDED_STORE(edit_diag_n++,v_edit_min);

				LIBMAUS2_LCS_SIMD_BANDED_STORE(dif_diag_n++,LIBMAUS2_LCS_SIMD_BANDED_SUBS(LIBMAUS2_LCS_SIMD_BANDED_SUBS(v_max,all1),LIBMAUS2_LCS_SIMD_BANDED_AND(v_edit_min,all_signed_max)));
			}
			// more than one word necessary
			else if ( words_necessary > 1 )
			{
				// one diag back
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE v_pos_diag_0_p = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(pos_diag_0++);
				// one diag back
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE v_edit_diag_0_p = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(edit_diag_0++);

				for ( size_t z = 1; z < words_necessary; ++z )
				{
					// two diags back
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE v_pos_diag_1 = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(pos_diag_1++);
					// load text
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_t = LIBMAUS2_LCS_SIMD_BANDED_LOAD_UNALIGNED(wt++);
					// load query
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_q = LIBMAUS2_LCS_SIMD_BANDED_LOAD_UNALIGNED(wq++);
					// compare (1 iff symbols are equal)
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_eq = LIBMAUS2_LCS_SIMD_BANDED_AND(LIBMAUS2_LCS_SIMD_BANDED_CMPEQ(v_t,v_q),all1);

					// next one back
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_pos_diag_0_n = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(pos_diag_0++);

					// pos_left
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const pos_left = v_pos_diag_0_p;
					// pos_top
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const pos_top =
						LIBMAUS2_LCS_SIMD_BANDED_OR(
							LIBMAUS2_LCS_SIMD_BANDED_SHIFTLEFT(v_pos_diag_0_p),
							LIBMAUS2_LCS_SIMD_BANDED_FIRST_TO_BACK(v_pos_diag_0_n)
						);


					// compute maximum over three
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_max =
						LIBMAUS2_LCS_SIMD_BANDED_MAX(
							LIBMAUS2_LCS_SIMD_BANDED_MAX(pos_left,pos_top),
							LIBMAUS2_LCS_SIMD_BANDED_ADD(v_pos_diag_1,v_eq)
						);

					// store
					LIBMAUS2_LCS_SIMD_BANDED_STORE(pos_diag_n++,v_max);

					v_pos_diag_0_p = v_pos_diag_0_n;

					// two diags back
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE v_edit_diag_1 = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(edit_diag_1++);
					// compare (1 iff symbols are not equal)
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_neq = LIBMAUS2_LCS_SIMD_BANDED_ANDNOT(LIBMAUS2_LCS_SIMD_BANDED_CMPEQ(v_t,v_q),all1);

					// next one back
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_edit_diag_0_n = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(edit_diag_0++);

					// edit_left
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const edit_left = v_edit_diag_0_p;
					// edit_top
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const edit_top =
						LIBMAUS2_LCS_SIMD_BANDED_OR(
							LIBMAUS2_LCS_SIMD_BANDED_SHIFTLEFT(v_edit_diag_0_p),
							LIBMAUS2_LCS_SIMD_BANDED_FIRST_TO_BACK(v_edit_diag_0_n)
						);

					// compute minimum over three
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_edit_min =
						LIBMAUS2_LCS_SIMD_BANDED_MIN(
							LIBMAUS2_LCS_SIMD_BANDED_MIN(
								LIBMAUS2_LCS_SIMD_BANDED_ADD(edit_left,all1),
								LIBMAUS2_LCS_SIMD_BANDED_ADD(edit_top ,all1)
							),
							LIBMAUS2_LCS_SIMD_BANDED_ADD(v_edit_diag_1,v_neq)
						);

					// store
					LIBMAUS2_LCS_SIMD_BANDED_STORE(edit_diag_n++,v_edit_min);
					
					LIBMAUS2_LCS_SIMD_BANDED_STORE(dif_diag_n++,LIBMAUS2_LCS_SIMD_BANDED_SUBS(LIBMAUS2_LCS_SIMD_BANDED_SUBS(v_max,all1),LIBMAUS2_LCS_SIMD_BANDED_AND(v_edit_min,all_signed_max)));

					v_edit_diag_0_p = v_edit_diag_0_n;
				}

				// two diags back
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE v_pos_diag_1 = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(pos_diag_1++);
				// load text
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_t = LIBMAUS2_LCS_SIMD_BANDED_LOAD_UNALIGNED(wt++);
				// load query
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_q = LIBMAUS2_LCS_SIMD_BANDED_LOAD_UNALIGNED(wq++);
				// compare (1 iff symbols are equal)
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_eq = LIBMAUS2_LCS_SIMD_BANDED_CMPEQ(v_t,v_q);

				// pos_left
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const pos_left = v_pos_diag_0_p;
				// pos_top
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const pos_top =
					LIBMAUS2_LCS_SIMD_BANDED_OR(
						LIBMAUS2_LCS_SIMD_BANDED_SHIFTLEFT(v_pos_diag_0_p),
						all0
					);

				// compute maximum over three
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_max =
					LIBMAUS2_LCS_SIMD_BANDED_MAX(
						LIBMAUS2_LCS_SIMD_BANDED_MAX(pos_left,pos_top),
						LIBMAUS2_LCS_SIMD_BANDED_ADD(v_pos_diag_1,v_eq)
					);

				// store
				LIBMAUS2_LCS_SIMD_BANDED_STORE(pos_diag_n++,v_max);

				// two diags back
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE v_edit_diag_1 = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(edit_diag_1++);
				// compare (1 iff symbols are not equal)
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_neq = LIBMAUS2_LCS_SIMD_BANDED_ANDNOT(LIBMAUS2_LCS_SIMD_BANDED_CMPEQ(v_t,v_q),all1);

				// edit_left
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const edit_left = v_edit_diag_0_p;
				// edit_top
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const edit_top =
					LIBMAUS2_LCS_SIMD_BANDED_OR(
						LIBMAUS2_LCS_SIMD_BANDED_SHIFTLEFT(v_edit_diag_0_p),
						edit_zff
					);

				// compute minimum over three
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_edit_min =
					LIBMAUS2_LCS_SIMD_BANDED_MIN(
						LIBMAUS2_LCS_SIMD_BANDED_MIN(
							LIBMAUS2_LCS_SIMD_BANDED_ADD(edit_left,all1),
							LIBMAUS2_LCS_SIMD_BANDED_ADD(edit_top ,all1)
						),
						LIBMAUS2_LCS_SIMD_BANDED_ADD(v_edit_diag_1,v_neq)
					);

				// store
				LIBMAUS2_LCS_SIMD_BANDED_STORE(edit_diag_n++,v_edit_min);

				//
				LIBMAUS2_LCS_SIMD_BANDED_STORE(
					dif_diag_n++,
					LIBMAUS2_LCS_SIMD_BANDED_SUBS(
						LIBMAUS2_LCS_SIMD_BANDED_SUBS(v_max,all1),
						LIBMAUS2_LCS_SIMD_BANDED_AND(v_edit_min,all_signed_max)
					)
				);
			}
		}
	}

	#if 0
	for ( int64_t i = 0; i <= static_cast<int64_t>(l_b); ++i )
	{
		for ( int64_t j = 0; j <= static_cast<int64_t>(l_a); ++j )
		{
			// convert square coordinates to diagonal based coordinates
			std::pair<int64_t,int64_t> const P = squareToDiag(std::pair<int64_t,int64_t>(i,j),d);

			//std::cerr << i << "," << j << " -> " << P.first << "," << P.second << std::endl;
			
			bool const antidiagvalid = (P.first >= 0) && (P.first <= static_cast<int64_t>(compdiag));
			bool const diagvalid = 
				(P.second >= 0) &&
				(
					((P.first & 1)==0)
					?
					(P.second < static_cast<int64_t>(2*d+1))
					:
					(P.second < static_cast<int64_t>(2*(d+1)))
				);
			bool const valid = antidiagvalid && diagvalid;

			if ( valid )
				std::cerr << std::setw(4) << static_cast<int>(diagmem [ words_necessary * elements_per_word * (P.first+1) + P.second ]) << std::setw(0);
			else
				std::cerr << std::setw(4) << '*' << std::setw(0);				
		}
		std::cerr << std::endl;
	}
	#endif

	#if 0
	for ( int64_t i = 0; i <= static_cast<int64_t>(l_b); ++i )
	{
		for ( int64_t j = 0; j <= static_cast<int64_t>(l_a); ++j )
		{
			// convert square coordinates to diagonal based coordinates
			std::pair<int64_t,int64_t> const P = squareToDiag(std::pair<int64_t,int64_t>(i,j),d);

			//std::cerr << i << "," << j << " -> " << P.first << "," << P.second << std::endl;
			
			bool const antidiagvalid = (P.first >= 0) && (P.first <= static_cast<int64_t>(compdiag));
			bool const diagvalid = 
				(P.second >= 0) &&
				(
					((P.first & 1)==0)
					?
					(P.second < static_cast<int64_t>(2*d+1))
					:
					(P.second < static_cast<int64_t>(2*(d+1)))
				);
			bool const valid = antidiagvalid && diagvalid;

			if ( valid )
				std::cerr << std::setw(4) << static_cast<int>(negdiagmem [ words_necessary * elements_per_word * (P.first+1) + P.second ]) << std::setw(0);
			else
				std::cerr << std::setw(4) << '*' << std::setw(0);				
		}
		std::cerr << std::endl;
	}
	#endif

	#if 0
	for ( int64_t i = 0; i <= static_cast<int64_t>(l_b); ++i )
	{
		for ( int64_t j = 0; j <= static_cast<int64_t>(l_a); ++j )
		{
			// convert square coordinates to diagonal based coordinates
			std::pair<int64_t,int64_t> const P = squareToDiag(std::pair<int64_t,int64_t>(i,j),d);

			//std::cerr << i << "," << j << " -> " << P.first << "," << P.second << std::endl;
			
			bool const antidiagvalid = (P.first >= 0) && (P.first <= static_cast<int64_t>(compdiag));
			bool const diagvalid = 
				(P.second >= 0) &&
				(
					((P.first & 1)==0)
					?
					(P.second < static_cast<int64_t>(2*d+1))
					:
					(P.second < static_cast<int64_t>(2*(d+1)))
				);
			bool const valid = antidiagvalid && diagvalid;

			if ( valid )
				std::cerr << std::setw(7) << 
					static_cast<LIBMAUS2_LCS_SIMD_BANDED_SIGNED_ELEMENT_TYPE>(difdiagmem [ words_necessary * elements_per_word * (P.first+1) + P.second ]) 
				<< std::setw(0);
			else
				std::cerr << std::setw(7) << '*' << std::setw(0);				
		}
		std::cerr << std::endl;
	}
	#endif

	// words_necessary
	LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE * max_score_it = reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE *>(difdiagmem) + words_necessary;
	LIBMAUS2_LCS_SIMD_BANDED_SIGNED_ELEMENT_TYPE maxscore = std::numeric_limits<LIBMAUS2_LCS_SIMD_BANDED_SIGNED_ELEMENT_TYPE>::min();
	uint64_t maxdi = 0;
	for ( uint64_t di = 0; di < compdiag; ++di )
	{
		LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE w = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(max_score_it++);
		for ( uint64_t i = 1; i < words_necessary; ++i )
			w = LIBMAUS2_LCS_SIMD_BANDED_MAXS(w,LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(max_score_it++));
		
		// map top to bottom (reduce to low 128 bits)
		w = LIBMAUS2_LCS_SIMD_BANDED_MAXS(w,_mm256_permute2f128_si256(w,w,0x1));
		w = LIBMAUS2_LCS_SIMD_BANDED_MAXS(w,_mm256_shuffle_epi32(w,0xe));
		w = LIBMAUS2_LCS_SIMD_BANDED_MAXS(w,_mm256_shuffle_epi32(w,0x1));
		w = LIBMAUS2_LCS_SIMD_BANDED_MAXS(w,_mm256_shufflelo_epi16(w,1));

		LIBMAUS2_LCS_SIMD_BANDED_SIGNED_ELEMENT_TYPE sp[sizeof(LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE)/sizeof(LIBMAUS2_LCS_SIMD_BANDED_SIGNED_ELEMENT_TYPE)] __attribute__((aligned(sizeof(LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE))));
		LIBMAUS2_LCS_SIMD_BANDED_STORE(reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE *>(&sp[0]),w);
				
		if ( sp[0] > maxscore )
		{
			maxscore = sp[0];
			maxdi = di;
		}		
	}
	
	uint64_t maxoff = 0;
	for ( uint64_t i = 0; i < words_necessary * elements_per_word; ++i )
		if ( difdiagmem [ words_necessary * elements_per_word * (maxdi+1) + i ] == maxscore )
			maxoff = i;

	// std::cerr << "maxscore " << maxscore << " di=" << maxdi << " maxoff=" << maxoff << std::endl;

	// std::pair<int64_t,int64_t> cur = squareToDiag(std::pair<int64_t,int64_t>(l_b,l_a),d);
	std::pair<int64_t,int64_t> cur(maxdi,maxoff);
	libmaus2::lcs::AlignmentTraceContainer::reset();

	std::pair<int64_t,int64_t> curX = diagToSquare(cur,d);
	std::pair<int64_t,int64_t> curY = squareToDiag(curX,d);
	
	assert ( curY.first == cur.first );
	assert ( curY.second == cur.second );
	assert ( curY == cur );
	
	// std::pair<int64_t,int64_t> cur = squareToDiag(std::pair<int64_t,int64_t>(l_b,l_a),d);
	// std::cerr << "using " << cur.first << "," << cur.second << std::endl;

	// trace back
	if (
		// (l_a,l_b) is on a valid diagonal
		(cur.first >= 0 && cur.first <= static_cast<int64_t>(compdiag)) &&
		// (l_a,l_b) is on a valid offset on that diagonal
		(
			(((cur.first & 1) == 0) && cur.second >= 0 && cur.second < static_cast<int64_t>(2*d+1))
			||
			(((cur.first & 1) == 1) && cur.second >= 0 && cur.second < static_cast<int64_t>(2*d+2))
		)
	)
	{
		// length of a diagonal in elements
		size_t const diaglen = words_necessary * elements_per_word;
		// pointer to current element
		LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE * p = diagmem + (cur.first + 1) * diaglen + cur.second;
		// position on a and p
		// size_t pa = l_a, pb = l_b;
		size_t pa = curX.second, pb = curX.first;

		// score for full alignment
		//int64_t const score = *p;
		
		// check whether we have sufficient space
		if ( libmaus2::lcs::AlignmentTraceContainer::capacity() < 2*std::min(l_a,l_b) )
		{
			libmaus2::lcs::AlignmentTraceContainer::resize(2*std::min(l_a,l_b));
			libmaus2::lcs::AlignmentTraceContainer::reset();
		}

		// while we have not reached (0,0)
		while ( cur.first != 0 || cur.second != static_cast<int64_t>(d) )
		{
			bool eq;

			// if we can go back on both a and b and the best entry is on the diagonal
			if ( pa && pb && *p == *(p-2*diaglen) + (eq=(a[pa-1] == b[pb-1])) )
			{
				pa -= 1;
				pb -= 1;
				p -= 2*diaglen;
				cur.first -= 2;

				if ( eq )
					*(--libmaus2::lcs::AlignmentTraceContainer::ta) = STEP_MATCH;
				else
					*(--libmaus2::lcs::AlignmentTraceContainer::ta) = STEP_MISMATCH;
			}
			// even diagonal
			else if ( (cur.first & 1) == 0 )
			{
				// left
				if ( pa && *p == *(p-diaglen) /* + 1 */ )
				{
					pa -= 1;
					p -= diaglen;
					cur.first -= 1;
					*(--libmaus2::lcs::AlignmentTraceContainer::ta) = STEP_DEL;
				}
				// top
				else
				{
					assert ( pb && *p == *(p-diaglen+1) /* + 1 */ );
					pb -= 1;
					p -= (diaglen-1);
					cur.first -= 1;
					cur.second += 1;
					*(--libmaus2::lcs::AlignmentTraceContainer::ta) = STEP_INS;
				}
			}
			// odd diagonal
			else
			{
				// left
				if ( pa && (cur.second-1) >= 0 && *p == *(p-diaglen-1) /* +1 */ )
				{
					pa -= 1;
					p -= (diaglen+1);
					cur.first -= 1;
					cur.second -= 1;
					*(--libmaus2::lcs::AlignmentTraceContainer::ta) = STEP_DEL;
				}
				// top
				else
				{
					assert ( pb && (cur.second < static_cast<int64_t>(2*d+1)) && *p == *(p-diaglen) /* +1 */ );
					pb -= 1;
					p -= diaglen;
					cur.first -= 1;
					*(--libmaus2::lcs::AlignmentTraceContainer::ta) = STEP_INS;
				}
			}
		}
	}
	
	return 
		std::pair<uint64_t,uint64_t>(curX.second, curX.first);

}

libmaus2::lcs::AlignmentTraceContainer const & libmaus2::lcs::LIBMAUS2_LCS_SIMD_BANDED_CLASS_NAME::getTraceContainer() const
{
	return *this;
}
