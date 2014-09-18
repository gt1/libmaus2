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
#include <libmaus/types/types.hpp>
#include <libmaus/rank/popcnt.hpp>
#include <libmaus/random/Random.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/timing/RealTimeClock.hpp>

/**
 * Count the number of c bytes in block of 64 bytes starting from p.
 * This function is programmed to avoid control branching.
 *
 * @param p block of 64 bytes
 * @param c character to be counted
 * @return number of c bytes in 64 byte block p
 **/
static uint64_t count64(uint8_t const * p, uint8_t const c ='\n')
{
	uint64_t w = 0;
	for ( unsigned int j = 0; j < 8; ++j )
	{
		// "SIMD" register for 8 bytes
		#if 1
		uint64_t v;
		// fill simd register, bytes becomes 0 if it equals c
		v = static_cast<uint8_t>(*(p++) - c);
		for ( unsigned int i = 1; i < 8; ++i )
		{
			v <<= 8;
			v |= static_cast<uint8_t>(*(p++) - c);
		}

		#else
		
		uint64_t v0 = static_cast<uint64_t>(static_cast<uint8_t>(p[0]-c));
		uint64_t v1 = static_cast<uint64_t>(static_cast<uint8_t>(p[1]-c));
		uint64_t v2 = static_cast<uint64_t>(static_cast<uint8_t>(p[2]-c));
		uint64_t v3 = static_cast<uint64_t>(static_cast<uint8_t>(p[3]-c));
		uint64_t v4 = static_cast<uint64_t>(static_cast<uint8_t>(p[4]-c));
		uint64_t v5 = static_cast<uint64_t>(static_cast<uint8_t>(p[5]-c));
		uint64_t v6 = static_cast<uint64_t>(static_cast<uint8_t>(p[6]-c));
		uint64_t v7 = static_cast<uint64_t>(static_cast<uint8_t>(p[7]-c));
		
		uint64_t v01 = v0|(v1<<8);
		uint64_t v23 = v2|(v3<<8);
		uint64_t v45 = v4|(v5<<8);
		uint64_t v67 = v6|(v7<<8);
		
		uint64_t v0123 = v01 | (v23<<16);
		uint64_t v4567 = v45 | (v67<<16);
		
		uint64_t v = v0123 | (v4567<<32);
		
		p += 8;
		
		#endif
		
		// set lowest bit in byte if any bit is set in the byte
		v |= (v>>4);
		v |= (v>>2);
		v |= (v>>1);
		// keep lowest bit in each byte
		v &= 0x0101010101010101ull;
		// shift w by one position
		w <<= 1;
		// "add" bits for this 8 byte block to w
		w |= v;
	}

	// invert bit mask	
	w = ~w;

	// count number of bits set in w (equals number of c bytes in block)
	return libmaus::rank::PopCnt8<sizeof(unsigned long long)>::popcnt8(w);	
}

/**
 * Count number of c bytes in block of n bytes start from p.
 * This function is programmed to avoid control branching.
 *
 * @param p block start pointer
 * @param n size of block in bytes
 * @param c character to be counted
 * @return number of c bytes in block p of length n
 **/
static uint64_t count(uint8_t const * p, uint64_t const n, uint8_t const c = '\n')
{
	uint64_t cnt = 0;
	uint64_t full = n / 64;
	uint64_t const rest = n - (full*64);
	
	while ( full-- )
	{
		__builtin_prefetch(p+4096,0/*rw*/,1);
		#if 0
		__builtin_prefetch(p+1024,0/*rw*/,2);
		__builtin_prefetch(p+128,0/*rw*/,3);
		#endif
		cnt += count64(p,c);
		p += 64;
	}
	
	for ( uint64_t i = 0; i < rest; ++i )
		cnt += (*(p++) == c);
		
	return cnt;
}

/**
 * Count number of bytes equalling c in block of length n starting from p.
 *
 * @param p block start pointer
 * @param n size of block in bytes
 * @param c character to be counted
 * @return number of c bytes in block p of length n
 **/
static uint64_t countBranch(uint8_t const * p, uint64_t const n, uint8_t const c = '\n')
{
	uint64_t cnt = 0;
	uint8_t const * const pe = p + n;
	while ( p != pe )
		cnt += ((*(p++)) == c);
	return cnt;
}

void testbytecnt(uint8_t const * B, uint64_t const n, uint64_t const runs)
{
	libmaus::timing::RealTimeClock rtc;
	uint64_t cnt;

	rtc.start();
	cnt = 0;
	for ( uint64_t r = 0; r < runs; ++r )
		cnt += count(B,n,'\n');
	std::cerr << "simd\t" << rtc.getElapsedSeconds() << "\t" << cnt << std::endl;

	rtc.start();
	cnt = 0;
	for ( uint64_t r = 0; r < runs; ++r )
		cnt += countBranch(B,n,'\n');
	std::cerr << "branch\t" << rtc.getElapsedSeconds() << "\t" << cnt << std::endl;
}

int main()
{
	libmaus::random::Random::setup(time(0));
	uint64_t const n = 64*1024*1024;
	uint64_t const runs = 128;
	libmaus::autoarray::AutoArray<uint8_t> B(n,false);
	libmaus::timing::RealTimeClock rtc;

	for ( uint64_t i = 0; i < n; ++i )
		B[i] = (libmaus::random::Random::rand8() & 1) ? '\n' : 'a';
	testbytecnt(B.begin(),B.size(),runs);

	for ( uint64_t i = 0; i < n; ++i )
		B[i] = libmaus::random::Random::rand8();
	testbytecnt(B.begin(),B.size(),runs);

	for ( uint64_t i = 0; i < n; ++i )
		B[i] = i & 0xFF;
	testbytecnt(B.begin(),B.size(),runs);
}
