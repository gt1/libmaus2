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

libmaus2::lcs::LIBMAUS2_SIMD_CLASS_NAME::LIBMAUS2_SIMD_CLASS_NAME() : diagmem(0), diagmemsize(0), aspace(0), aspacesize(0), bspace(0), bspacesize(0)
{

}
			
libmaus2::lcs::LIBMAUS2_SIMD_CLASS_NAME::~LIBMAUS2_SIMD_CLASS_NAME()
{
	free(diagmem);
	diagmem = 0;
	diagmemsize = 0;
	free(aspace);
	aspace = 0;
	aspacesize = 0;
	free(bspace);
	bspace = 0;
	bspacesize = 0;
}
			
void libmaus2::lcs::LIBMAUS2_SIMD_CLASS_NAME::allocateMemory(
	size_t const rsize,
	size_t const sizealign,
	LIBMAUS2_SIMD_ELEMENT_TYPE * & mem,
	size_t & memsize
)
{
	if ( mem )
	{
		free(mem);
		memsize = 0;
	}

	size_t const nsize = ((rsize + sizealign-1)/sizealign)*sizealign;

	if ( nsize > memsize )
	{
		if ( posix_memalign(reinterpret_cast<void **>(&mem),getpagesize(),nsize) != 0 )
		{
			std::cerr << "posix_memalign failed to allocate " << nsize << " bytes of memory." << std::endl;
			exit(1);
		}
		else
		{
			memsize = nsize;
		}
	}
}

