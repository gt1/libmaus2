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
		out << static_cast<int>(sp[i]) << 
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

libmaus2::lcs::LIBMAUS2_LCS_SIMD_BANDED_CLASS_NAME::LIBMAUS2_LCS_SIMD_BANDED_CLASS_NAME() : diagmem(0), diagmemsize(0), textmem(0), textmemsize(0), querymem(0), querymemsize(0)
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


void libmaus2::lcs::LIBMAUS2_LCS_SIMD_BANDED_CLASS_NAME::align(uint8_t const * a, size_t const l_a, uint8_t const * b, size_t const l_b, size_t const d)
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
				
	std::fill(diagmem,diagmem + 2 * words_necessary * elements_per_word, std::numeric_limits<LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE>::max());
	diagmem [ words_necessary * elements_per_word + d ] = 0;

	LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE * diag_1 = reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE *>(diagmem);
	LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE * diag_0 = diag_1 + words_necessary;
	LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE * diag_n = diag_0 + words_necessary;

	LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const ff0 = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const *>(&LIBMAUS2_LCS_SIMD_BANDED_FIRST_FF_REST_0[0]));
	LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const zff = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const *>(&LIBMAUS2_LCS_SIMD_BANDED_LAST_FF_REST_0[0]));

	#if 0
	LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE ffback[sizeof(LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE)/sizeof(LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE)] __attribute__((aligned(sizeof(LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE)))) = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	ffback[2*d+1] = std::numeric_limits<LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE>::max();
	LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE v_ffback = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const *>(&ffback[0]));
	#endif
	
	LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE v_all_one = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const *>(&LIBMAUS2_LCS_SIMD_BANDED_ALL_ONE[0]));

	LIBMAUS2_LCS_SIMD_BANDED_INIT
	
	for ( int64_t di = 1; di <= static_cast<int64_t>(compdiag); ++di )
	{
		assert ( diag_1 == reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE *>(diagmem) + (di-1)*words_necessary );
		// std::cerr << "di=" << di << " allocdiag=" << allocdiag << std::endl;
	
		LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE const * t = textmem + (di>>1);
		// LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE const * q = querymem + queryelements - (2*d+1) - ((di+1)>>1); 
		LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE const * q = querymem + queryelements - bpad - (d) - ((di+1)>>1); 
		LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const * wt = reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const *>(t);
		LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const * wq = reinterpret_cast<LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const *>(q);
		
		assert ( t + words_necessary * elements_per_word <= textmem + textelementsalloc );
		assert ( q + words_necessary * elements_per_word <= querymem + queryelementsalloc );
		#if 0
		if ( !( t + words_necessary * elements_per_word <= textmem + textelementsalloc ) )
		{
			std::cerr << "di=" << di << std::endl;
			std::cerr << "t=" << t << std::endl;
			assert ( t + words_necessary * elements_per_word <= textmem + textelementsalloc );
		}
		#endif
		#if 0
		if ( q + words_necessary * elements_per_word > querymem + queryelementsalloc )
		{		
			std::cerr << "di=" << di << std::endl;
			std::cerr << "q=" << q << std::endl;
			assert ( q + words_necessary * elements_per_word <= querymem + queryelementsalloc );
		}
		#endif
		
		#if 0
		{
			LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE v_t = LIBMAUS2_LCS_SIMD_BANDED_LOAD_UNALIGNED(wt);
			LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE v_q = LIBMAUS2_LCS_SIMD_BANDED_LOAD_UNALIGNED(wq);
			std::cerr << "di=" << di << " " << formatRegisterChar(v_t) << " " << formatRegisterChar(v_q) << std::endl;
		}
		#endif
			
		// uneven row
		if ( (di & 1) == 1 )
		{
			// everything fits in a single word
			if ( words_necessary == 1 )
			{
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE prev_0 = ff0;
								
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE v_diag_1 = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(diag_1++);
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE v_diag_0 = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(diag_0++);
	
				#if 0
				// insert FF at back
				v_diag_0 = LIBMAUS2_LCS_SIMD_BANDED_OR(v_diag_0,v_ffback);
				#endif
				
				// shift to right and insert ff at front
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE vdiag_0_shift = LIBMAUS2_LCS_SIMD_BANDED_OR(LIBMAUS2_LCS_SIMD_BANDED_SHIFTRIGHT(v_diag_0),prev_0);
				
				// load text
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_t = LIBMAUS2_LCS_SIMD_BANDED_LOAD_UNALIGNED(wt++);
				
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_q = LIBMAUS2_LCS_SIMD_BANDED_LOAD_UNALIGNED(wq++);

				// compare (1 iff symbols are not equal)
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_eq = LIBMAUS2_LCS_SIMD_BANDED_ANDNOT(LIBMAUS2_LCS_SIMD_BANDED_CMPEQ(v_t,v_q),v_all_one);
				
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_min =
					LIBMAUS2_LCS_SIMD_BANDED_MIN(
						LIBMAUS2_LCS_SIMD_BANDED_MIN(
							LIBMAUS2_LCS_SIMD_BANDED_ADD(v_diag_0,v_all_one),
							LIBMAUS2_LCS_SIMD_BANDED_ADD(vdiag_0_shift,v_all_one)
						),
						LIBMAUS2_LCS_SIMD_BANDED_ADD(v_diag_1,v_eq)
					);
					
				LIBMAUS2_LCS_SIMD_BANDED_STORE(diag_n++,v_min);
			}
			// more than one word necessary
			else if ( words_necessary > 1 )
			{
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE prev = ff0;	
				
				for ( size_t z = 0; z < words_necessary; ++z )
				{
					// two diags back			
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_diag_1 = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(diag_1++);
					// load text
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_t = LIBMAUS2_LCS_SIMD_BANDED_LOAD_UNALIGNED(wt++);
					// load query				
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_q = LIBMAUS2_LCS_SIMD_BANDED_LOAD_UNALIGNED(wq++);
					// compare (1 iff symbols are not equal)
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_eq = LIBMAUS2_LCS_SIMD_BANDED_ANDNOT(LIBMAUS2_LCS_SIMD_BANDED_CMPEQ(v_t,v_q),v_all_one);

					// next one back
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_diag_0 = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(diag_0++);
					
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const left = v_diag_0;
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const top = 
						LIBMAUS2_LCS_SIMD_BANDED_OR(
							LIBMAUS2_LCS_SIMD_BANDED_SHIFTRIGHT(v_diag_0),
							prev
						);

					// compute minimum over three
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_min =
						LIBMAUS2_LCS_SIMD_BANDED_MIN(
							LIBMAUS2_LCS_SIMD_BANDED_MIN(
								LIBMAUS2_LCS_SIMD_BANDED_ADD(left,v_all_one),
								LIBMAUS2_LCS_SIMD_BANDED_ADD(top ,v_all_one)
							),
							LIBMAUS2_LCS_SIMD_BANDED_ADD(v_diag_1,v_eq)
						);
				
					// store		
					LIBMAUS2_LCS_SIMD_BANDED_STORE(diag_n++,v_min);
					
					prev = LIBMAUS2_LCS_SIMD_BANDED_SELECTLAST(v_diag_0);
				}
			}
		}
		// even row
		else
		{
			// everything fits in a single machine word
			if ( words_necessary == 1 )
			{
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE v_diag_1 = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(diag_1++);
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE v_diag_0 = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(diag_0++);
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE v_diag_0_shift = LIBMAUS2_LCS_SIMD_BANDED_OR(LIBMAUS2_LCS_SIMD_BANDED_SHIFTLEFT(v_diag_0),zff);

				// load text
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_t = LIBMAUS2_LCS_SIMD_BANDED_LOAD_UNALIGNED(wt++);				
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_q = LIBMAUS2_LCS_SIMD_BANDED_LOAD_UNALIGNED(wq++);

				// compare (1 iff symbols are not equal)
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_eq = LIBMAUS2_LCS_SIMD_BANDED_ANDNOT(LIBMAUS2_LCS_SIMD_BANDED_CMPEQ(v_t,v_q),v_all_one);
				
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_min =
					LIBMAUS2_LCS_SIMD_BANDED_MIN(
						LIBMAUS2_LCS_SIMD_BANDED_MIN(
							LIBMAUS2_LCS_SIMD_BANDED_ADD(v_diag_0,v_all_one),
							LIBMAUS2_LCS_SIMD_BANDED_ADD(v_diag_0_shift,v_all_one)
						),
						LIBMAUS2_LCS_SIMD_BANDED_ADD(v_diag_1,v_eq)
					);
					
				LIBMAUS2_LCS_SIMD_BANDED_STORE(diag_n++,v_min);
			}
			// more than one word necessary
			else if ( words_necessary > 1 )
			{
				// one diag back
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE v_diag_0_p = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(diag_0++);
			
				for ( size_t z = 1; z < words_necessary; ++z )
				{
					// two diags back			
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE v_diag_1 = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(diag_1++);
					// load text
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_t = LIBMAUS2_LCS_SIMD_BANDED_LOAD_UNALIGNED(wt++);
					// load query				
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_q = LIBMAUS2_LCS_SIMD_BANDED_LOAD_UNALIGNED(wq++);
					// compare (1 iff symbols are not equal)
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_eq = LIBMAUS2_LCS_SIMD_BANDED_ANDNOT(LIBMAUS2_LCS_SIMD_BANDED_CMPEQ(v_t,v_q),v_all_one);

					// next one back
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_diag_0_n = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(diag_0++);
					
					// left
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const left = v_diag_0_p;
					// top
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const top =
						LIBMAUS2_LCS_SIMD_BANDED_OR(
							LIBMAUS2_LCS_SIMD_BANDED_SHIFTLEFT(v_diag_0_p),
							LIBMAUS2_LCS_SIMD_BANDED_FIRST_TO_BACK(v_diag_0_n)
						);

					// compute minimum over three
					LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_min =
						LIBMAUS2_LCS_SIMD_BANDED_MIN(
							LIBMAUS2_LCS_SIMD_BANDED_MIN(
								LIBMAUS2_LCS_SIMD_BANDED_ADD(left,v_all_one),
								LIBMAUS2_LCS_SIMD_BANDED_ADD(top ,v_all_one)
							),
							LIBMAUS2_LCS_SIMD_BANDED_ADD(v_diag_1,v_eq)
						);
				
					// store		
					LIBMAUS2_LCS_SIMD_BANDED_STORE(diag_n++,v_min);
					
					v_diag_0_p = v_diag_0_n;
				}

				// two diags back			
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE v_diag_1 = LIBMAUS2_LCS_SIMD_BANDED_LOAD_ALIGNED(diag_1++);
				// load text
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_t = LIBMAUS2_LCS_SIMD_BANDED_LOAD_UNALIGNED(wt++);
				// load query				
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_q = LIBMAUS2_LCS_SIMD_BANDED_LOAD_UNALIGNED(wq++);
				// compare (1 iff symbols are not equal)
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_eq = LIBMAUS2_LCS_SIMD_BANDED_ANDNOT(LIBMAUS2_LCS_SIMD_BANDED_CMPEQ(v_t,v_q),v_all_one);

				// left
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const left = v_diag_0_p;
				// top
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const top =
					LIBMAUS2_LCS_SIMD_BANDED_OR(
						LIBMAUS2_LCS_SIMD_BANDED_SHIFTLEFT(v_diag_0_p),
						zff
					);

				// compute minimum over three
				LIBMAUS2_LCS_SIMD_BANDED_WORD_TYPE const v_min =
					LIBMAUS2_LCS_SIMD_BANDED_MIN(
						LIBMAUS2_LCS_SIMD_BANDED_MIN(
							LIBMAUS2_LCS_SIMD_BANDED_ADD(left,v_all_one),
							LIBMAUS2_LCS_SIMD_BANDED_ADD(top ,v_all_one)
						),
						LIBMAUS2_LCS_SIMD_BANDED_ADD(v_diag_1,v_eq)
					);
				
				// store		
				LIBMAUS2_LCS_SIMD_BANDED_STORE(diag_n++,v_min);
			}		
		}
	}

	#if 0
	for ( int64_t i = 0; i <= l_b; ++i )
	{
		for ( int64_t j = 0; j <= l_a; ++j )
		{
			std::pair<int64_t,int64_t> const P = squareToDiag(std::pair<int64_t,int64_t>(i,j),d);					
				
			// std::cerr << i << "," << j << " -> " << P.first << "," << P.second << std::endl;
			
			#if 1
			if ( P.first >= 0 && P.first <= compdiag && P.first % 2 == 0 && P.second >= 0 && P.second < 2*d+1 ) 
			{
				std::cerr << std::setw(4) << static_cast<int>(diagmem [ words_necessary * elements_per_word * (P.first+1) + P.second ]) << std::setw(0);
			}
			else if ( P.first >= 0 && P.first <= compdiag && P.first % 2 == 1 && P.second >= 0 && P.second < 2*(d+1) )
			{
				std::cerr << std::setw(4) << static_cast<int>(diagmem [ words_necessary * elements_per_word * (P.first+1) + P.second ]) << std::setw(0);						
			}
			else
			{
				std::cerr << std::setw(4) << '*' << std::setw(0);
			}
			#endif
		}
		std::cerr << std::endl;
	}
	#endif
	
	std::pair<int64_t,int64_t> cur = squareToDiag(std::pair<int64_t,int64_t>(l_b,l_a),d);
	libmaus2::lcs::AlignmentTraceContainer::reset();
	
	// trace back
	if ( 
		(cur.first >= 0 && cur.first <= static_cast<int64_t>(compdiag)) && 
		(
			(((cur.first & 1) == 0) && cur.second >= 0 && cur.second < static_cast<int64_t>(2*d+1))
			||
			(((cur.first & 1) == 1) && cur.second >= 0 && cur.second < static_cast<int64_t>(2*d+2))
		)
	)
	{
		size_t const diaglen = words_necessary * elements_per_word;
		LIBMAUS2_LCS_SIMD_BANDED_ELEMENT_TYPE * p = diagmem + (cur.first + 1) * diaglen + cur.second;
		size_t pa = l_a, pb = l_b;
		
		int64_t const editdistance = *p;
		if ( libmaus2::lcs::AlignmentTraceContainer::capacity() < std::min(l_a,l_b) + editdistance )
		{
			libmaus2::lcs::AlignmentTraceContainer::resize(std::min(l_a,l_b) + editdistance);
			libmaus2::lcs::AlignmentTraceContainer::reset();
		}
		
		while ( cur.first != 0 || cur.second != static_cast<int64_t>(d) )
		{
			bool neq;
			
			if ( pa && pb && *p == *(p-2*diaglen) + (neq=(a[pa-1] != b[pb-1])) )
			{
				pa -= 1;
				pb -= 1;
				p -= 2*diaglen;
				cur.first -= 2;
				
				if ( neq )
					*(--libmaus2::lcs::AlignmentTraceContainer::ta) = STEP_MISMATCH;
				else
					*(--libmaus2::lcs::AlignmentTraceContainer::ta) = STEP_MATCH;
			}
			// even
			else if ( (cur.first & 1) == 0 )
			{
				// left
				if ( pa && *p == *(p-diaglen) + 1 )
				{
					pa -= 1;
					p -= diaglen;
					cur.first -= 1;
					*(--libmaus2::lcs::AlignmentTraceContainer::ta) = STEP_DEL;
				}
				// top
				else
				{
					assert ( pb && *p == *(p-diaglen+1) + 1 );
					pb -= 1;
					p -= (diaglen-1);
					cur.first -= 1;
					cur.second += 1;
					*(--libmaus2::lcs::AlignmentTraceContainer::ta) = STEP_INS;
				}
			}
			// odd
			else
			{
				// left
				if ( pa && (cur.second-1) >= 0 && *p == *(p-diaglen-1)+1 )
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
					assert ( pb && (cur.second < static_cast<int64_t>(2*d+1)) && *p == *(p-diaglen)+1 );
					pb -= 1;
					p -= diaglen;
					cur.first -= 1;
					*(--libmaus2::lcs::AlignmentTraceContainer::ta) = STEP_INS;
				}
			}
		}
	}
}

libmaus2::lcs::AlignmentTraceContainer const & libmaus2::lcs::LIBMAUS2_LCS_SIMD_BANDED_CLASS_NAME::getTraceContainer() const
{
	return *this;
}
