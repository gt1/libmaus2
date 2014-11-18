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
#include <libmaus/digest/SHA2_256_sse4.hpp>
#include <libmaus/digest/sha256.h>
#include <emmintrin.h>
#include <immintrin.h>
#include <libmaus/rank/BSwapBase.hpp>

libmaus::digest::SHA2_256_sse4::SHA2_256_sse4() 
: block(2*(1ull<<libmaus::digest::SHA2_256_sse4::blockshift),false), 
  digestw(base_type::digestlength / sizeof(uint32_t),false), 
  digestinit(base_type::digestlength / sizeof(uint32_t),false),
  index(0), blockcnt(0)
{
	#if ! ( defined(LIBMAUS_USE_ASSEMBLY) && defined(LIBMAUS_HAVE_i386) && defined(LIBMAUS_HAVE_SHA2_ASSEMBLY) )
	libmaus::exception::LibMausException lme;
	lme.getStream() << "SHA2_256_sse4(): code has not been compiled into libmaus" << std::endl;
	lme.finish();
	throw lme;
	#endif
	
	if ( !libmaus::util::I386CacheLineSize::hasSSE41() )
	{
		libmaus::exception::LibMausException lme;
		lme.getStream() << "SHA2_256_sse4(): machine does not support SSE4" << std::endl;
		lme.finish();
		throw lme;
	}

	// initial state for sha256
	static uint32_t const digest[8] = 
	{
		0x6a09e667ul, 0xbb67ae85ul, 0x3c6ef372ul, 0xa54ff53aul, 
		0x510e527ful, 0x9b05688cul, 0x1f83d9abul, 0x5be0cd19ul
	};
	
	for ( unsigned int i = 0; i < 8; ++i )
		digestinit[i] = digest[i];
}
libmaus::digest::SHA2_256_sse4::~SHA2_256_sse4()
{

}

void libmaus::digest::SHA2_256_sse4::init()
{
	index = 0;
	blockcnt = 0;

	__m128i * po = reinterpret_cast<__m128i *>(&digestw[0]);
	__m128i * pi = reinterpret_cast<__m128i *>(&digestinit[0]);

	// copy 128 bit words (SSE2 instructions)
	__m128i ra = _mm_load_si128(pi++);
	_mm_store_si128(po++,ra);
	ra = _mm_load_si128(pi++);
	_mm_store_si128(po++,ra);	
}
void libmaus::digest::SHA2_256_sse4::update(uint8_t const * t, size_t l)
{
	// something already in the buffer?
	if ( index )
	{
		uint64_t const tocopy = std::min(l,static_cast<size_t>(1ull<<base_type::blockshift)-index);
		std::copy(t,t+tocopy,&block[index]);
		index += tocopy;
		t += tocopy;
		l -= tocopy;
		
		if ( index == (1ull<<base_type::blockshift) )
		{
			// block is complete, handle it
			sha256_sse4(&block[0],&digestw[0],1);
			
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
	sha256_sse4(t,&digestw[0],fullblocks);

	blockcnt += fullblocks;
	t += fullblocks << base_type::blockshift;
	l -= fullblocks << base_type::blockshift;
		
	std::copy(t,t+l,&block[index]);
	index += l;
}
void libmaus::digest::SHA2_256_sse4::digest(uint8_t * digest)
{
	uint64_t const numbytes = (1ull<<base_type::blockshift) * blockcnt + index;
	uint64_t const numbits = numbytes << 3;
		
	// write start of padding
	block[index++] = 0x80;
	
	uint64_t const blockspace = (1ull<<base_type::blockshift)-index;
		
	if ( blockspace >= 8 )
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
		
		uint64_t * p = (reinterpret_cast<uint64_t *>(&block[index]));
		uint64_t * const pe = p + (((1ull<<base_type::blockshift)-index)/8-1);
		
		// use 64 bit = 8 byte words
		while ( p != pe )
			*(p++) = 0;
			
		uint8_t * pp = reinterpret_cast<uint8_t *>(p);
		
		*p = libmaus::rank::BSwapBase::bswap8(numbits);

		sha256_sse4(&block[0],&digestw[0],1);
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
		uint64_t restwords = (((1ull<<(base_type::blockshift+1))-index) >> 3) - 1;
		
		// erase using 128 bit words
		__m128i * p128 = reinterpret_cast<__m128i *>(&block[index]);
		__m128i * p128e = p128 + (restwords>>1);
		__m128i z128 = _mm_setzero_si128();
		
		while ( p128 != p128e )
			_mm_store_si128(p128++,z128);
		
		// erase another 64 bit word
		uint64_t * p = (reinterpret_cast<uint64_t *>(p128)); *p++ = 0;

		*p = libmaus::rank::BSwapBase::bswap8(numbits);

		sha256_sse4(&block[0],&digestw[0],2);
	}
	
	uint32_t * digest32 = reinterpret_cast<uint32_t *>(&digest[0]);
	uint32_t * digest32e = digest32 + (base_type::digestlength / sizeof(uint32_t));
	uint32_t * digesti = &digestw[0];

	while ( digest32 != digest32e )
		*(digest32++) = libmaus::rank::BSwapBase::bswap4(*(digesti++));
}
void libmaus::digest::SHA2_256_sse4::copyFrom(SHA2_256_sse4 const & O)
{
	// blocksize is 64 = 4 * 16
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
	
	// digest length is 32 = 2 * 16
	__m128i const * digestin  = reinterpret_cast<__m128i const *>(&O.digestw[0]);
	__m128i       * digestout = reinterpret_cast<__m128i       *>(&  digestw[0]);

	reg = _mm_load_si128(digestin++);
	_mm_store_si128(digestout++,reg);	
	reg = _mm_load_si128(digestin++);
	_mm_store_si128(digestout++,reg);	

	blockcnt = O.blockcnt;
	index = O.index;
}
