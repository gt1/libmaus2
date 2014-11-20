/*
    libmaus
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#include <libmaus/digest/SHA2_512_sse4.hpp>
#include <libmaus/digest/sha512.h>
#include <libmaus/rank/BSwapBase.hpp>
#include <algorithm>

#if defined(LIBMAUS_HAVE_x86_64)
#include <emmintrin.h>
#endif

libmaus::digest::SHA2_512_sse4::SHA2_512_sse4() 
: block(2*(1ull<<libmaus::digest::SHA2_512_sse4::blockshift),false), 
  digestw(base_type::digestlength / sizeof(uint64_t),false), 
  digestinit(base_type::digestlength / sizeof(uint64_t),false),
  index(0), blockcnt(0)
{
	#if ! ( defined(LIBMAUS_USE_ASSEMBLY) && defined(LIBMAUS_HAVE_x86_64) && defined(LIBMAUS_HAVE_i386) && defined(LIBMAUS_HAVE_SHA2_ASSEMBLY) )
	libmaus::exception::LibMausException lme;
	lme.getStream() << "SHA2_512_sse4(): code has not been compiled into libmaus" << std::endl;
	lme.finish();
	throw lme;
	#endif
	
	#if defined(LIBMAUS_USE_ASSEMBLY) && defined(LIBMAUS_HAVE_i386)
	if ( !libmaus::util::I386CacheLineSize::hasSSE41() )
	#else
	if ( true )
	#endif
	{
		libmaus::exception::LibMausException lme;
		lme.getStream() << "SHA2_512_sse4(): machine does not support SSE4" << std::endl;
		lme.finish();
		throw lme;
	}

	// initial state for sha512
	static uint64_t const digest[8] = 
	{
                0x6A09E667F3BCC908ull,0xBB67AE8584CAA73Bull,
                0x3C6EF372FE94F82Bull,0xA54FF53A5F1D36F1ull,
                0x510E527FADE682D1ull,0x9B05688C2B3E6C1Full,
		0x1F83D9ABFB41BD6Bull,0x5BE0CD19137E2179ull
	};
	
	for ( unsigned int i = 0; i < 8; ++i )
		digestinit[i] = digest[i];
}
libmaus::digest::SHA2_512_sse4::~SHA2_512_sse4()
{

}

void libmaus::digest::SHA2_512_sse4::init()
{
	#if defined(LIBMAUS_HAVE_x86_64)
	index = 0;
	blockcnt = 0;

	__m128i * po = reinterpret_cast<__m128i *>(&digestw[0]);
	__m128i * pi = reinterpret_cast<__m128i *>(&digestinit[0]);

	// copy 8*64=512 bits using 128 bit words (4 copies, SSE2 instructions)
	__m128i ra = _mm_load_si128(pi++);
	_mm_store_si128(po++,ra);
	ra = _mm_load_si128(pi++);
	_mm_store_si128(po++,ra);	
	ra = _mm_load_si128(pi++);
	_mm_store_si128(po++,ra);	
	ra = _mm_load_si128(pi++);
	_mm_store_si128(po++,ra);
	#endif	
}
void libmaus::digest::SHA2_512_sse4::update(
	#if defined(LIBMAUS_HAVE_x86_64)
	uint8_t const * t, 
	size_t l
	#else
	uint8_t const *, 
	size_t	
	#endif
)
{
	#if defined(LIBMAUS_HAVE_x86_64)
	// something already in the buffer?
	if ( index )
	{
		uint64_t const tocopy = std::min(static_cast<uint64_t>(l),static_cast<uint64_t>(static_cast<size_t>(1ull<<base_type::blockshift)-index));
		std::copy(t,t+tocopy,&block[index]);
		index += tocopy;
		t += tocopy;
		l -= tocopy;
		
		if ( index == (1ull<<base_type::blockshift) )
		{
			// block is complete, handle it
			sha512_sse4(&block[0],&digestw[0],1);
			
			//
			blockcnt += 1;
			index = 0;
		}
		else
		{
			// done, block is not complete and there is no more data
			return;
		}
	}

	uint64_t const fullblocks = (l >> base_type::blockshift);
		
	// handle fullblocks blocks without copying them
	sha512_sse4(t,&digestw[0],fullblocks);

	blockcnt += fullblocks;
	t += fullblocks << base_type::blockshift;
	l -= fullblocks << base_type::blockshift;
		
	std::copy(t,t+l,&block[index]);
	index += l;
	#endif
}
void libmaus::digest::SHA2_512_sse4::digest(
	#if defined(LIBMAUS_HAVE_x86_64)
	uint8_t * digest
	#else
	uint8_t *	
	#endif
)
{
	#if defined(LIBMAUS_HAVE_x86_64)
	// total number of bytes
	uint64_t const numbytes = (1ull<<base_type::blockshift) * blockcnt + index;
	// total number of bits
	uint64_t const numbits = numbytes << 3;
		
	// write start of padding
	block[index++] = 0x80;
	
	// space left in current block after putting 0x80 marker
	uint64_t const blockspace = (1ull<<base_type::blockshift)-index;
	
	// enough room to store the length of the message	
	if ( blockspace >= numlen )
	{
		// not multiple of 2?
		if ( index & 1 )
		{
			block[index] = 0;
			index += 1;
		}
		// not multiple of 4?
		if ( index & 2 )
		{
			*(reinterpret_cast<uint16_t *>(&block[index])) = 0;
			index += 2;
		}
		// not multiple of 8?
		if ( index & 4 )
		{		
			*(reinterpret_cast<uint32_t *>(&block[index])) = 0;
			index += 4;
		}
		// not multiple of 16?
		if ( index & 8 )
		{		
			*(reinterpret_cast<uint64_t *>(&block[index])) = 0;
			index += 8;
		}

		__m128i * p128 = reinterpret_cast<__m128i *>(&block[index]);
		__m128i * pe128 = p128 + ((((1ull<<base_type::blockshift)-index) >> 4)-1);
		__m128i z128 = _mm_setzero_si128();

		// use 64 bit = 8 byte words
		while ( p128 != pe128 )
			_mm_store_si128(p128++,z128);
			
		uint64_t * p = reinterpret_cast<uint64_t *>(p128);
		
		// fix this, implement for larger numbers
		*(p++) = 0;
		* p = libmaus::rank::BSwapBase::bswap8(numbits);

		sha512_sse4(&block[0],&digestw[0],1);
	}
	else
	{
		// not multiple of 2?
		if ( index & 1 )
		{
			block[index] = 0;
			index += 1;
		}
		// not multiple of 4?
		if ( index & 2 )
		{
			*(reinterpret_cast<uint16_t *>(&block[index])) = 0;
			index += 2;
		}
		// not multiple of 8?
		if ( index & 4 )
		{		
			*(reinterpret_cast<uint32_t *>(&block[index])) = 0;
			index += 4;
		}
		// not multiple of 16?
		if ( index & 8 )
		{		
			*(reinterpret_cast<uint64_t *>(&block[index])) = 0;
			index += 8;
		}

		// rest of words in first block + all but one word in second block
		uint64_t restwords = (((1ull << (base_type::blockshift+1))-index)>>4) - (numlen>>4);
				
		// erase using 128 bit words
		__m128i * p128 = reinterpret_cast<__m128i *>(&block[index]);
		__m128i * p128e = p128 + (restwords);
		__m128i z128 = _mm_setzero_si128();
		
		while ( p128 != p128e )
			_mm_store_si128(p128++,z128);
		
		// erase another 64 bit word, fixme: handle true 128 bit numbers
		uint64_t * p = (reinterpret_cast<uint64_t *>(p128)); *p++ = 0;
		*p = libmaus::rank::BSwapBase::bswap8(numbits);

		sha512_sse4(&block[0],&digestw[0],2);
	}
	
	uint64_t * digest64 = reinterpret_cast<uint64_t *>(&digest[0]);
	uint64_t * digest64e = digest64 + (base_type::digestlength / sizeof(uint64_t));
	uint64_t * digesti = &digestw[0];

	while ( digest64 != digest64e )
		*(digest64++) = libmaus::rank::BSwapBase::bswap8(*(digesti++));
	#endif
}
void libmaus::digest::SHA2_512_sse4::copyFrom(
	#if defined(LIBMAUS_HAVE_x86_64)
	SHA2_512_sse4 const & O
	#else
	SHA2_512_sse4 const &
	#endif
)
{
	#if defined(LIBMAUS_HAVE_x86_64)
	// blocksize is 128 bytes
	__m128i reg;
	__m128i const * blockin  = reinterpret_cast<__m128i const *>(&O.block[0]);
	__m128i       * blockout = reinterpret_cast<__m128i       *>(&  block[0]);
	
	reg = _mm_load_si128(blockin++);
	_mm_store_si128(blockout++,reg);	
	reg = _mm_load_si128(blockin++);
	_mm_store_si128(blockout++,reg);	
	reg = _mm_load_si128(blockin++);
	_mm_store_si128(blockout++,reg);	
	reg = _mm_load_si128(blockin++);
	_mm_store_si128(blockout++,reg);	
	reg = _mm_load_si128(blockin++);
	_mm_store_si128(blockout++,reg);	
	reg = _mm_load_si128(blockin++);
	_mm_store_si128(blockout++,reg);	
	reg = _mm_load_si128(blockin++);
	_mm_store_si128(blockout++,reg);	
	reg = _mm_load_si128(blockin++);
	_mm_store_si128(blockout++,reg);	
	
	// digest length is 64
	__m128i const * digestin  = reinterpret_cast<__m128i const *>(&O.digestw[0]);
	__m128i       * digestout = reinterpret_cast<__m128i       *>(&  digestw[0]);

	reg = _mm_load_si128(digestin++);
	_mm_store_si128(digestout++,reg);	
	reg = _mm_load_si128(digestin++);
	_mm_store_si128(digestout++,reg);	
	reg = _mm_load_si128(digestin++);
	_mm_store_si128(digestout++,reg);	
	reg = _mm_load_si128(digestin++);
	_mm_store_si128(digestout++,reg);	

	blockcnt = O.blockcnt;
	index = O.index;
	#endif
}