void libmaus2::lcs::LIBMAUS2_SIMD_CLASS_NAME::align(
	uint8_t const * a,
	size_t const l_a,
	uint8_t const * b,
	size_t const l_b
)
{
	assert ( l_a );
	assert ( l_b );

	static unsigned int const bits_per_word = 8*sizeof(LIBMAUS2_SIMD_WORD_TYPE);
	static unsigned int const el_per_word = bits_per_word / (8*sizeof(LIBMAUS2_SIMD_ELEMENT_TYPE));

	LIBMAUS2_SIMD_INIT

	// number of diagonals we perform computations for
	size_t const compdiag = l_a + l_b;
	// number of preset diagonals
	size_t const prediag = 2;
	// number of diagonals allocated
	size_t const allocdiag = compdiag + prediag;
	
	// length of diagonal (0,1,l_a)
	size_t const diaglen = l_a+1;
	// length of allocated diagonal (in elements)
	size_t const preallocdiaglen = diaglen;
	size_t const wordsperdiag = (preallocdiaglen + el_per_word - 1) / el_per_word;
	size_t const allocdiaglen = wordsperdiag * el_per_word;
	size_t const allocdiaglenbytes = allocdiaglen * sizeof(LIBMAUS2_SIMD_ELEMENT_TYPE);
	size_t const allocmemsize = allocdiag * allocdiaglenbytes;
	
	allocateMemory(allocmemsize,sizeof(LIBMAUS2_SIMD_WORD_TYPE),diagmem,diagmemsize);
	allocateMemory(
		(l_a+1)*sizeof(LIBMAUS2_SIMD_ELEMENT_TYPE)+
		(sizeof(LIBMAUS2_SIMD_WORD_TYPE)/sizeof(LIBMAUS2_SIMD_ELEMENT_TYPE)-1)*sizeof(LIBMAUS2_SIMD_ELEMENT_TYPE),
		sizeof(LIBMAUS2_SIMD_WORD_TYPE),aspace,aspacesize
	);
	allocateMemory(
		(diaglen + compdiag)*sizeof(LIBMAUS2_SIMD_ELEMENT_TYPE)+
		(sizeof(LIBMAUS2_SIMD_WORD_TYPE)/sizeof(LIBMAUS2_SIMD_ELEMENT_TYPE)-1)*sizeof(LIBMAUS2_SIMD_ELEMENT_TYPE),
		sizeof(LIBMAUS2_SIMD_WORD_TYPE),bspace,bspacesize
	);
	
	LIBMAUS2_SIMD_ELEMENT_TYPE const asub = std::numeric_limits<LIBMAUS2_SIMD_ELEMENT_TYPE>::max()-1;
	LIBMAUS2_SIMD_ELEMENT_TYPE const bsub = std::numeric_limits<LIBMAUS2_SIMD_ELEMENT_TYPE>::max()-2;
	
	std::copy(a,a+l_a,aspace+1);
	aspace[0] = asub;
	std::fill(aspace+l_a+1,aspace+l_a+1+(sizeof(LIBMAUS2_SIMD_WORD_TYPE)/sizeof(LIBMAUS2_SIMD_ELEMENT_TYPE)-1),asub);
	
	LIBMAUS2_SIMD_ELEMENT_TYPE * bptr = bspace + diaglen + compdiag;
	std::fill(bptr,bptr+(sizeof(LIBMAUS2_SIMD_WORD_TYPE)/sizeof(LIBMAUS2_SIMD_ELEMENT_TYPE)-1),bsub);
	for ( uint64_t i = 0; i < l_a; ++i )
		*(--bptr) = bsub;
	for ( uint64_t i = 0; i < l_b; ++i )
		*(--bptr) = b[i];
	while ( bptr != bspace )
		*(--bptr) = bsub;
		
	#if 0
	std::cerr << "l_a=" << l_a << std::endl;
	std::cerr << "l_b=" << l_b << std::endl;
	std::cerr << "compdiag=" << compdiag << std::endl;
	std::cerr << "prediag=" << prediag << std::endl;
	std::cerr << "allocdiag=" << allocdiag << std::endl;
	std::cerr << "diaglen=" << diaglen << std::endl;
	std::cerr << "preallocdiaglen=" << preallocdiaglen << std::endl;
	std::cerr << "words per diag=" << wordsperdiag << std::endl;
	std::cerr << "allocdiaglen=" << allocdiaglen << std::endl;
	std::cerr << "allocdiaglenbytes=" << allocdiaglenbytes << std::endl;
	std::cerr << "allocmemsize=" << allocmemsize << std::endl;
	#endif
	
	LIBMAUS2_SIMD_WORD_TYPE * const pdiag = reinterpret_cast<LIBMAUS2_SIMD_WORD_TYPE *>(diagmem);
	
	LIBMAUS2_SIMD_WORD_TYPE const xmminit = LIBMAUS2_SIMD_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_SIMD_WORD_TYPE const *>(&LIBMAUS2_SIMD_ALL_FF[0]));
	LIBMAUS2_SIMD_WORD_TYPE * initp = pdiag;
	for ( size_t i = 0; i < prediag * wordsperdiag; ++i )
		LIBMAUS2_SIMD_STORE(initp++,xmminit);
	// set D(0,0) to 0
	diagmem [ allocdiaglen ] = 0;

	LIBMAUS2_SIMD_WORD_TYPE * in0 = pdiag;
	LIBMAUS2_SIMD_WORD_TYPE * in1 = in0 + wordsperdiag;
	LIBMAUS2_SIMD_WORD_TYPE * out = in1 + wordsperdiag;
	
	assert ( wordsperdiag > 0 );
	
	// every entry is 1
	LIBMAUS2_SIMD_WORD_TYPE const x1 = LIBMAUS2_SIMD_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_SIMD_WORD_TYPE const *>(&LIBMAUS2_SIMD_ALL_ONE));
	// first element is ff and all others zero
	LIBMAUS2_SIMD_WORD_TYPE const ff0 = LIBMAUS2_SIMD_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_SIMD_WORD_TYPE const *>(&LIBMAUS2_SIMD_FIRST_FF_REST_0[0]));
	
	if ( wordsperdiag == 1 )
	{	
		LIBMAUS2_SIMD_ELEMENT_TYPE const * bptr = bspace + l_a + l_b ;
		
		for ( size_t i = 0; i < compdiag; ++i, --bptr )
		{
			assert ( in0 == pdiag + i * wordsperdiag );
			assert ( in1 == pdiag + i * wordsperdiag + wordsperdiag );
			assert ( out == pdiag + i * wordsperdiag + wordsperdiag + wordsperdiag );
			
			// previous value for 2 diags back
			LIBMAUS2_SIMD_WORD_TYPE prev2 = ff0;
			// previous value for 1 diag back
			LIBMAUS2_SIMD_WORD_TYPE prev1 = ff0;

			LIBMAUS2_SIMD_WORD_TYPE const cv = LIBMAUS2_SIMD_ANDNOT(
				LIBMAUS2_SIMD_CMPEQ(
					LIBMAUS2_SIMD_LOAD_ALIGNED(reinterpret_cast<LIBMAUS2_SIMD_WORD_TYPE const *>(aspace)),
					LIBMAUS2_SIMD_LOAD_UNALIGNED(reinterpret_cast<LIBMAUS2_SIMD_WORD_TYPE const *>(bptr))
				),
				x1
			);
			
			// two diags back				
			LIBMAUS2_SIMD_WORD_TYPE const vm2p = LIBMAUS2_SIMD_LOAD_ALIGNED(in0++);
			LIBMAUS2_SIMD_WORD_TYPE const vm2s = LIBMAUS2_SIMD_ADD(LIBMAUS2_SIMD_OR(LIBMAUS2_SIMD_SHIFTRIGHT(vm2p),prev2),cv);
			// one diag back
			LIBMAUS2_SIMD_WORD_TYPE vm1 = LIBMAUS2_SIMD_LOAD_ALIGNED(in1++);
			// shuffled version of one diag back (last position is filled by 0xff)
			LIBMAUS2_SIMD_WORD_TYPE const vm1s = LIBMAUS2_SIMD_ADD(LIBMAUS2_SIMD_OR(LIBMAUS2_SIMD_SHIFTRIGHT(vm1),prev1),x1);
			// update vm1
			vm1  = LIBMAUS2_SIMD_ADD(vm1,x1);
									
			LIBMAUS2_SIMD_STORE(out++,LIBMAUS2_SIMD_MIN(LIBMAUS2_SIMD_MIN(vm1,vm1s),vm2s));
		}
	}
	else
	{
		assert ( wordsperdiag > 1 );
		
		for ( size_t i = 0; i < compdiag; ++i )
		{
			LIBMAUS2_SIMD_ELEMENT_TYPE const * bptr = bspace + l_a + l_b - i ;
			
			assert ( in0 == pdiag + i * wordsperdiag );
			assert ( in1 == pdiag + i * wordsperdiag + wordsperdiag );
			assert ( out == pdiag + i * wordsperdiag + wordsperdiag + wordsperdiag );
			
			// previous value for 2 diags back
			LIBMAUS2_SIMD_WORD_TYPE prev2 = ff0;
			// previous value for 1 diag back
			LIBMAUS2_SIMD_WORD_TYPE prev1 = ff0;
			
			LIBMAUS2_SIMD_WORD_TYPE const * atext = reinterpret_cast<LIBMAUS2_SIMD_WORD_TYPE const *>(aspace);
			LIBMAUS2_SIMD_WORD_TYPE const * btext = reinterpret_cast<LIBMAUS2_SIMD_WORD_TYPE const *>(bptr);

			// std::cerr << std::string(80,'-') << std::endl;

			for ( size_t j = 0; j < wordsperdiag; ++j )
			{
				LIBMAUS2_SIMD_WORD_TYPE const cv = LIBMAUS2_SIMD_ANDNOT(
					LIBMAUS2_SIMD_CMPEQ(
						LIBMAUS2_SIMD_LOAD_ALIGNED  (atext++),
						LIBMAUS2_SIMD_LOAD_UNALIGNED(btext++)
					),
					x1
				);
				
				LIBMAUS2_SIMD_WORD_TYPE const vm2 = LIBMAUS2_SIMD_LOAD_ALIGNED(in0++);
				// two diags back				
				LIBMAUS2_SIMD_WORD_TYPE const vm2s = LIBMAUS2_SIMD_ADD(LIBMAUS2_SIMD_OR(LIBMAUS2_SIMD_SHIFTRIGHT(vm2),prev2),cv);
				// one diag back
				LIBMAUS2_SIMD_WORD_TYPE const vm1 = LIBMAUS2_SIMD_LOAD_ALIGNED(in1++);
				// shuffled version of one diag back (last position is filled by 0xff)
				LIBMAUS2_SIMD_WORD_TYPE const vm1s = LIBMAUS2_SIMD_ADD(LIBMAUS2_SIMD_OR(LIBMAUS2_SIMD_SHIFTRIGHT(vm1),prev1),x1);
				// update vm1
				LIBMAUS2_SIMD_WORD_TYPE const vm1u  = LIBMAUS2_SIMD_ADD(vm1,x1);
										
				LIBMAUS2_SIMD_STORE(out++,LIBMAUS2_SIMD_MIN(LIBMAUS2_SIMD_MIN(vm1u,vm1s),vm2s));
				
				prev2 = LIBMAUS2_SIMD_SELECTLAST(vm2);
				prev1 = LIBMAUS2_SIMD_SELECTLAST(vm1);					
			}

			// std::cerr << "diag=" << formatRegister(LIBMAUS2_SIMD_LOAD_ALIGNED(out-1)) << std::endl;
		}
	}

	#if 0
	unsigned int const width = 5;
	std::cerr << ' ';
	std::cerr << std::setw(width) << ' ' << std::setw(0);
	for ( size_t i = 0; i < l_a; ++i )
		std::cerr << std::setw(width) << a[i] << std::setw(0);
	std::cerr << std::endl;
	
	for ( size_t y = 0; y <= l_b; ++y )
	{
		std::cerr.put(y>0 ? b[y-1] : ' ');
			
		for ( size_t x = 0; x <= l_a; ++x )
		{
			std::pair<int64_t,int64_t> P = squareToDiag(std::pair<int64_t,int64_t>(y,x));
			std::cerr << std::setw(width) << static_cast<int>(diagmem[(P.first+1)*allocdiaglen + P.second]) << std::setw(0);
		}
		std::cerr.put('\n');
	}
	#endif
	
	std::pair<int64_t,int64_t> const Ped(l_b,l_a);
	std::pair<int64_t,int64_t> const PDed(squareToDiag(Ped));
	int64_t const editdistance = diagmem[(PDed.first+1)*allocdiaglen + PDed.second];
	libmaus2::lcs::AlignmentTraceContainer::resize(std::max(l_a,l_b)+editdistance);
	
	int64_t py = l_b;
	int64_t px = l_a;

	LIBMAUS2_SIMD_ELEMENT_TYPE const * pcur = diagmem + (py+px+1) * allocdiaglen + px;
	LIBMAUS2_SIMD_ELEMENT_TYPE cval = (l_a || l_b) ? (*pcur) : 0;

	while ( pcur != diagmem + allocdiaglen )
	{
		LIBMAUS2_SIMD_ELEMENT_TYPE const * ptop   = pcur - allocdiaglen;
		LIBMAUS2_SIMD_ELEMENT_TYPE const * pleft  = ptop - 1;
		LIBMAUS2_SIMD_ELEMENT_TYPE const * pdiag  = pleft - allocdiaglen;
		
		LIBMAUS2_SIMD_ELEMENT_TYPE nval;
		LIBMAUS2_SIMD_ELEMENT_TYPE neq;
		
		if ( px && py && cval == (nval=*pdiag)+(neq = a[px-1] != b[py-1]) )
		{
			px -= 1;
			py -= 1;

			pcur -= (allocdiaglen<<1)+1;		
		
			if ( neq )
				*(--libmaus2::lcs::AlignmentTraceContainer::ta) = STEP_MISMATCH;
			else
				*(--libmaus2::lcs::AlignmentTraceContainer::ta) = STEP_MATCH;

			cval = nval;
		}
		else if ( py && (nval=*ptop)+1 == cval )
		{
			*(--libmaus2::lcs::AlignmentTraceContainer::ta) = STEP_INS;
			
			py -= 1;	
			
			pcur -= allocdiaglen;
			
			cval = nval;
		}
		else if ( px && (nval=*pleft)+1 == cval )
		{
			*(--libmaus2::lcs::AlignmentTraceContainer::ta) = STEP_DEL;
			
			px -= 1;
			
			pcur -= (allocdiaglen+1);

			cval = nval;
		}
	}	
}
