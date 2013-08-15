/*
    libmaus
    Copyright (C) 2009-2013 German Tischler
    Copyright (C) 2011-2013 Genome Research Limited

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

#include <libmaus/sorting/InPlaceParallelSort.hpp>

void testBlockSwapDifferent()
{
	uint8_t p[256];
	
	for ( uint64_t s = 0; s <= sizeof(p); ++s )
	{
		uint8_t *pp = &p[0];
		uint64_t t = sizeof(p)/sizeof(p[0])-s;
		for ( uint64_t i = 0; i < s; ++i )
			*(pp++) = 1;
		for ( uint64_t i = s; i < (s+t); ++i )
			*(pp++) = 0;
		
		libmaus::sorting::InPlaceParallelSort::blockswap<uint64_t>(&p[0],s,t);
	
		for ( uint64_t i = 0; i < t; ++i )
			assert ( p[i] == 0 );
		for ( uint64_t i = 0; i < s; ++i )
			assert ( p[t+i] == 1 );
	}
}

void testBlockSwap()
{
	uint8_t p[2*89];
	uint8_t q[2*89];
	
	for ( uint64_t i = 0; i < 89; ++i )
	{
		p[i] = i;
		p[i+89] = (89-i-1);

		q[i+89] = i;
		q[i   ] = (89-i-1);
	}
		
	libmaus::sorting::InPlaceParallelSort::blockswap<uint64_t>(&p[0],&p[0]+89,89);
	
	for ( uint64_t i = 0; i < 2*89; ++i )
	{
		//std::cerr << "p[" << i << "]=" << static_cast<int64_t>(p[i]) << std::endl;
		assert ( p[i] == q[i] );
	}
}

void testblockmerge()
{
	uint32_t A[256];
	
	for ( uint64_t i = 0; i < 128; ++i )
	{
		A[i] = i;
		A[i+128] = 3*i+1;
	}
	
	// TrivialBaseSort TBS;
	libmaus::sorting::InPlaceParallelSort::FixedSizeBaseSort TBS(4);
	libmaus::sorting::InPlaceParallelSort::mergestep(&A[0],128,128,TBS);
		
	for ( uint64_t i = 1; i < sizeof(A)/sizeof(A[0]); ++i )
		assert ( A[i-1] <= A[i] );

	for ( uint64_t i = 0; i < 128; ++i )
	{
		A[i] = i;
		A[i+128] = 3*i+1;
	}
	
	std::reverse(&A[0],&A[128]);
	std::reverse(&A[128],&A[256]);

	libmaus::sorting::InPlaceParallelSort::mergestep(&A[0],128,128,std::greater<uint32_t>(),TBS);

	for ( uint64_t i = 1; i < sizeof(A)/sizeof(A[0]); ++i )
		assert ( A[i-1] >= A[i] );
	
	#if 0	
	for ( uint64_t i = 0; i < 256; ++i )
		std::cerr << "A[" << i << "]=" << A[i] << std::endl;
	#endif
}

void testinplacesort()
{
	uint64_t const n = 8ull*1024ull*1024ull*1024ull;
	libmaus::autoarray::AutoArray<uint32_t> A(n,false);
	
	#if defined(_OPENMP)
	#pragma omp parallel for
	#endif
	for ( int64_t i = 0; i < static_cast<int64_t>(n); ++i )
		A[i] = n-i-1;

	// libmaus::sorting::InPlaceParallelSort::ParallelFixedSizeBaseSort<uint32_t *, std::less<uint32_t> > TBS(512*1024, 4096) ;
	// FixedSizeBaseSort TBS(32*1024);
	// TrivialBaseSort TBS;
	// libmaus::sorting::InPlaceParallelSort::inplacesort(&A[0],&A[n],std::less<uint32_t>(),TBS);
	libmaus::sorting::InPlaceParallelSort::inplacesort(&A[0],&A[n]);
	
	#if 0
	for ( uint64_t i = 0; i < n; ++i )
		std::cerr << "A[" << i << "]=" << A[i] << std::endl;
	#endif

	for ( uint64_t i = 1; i < n; ++i )
		assert ( A[i-1] <= A[i] );
}

int main()
{
	testBlockSwapDifferent();
	testBlockSwap();
	testblockmerge();
	testinplacesort();
}
