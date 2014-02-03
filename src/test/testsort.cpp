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
#include <libmaus/sorting/ParallelStableSort.hpp>
#include <libmaus/random/Random.hpp>

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

void testinplacesort2()
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
	libmaus::sorting::InPlaceParallelSort::inplacesort2(&A[0],&A[n]);
	
	#if 0
	for ( uint64_t i = 0; i < n; ++i )
		std::cerr << "A[" << i << "]=" << A[i] << std::endl;
	#endif

	for ( uint64_t i = 1; i < n; ++i )
		assert ( A[i-1] <= A[i] );
}


void testMultiMerge()
{
	libmaus::random::Random::setup();
	
	uint64_t n = 105812;
	libmaus::autoarray::AutoArray<uint64_t> V(n,false);
	libmaus::autoarray::AutoArray<uint64_t> W(n,false);
	for ( uint64_t i = 0; i < n; ++i )
		V[i] = ((i*29)%(31));
		// V[i] = libmaus::random::Random::rand8() % 2;
	
	uint64_t const l = n/2;
	uint64_t const r = n-l;
	uint64_t const l0 = l/2;
	uint64_t const l1 = l-l0;
	uint64_t const r0 = r/2;
	uint64_t const r1 = r-r0;
	
	std::sort(V.begin(),V.begin()+l);
	std::sort(V.begin()+l,V.begin()+l+r);

	#if 0	
	for ( uint64_t i = 0; i < l; ++i )
		std::cerr << V[i] << ";";
	std::cerr << std::endl;
	for ( uint64_t i = 0; i < r; ++i )
		std::cerr << V[l+i] << ";";
	std::cerr << std::endl;
	#endif

	libmaus::sorting::ParallelStableSort::parallelMerge(V.begin(),V.begin()+l,V.begin()+l,V.begin()+l+r,W.begin(),std::less<uint64_t>());
	
	std::cerr << std::string(80,'-') << std::endl;
	for ( uint64_t i = 0; i < W.size(); ++i )
	{
		std::cerr << W[i] << ";";
		assert ( i == 0 || W[i-1] <= W[i] );
	}
	std::cerr << std::endl;
}

void testMultiSort()
{
	libmaus::random::Random::setup();
	
	uint64_t rn = 16384;
	libmaus::autoarray::AutoArray<uint64_t> V(rn,false);
	libmaus::autoarray::AutoArray<uint64_t> W(rn,false);
	for ( uint64_t i = 0; i < V.size(); ++i )
		V[i] = ((i*29)%(31));
		// V[i] = libmaus::random::Random::rand8() % 2;
		// V[i] = libmaus::random::Random::rand8();

	std::less<uint64_t> order;
	uint64_t const * in = libmaus::sorting::ParallelStableSort::parallelSort(V.begin(),V.end(),W.begin(),W.end(),order);
	
	for ( uint64_t i = 0; i < V.size(); ++i )
	{
		std::cerr << in[i] << ";";
		assert ( i == 0 || in[i-1] <= in[i] );
	}
	std::cerr << std::endl;
}

int main()
{
	// testMultiMerge();
	testMultiSort();
	return 0;
	testBlockSwapDifferent();
	testBlockSwap();
	testblockmerge();
	//testinplacesort();
	testinplacesort2();
}
