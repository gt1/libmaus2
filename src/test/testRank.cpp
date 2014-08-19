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

#include <libmaus/rank/ERankBase.hpp>

#include <libmaus/lf/MultiRankCacheLF.hpp>
#include <libmaus/bp/BalancedParentheses.hpp>
#include <libmaus/util/IncreasingStack.hpp>
#include <libmaus/rank/CacheLineRank.hpp>
#include <libmaus/rank/ImpCacheLineRank.hpp>

#include <libmaus/util/Demangle.hpp>
#include <libmaus/bitio/OutputBuffer.hpp>
#include <libmaus/bitio/readElias.hpp>
#include <libmaus/bitio/FastWriteBitWriter.hpp>

#include <libmaus/wavelet/WaveletTree.hpp>

#include <libmaus/rank/ERank3.hpp>
#include <libmaus/rank/ERank3C.hpp>

#include <libmaus/rank/DecodeCache.hpp>
#include <libmaus/rank/EncodeCache.hpp>
#include <libmaus/rank/ChooseCache.hpp>

#include <libmaus/types/types.hpp>
#include <libmaus/rank/Proc01.hpp>
#include <libmaus/rank/ERank222B.hpp>
#include <libmaus/rank/ERank22B.hpp>
#include <libmaus/rank/ERank222.hpp>
#include <libmaus/rank/ERank22.hpp>
#include <libmaus/rank/ERank2.hpp>
#include <libmaus/rank/ERank.hpp>
#include <libmaus/rank/ERank222BP.hpp>
#include <libmaus/rank/ERank222BAppend.hpp>
#include <libmaus/rank/ERank222BAppendDynamic.hpp>
#include <libmaus/rank/FastRank.hpp>

#include <libmaus/rank/log2table.hpp>

#include <libmaus/autoarray/AutoArray.hpp>

#include <iostream>
#include <typeinfo>
#include <cstdlib>
#include <cassert>
#include <string>
#include <ctime>
#include <algorithm>
#include <numeric>

#include <libmaus/select/ESelect222B.hpp>

// using bitio::getBit1;
unsigned int shiftRand()
{
	return (rand() << 24) ^	(rand() << 16) ^ (rand() <<  8) ^ (rand());
}

template<typename bitwriter>
void randomBitVect(bitwriter & B, unsigned int const n)
{
	unsigned int i = 0;
	while ( i < n )
	{
		// unsigned int cnt = shiftRand() % n + 1;
		unsigned int cnt = (shiftRand() % 16384) + 1;

		switch ( shiftRand() % 3 )
		{
			case 0:
				//std::cerr << "adding " << cnt << " bits of " << 0 << std::endl;
				while ( (cnt--) && (i++ < n) ) B.writeBit(false);
				break;
			case 1:
				//std::cerr << "adding " << cnt << " bits of " << 1 << std::endl;
				while ( (cnt--) && (i++ < n) ) B.writeBit(true);
				break;
			case 2:
			default:
				//std::cerr << "adding " << cnt << " random bits " << std::endl;
				while ( (cnt--) && (i++ < n) ) B.writeBit(rand() & 1);
				break;
		}
	}
	B.flush();
}


unsigned int rmq(unsigned int const * const A, unsigned int const l, unsigned int const r)
{
	unsigned int v = std::numeric_limits<unsigned int>::max();
	
	for ( unsigned int i = l; i < r; ++i )
		v = std::min(v,A[i]);

	return v;
}

unsigned int rmqi(unsigned int const * const A, unsigned int const l, unsigned int const r)
{
	unsigned int v = std::numeric_limits<unsigned int>::max();
	
	for ( unsigned int i = l; i < r; ++i )
	{
		if ( v == std::numeric_limits<unsigned int>::max() )
			v = A[i];
		else
			v = std::max(v,A[i]);
	}

	return v;
}


template<typename eclass>
bool checkE2(unsigned int const rvecsize)
{
	typedef typename eclass::writer_type writer_type;
	typedef typename writer_type::data_type data_type;
	
	unsigned int const vecsize = ((rvecsize + 63) / 64) * 64;

	std::cerr 
		<< "checking rank class " << ::libmaus::util::Demangle::demangle<eclass>() << " "
		<< "writer type " << ::libmaus::util::Demangle::demangle<writer_type>() << " "
		<< "data type " << ::libmaus::util::Demangle::demangle<typename writer_type::data_type>() << std::endl;

	::libmaus::autoarray::AutoArray< data_type > AA( vecsize/ ( 8 * sizeof(data_type) ) , false );
	
	clock_t t_rank = 0;
	clock_t t_select = 0;
	
	unsigned int const loops = 10;
	double allselops = 0;
	
	bool ok = true;
	
	for ( unsigned int i = 0; i < loops; ++i )
	{
		writer_type B(AA.get());
		randomBitVect(B,vecsize);

		eclass E2(AA.get(),vecsize);
		
		// std::cerr << "size " << E2.byteSize() << std::endl;
		
		clock_t bef = clock();
		uint64_t c = 0;
		for ( uint64_t j = 0; j < vecsize; ++j )
		{
			if ( ::libmaus::bitio::getBit(AA.get(),j) )
				c++;
			assert ( E2.rank1(j) == c );
			ok = ok && ( E2.rank1(j) == c );
		}
		clock_t aft = clock();
		t_rank += (aft-bef);

		bef = clock();
		c = 0;
		unsigned int z = 0;
		for ( unsigned int j = 0; j < vecsize; ++j )
		{
			if ( ::libmaus::bitio::getBit(AA.get(),j) )
			{
				ok = ok && ( E2.select1(c) == j );
				c++;
			}
			else
			{
				ok = ok && ( E2.select0(z) == j );
				z++;
			}
		}
		allselops += c+z;
		aft = clock();
		t_select += (aft-bef);
	}
	
	std::string const classname = ::libmaus::util::Demangle::demangle<eclass>();

	if ( ok )
	{
		double const allops = loops * vecsize;

		std::cerr << classname << " loops " << loops << " vecsize " << vecsize << " check OK"<< std::endl;	
		std::cerr << "t_rank=" << t_rank << " per op " << (t_rank/allops)/CLOCKS_PER_SEC << " ops per sec " << 1.0/((t_rank/allops)/CLOCKS_PER_SEC) << std::endl;
		std::cerr << "t_select=" << t_select << " ops " << allselops << " per op " << (t_select/allselops)/CLOCKS_PER_SEC << " ops per sec " << 1.0/((t_select/allselops)/CLOCKS_PER_SEC) << std::endl;
	}
	else
	{
		std::cerr << classname << " loops " << loops << " vecsize " << vecsize << " check failed"<< std::endl;
	}
	
	return ok;
}

bool checkE2Append(unsigned int const rvecsize)
{
	typedef ::libmaus::rank::ERank222B eclass;
	typedef ::libmaus::rank::ERank222BAppend eappclass;
	typedef eclass::writer_type writer_type;
	typedef writer_type::data_type data_type;

	unsigned int const vecsize = ((rvecsize + 63) / 64) * 64;

	std::cerr 
		<< "rank class " << ::libmaus::util::Demangle::demangle<eappclass>() << " "
		<< "writer type " << ::libmaus::util::Demangle::demangle<writer_type>() << " "
		<< "data type " << ::libmaus::util::Demangle::demangle<data_type>() << std::endl;

	::libmaus::autoarray::AutoArray< data_type > AA( vecsize/ ( 8 * sizeof(data_type) ) , false );
		
	unsigned int const loops = 100;
	
	bool ok = true;
	
	for ( unsigned int i = 0; i < loops; ++i )
	{
		std::cerr << "loop " << i+1 << std::endl;
	
		// initialize random bit vector
		writer_type B(AA.get());
		randomBitVect(B,vecsize);

		// initialize rank dictionary on AA
		eclass E2(AA.get(),vecsize);
		
		#if 0
		::libmaus::autoarray::AutoArray< data_type > AAA( AA.size(), false );
		eappclass E2APP(AAA.get(),vecsize);
		#else
		libmaus::rank::ERank222BAppendDynamic E2APP;
		#endif
		
		std::cerr << ::libmaus::util::Demangle::demangle< libmaus::rank::ERank222BAppendDynamic >() << " loop " << (i+1) << std::endl;
		
		for ( uint64_t j = 0; j < vecsize; ++j )
		{
			bool const bit = libmaus::bitio::getBit(AA.get(),j);
			E2APP.appendBit(bit);
			
			if ( j % 1921 == 0 )
			{
				#if defined(_OPENMP)
				#pragma omp parallel for
				#endif
				for ( int k = 0; k <= static_cast<int>(j); ++k )
					assert ( E2APP.rank1(k) == E2.rank1(k) );
				
				#if defined(_OPENMP)
				#pragma omp parallel for
				#endif
				for ( int k = 0; k <= static_cast<int>(j); ++k )
					if ( libmaus::bitio::getBit(AA.get(),k) )
						assert ( E2APP.select1(E2APP.rank1(k)-1) == static_cast<unsigned int>(k) );
					else
						assert ( E2APP.select0(E2APP.rank0(k)-1) == static_cast<unsigned int>(k) );
			}
		}
	}
	
	return ok;
}

template<typename eclass>
bool checkE2Sel1(unsigned int const rvecsize)
{
	typedef typename eclass::writer_type writer_type;
	typedef typename writer_type::data_type data_type;
	
	unsigned int const vecsize = ((rvecsize + 63) / 64) * 64;

	std::cerr 
		<< "rank class " << ::libmaus::util::Demangle::demangle<eclass>() << " "
		<< "writer type " << ::libmaus::util::Demangle::demangle<writer_type>() << " "
		<< "data type " << ::libmaus::util::Demangle::demangle<data_type>() << std::endl;

	::libmaus::autoarray::AutoArray< data_type > AA( vecsize/ ( 8 * sizeof(data_type) ) , false );
	
	clock_t t_select = 0;
	
	unsigned int const loops = 10;
	double allselops = 0;
	
	bool ok = true;
	
	for ( unsigned int i = 0; i < loops; ++i )
	{
		writer_type B(AA.get());
		randomBitVect(B,vecsize);

		eclass E2(AA.get(),vecsize);

		clock_t bef = clock();
		uint64_t c = 0;
		for ( unsigned int j = 0; j < vecsize; ++j )
			if ( ::libmaus::bitio::getBit(AA.get(),j) )
				ok = ok && ( E2.select1(c++) == j );

		allselops += c;
		clock_t aft = clock();
		t_select += (aft-bef);
	}

	std::string const classname = ::libmaus::util::Demangle::demangle<eclass>();

	if ( ok )
	{
		std::cerr << classname << " loops " << loops << " vecsize " << vecsize << " check OK"<< std::endl;	
		std::cerr << "t_select=" << t_select << " ops " << allselops << " per op " << (t_select/allselops)/CLOCKS_PER_SEC << " ops per sec " << 1.0/((t_select/allselops)/CLOCKS_PER_SEC) << std::endl;
	}
	else
	{
		std::cerr << classname << " loops " << loops << " vecsize " << vecsize << " check failed"<< std::endl;
	}
	
	return ok;
}

template<typename eclass>
bool checkE2Sel0(unsigned int const rvecsize)
{
	typedef typename eclass::writer_type writer_type;
	typedef typename writer_type::data_type data_type;
	
	unsigned int const vecsize = ((rvecsize + 63) / 64) * 64;

	std::cerr 
		<< "rank class " << ::libmaus::util::Demangle::demangle<eclass>() << " "
		<< "writer type " << ::libmaus::util::Demangle::demangle<writer_type>() << " "
		<< "data type " << ::libmaus::util::Demangle::demangle<data_type>() << std::endl;

	::libmaus::autoarray::AutoArray< data_type > AA( vecsize/ ( 8 * sizeof(data_type) ) , false );
	
	clock_t t_select = 0;
	
	unsigned int const loops = 10;
	double allselops = 0;
	
	bool ok = true;
	
	for ( unsigned int i = 0; i < loops; ++i )
	{
		writer_type B(AA.get());
		randomBitVect(B,vecsize);

		eclass E2(AA.get(),vecsize);

		clock_t bef = clock();
		uint64_t c = 0;
		for ( unsigned int j = 0; j < vecsize; ++j )
			if ( !::libmaus::bitio::getBit(AA.get(),j) )
				ok = ok && ( E2.select1(c++) == j );

		allselops += c;
		clock_t aft = clock();
		t_select += (aft-bef);
	}
	
	std::string const classname = ::libmaus::util::Demangle::demangle<eclass>();

	if ( ok )
	{
		std::cerr << classname << " loops " << loops << " vecsize " << vecsize << " check OK"<< std::endl;	
		std::cerr << "t_select=" << t_select << " ops " << allselops << " per op " << (t_select/allselops)/CLOCKS_PER_SEC << " ops per sec " << 1.0/((t_select/allselops)/CLOCKS_PER_SEC) << std::endl;
	}
	else
	{
		std::cerr << classname << " loops " << loops << " vecsize " << vecsize << " check failed"<< std::endl;
	}
	
	return ok;
}

template<typename eclass>
bool checkEP28(unsigned int const rvecsize)
{
	unsigned int const vecsize = ((rvecsize + 63) / 64) * 64;

	::libmaus::autoarray::AutoArray<uint64_t> AA ( vecsize/64 + 1, false ); AA[vecsize/64] = 0;
	::libmaus::autoarray::AutoArray<uint64_t> AAA( vecsize/64    , false );

	clock_t t_rank = 0;
	clock_t t_select = 0;
	
	unsigned int const loops = 10;
	double allselops = 0;
	
	bool ok = true;
	
	for ( unsigned int i = 0; i < loops; ++i )
	{
		::libmaus::bitio::BitWriter8 B(AA.get());
		randomBitVect(B,vecsize);

		::libmaus::bitio::BitWriter8 BAAA(AAA.get());
		for ( unsigned int j = 0; j < vecsize; ++j )
			if ( ::libmaus::bitio::getBit8( AA.get(), j ) == 0 && ::libmaus::bitio::getBit8( AA.get(), j+1) == 1 )
				BAAA.writeBit(1);
			else
				BAAA.writeBit(0);

		eclass E2(AA.get(),vecsize);
		
		clock_t bef = clock();
		unsigned int c = 0;
		for ( unsigned int j = 0; j < vecsize; ++j )
		{
			if ( ::libmaus::bitio::getBit8(AAA.get(),j) )
				c++;
			ok = ok && ( E2.rank1(j) == c );
		}
		clock_t aft = clock();
		t_rank += (aft-bef);

		bef = clock();
		c = 0;
		unsigned int z = 0;
		for ( unsigned int j = 0; j < vecsize; ++j )
		{
			if ( ::libmaus::bitio::getBit8(AAA.get(),j) )
			{
				ok = ok && ( E2.select1(c) == j );
				c++;
			}
			/*
			else
			{
				ok = ok && ( E2.select0(c) == j );
				z++;			
			}
			*/
		}
		allselops += c+z;
		aft = clock();
		t_select += (aft-bef);
	}
	
	std::string const classname = ::libmaus::util::Demangle::demangle<eclass>();

	if ( ok )
	{
		double const allops = loops * vecsize;

		std::cerr << classname << " loops " << loops << " vecsize " << vecsize << " check OK"<< std::endl;	
		std::cerr << "t_rank=" << t_rank << " per op " << (t_rank/allops)/CLOCKS_PER_SEC << " ops per sec " << 1.0/((t_rank/allops)/CLOCKS_PER_SEC) << std::endl;
		std::cerr << "t_select=" << t_select << " ops " << allselops << " per op " << (t_select/allselops)/CLOCKS_PER_SEC << " ops per sec " << 1.0/((t_select/allselops)/CLOCKS_PER_SEC) << std::endl;
	}
	else
	{
		std::cerr << classname << " loops " << loops << " vecsize " << vecsize << " check failed"<< std::endl;
	}
	
	return ok;
}

#include <cmath>

void initRand()
{
	// srand(4);
	srand( time(0) );
}

void checkCaches()
{
	/*
	for ( unsigned int i = (1u<<15); i <= 1u<<16; ++i )
	{
		std::cerr 
			<< static_cast<double>(i)/(1u<<16) << " / " << static_cast<double>(logtable2_16_16[i]) / (1u<<16) 
			<< "\t"
			<< i
			<< "\t"
			<< logtable2_16_16[i]
			<< std::endl;
	}
	*/
	::libmaus::rank::EncodeCache<16,uint16_t> EC16;
	::libmaus::rank::DecodeCache<16,uint16_t> DC16;

	for ( unsigned int i = 0; i < (1u<<16); ++i )
	{
		unsigned int const b = EC16.popcnt(i);
		// std::cerr << i << "\t" << EC16.encode(i) << "\t" << static_cast<int>(EC16.max_n[b]) << "\t" << static_cast<int>(EC16.bits_n[b]) << std::endl;
		assert ( DC16.decode(EC16.encode(i),b) == i );
	}

	unsigned int const bb = 16;
	for ( unsigned int i = 0; i <= bb; ++i )
	{
		double log0 = i ? ::log(i/static_cast<double>(bb))/::log(2.0) : 0;
		double log1 = (bb-i) ? ::log((bb-i)/static_cast<double>(bb))/::log(2.0) : 0;
		double ent0 = i * -log0;
		double ent1 = (bb-i) * -log1;
		std::cerr << i << "\t" 
			<< ::libmaus::rank::entropy_estimate_down(i,bb) 
			<< "\t"
			<< ::libmaus::rank::entropy_estimate_up(i,bb)
			<< "\t"
			<< static_cast<int>(EC16.bits_n[i])
			<< "\t" << ent0
			<< "\t" << ent1 << std::endl;
	}

}

bool checkRank()
{
	unsigned int const vecsize =  10*1024*1024;
	// unsigned int const vecsize =  100*1024*1024;
	
	bool ok = true;

	ok = ok && checkE2< ::libmaus::rank::ERank3C>(vecsize);
	ok = ok && checkE2< ::libmaus::rank::ERank222B>(vecsize);

	ok = ok && checkE2Sel0< libmaus::select::ESelect222B<0> >(vecsize);
	ok = ok && checkE2Sel1< libmaus::select::ESelect222B<1> >(vecsize);

	ok = ok && checkE2< ::libmaus::rank::ERank>(vecsize);
	ok = ok && checkE2< ::libmaus::rank::ERank2>(vecsize);
	ok = ok && checkE2< ::libmaus::rank::ERank22>(vecsize);
	ok = ok && checkE2< ::libmaus::rank::ERank222>(vecsize);

	ok = ok && checkE2< ::libmaus::rank::ERank22B>(vecsize);

	ok = ok && checkE2< ::libmaus::rank::ERank3>(vecsize);

	ok = ok && checkEP28< ::libmaus::rank::ERank222BP <  ::libmaus::rank::Proc01<uint64_t> > >(vecsize);
	
	if ( ok )
		std::cerr << "All tests ok." << std::endl;
	else
		std::cerr << "Test failed." << std::endl;

	return ok;
}

unsigned int smallestLargerEqual(unsigned int const * const A, unsigned int left, unsigned int right, unsigned int refpos)
{
	unsigned int p = std::numeric_limits<unsigned int>::max();
	for ( unsigned int i = left; i < right; ++i )
		if ( A[i] >= refpos )
			p = std::min(p,A[i]);
	return p;
}

bool checkRNVPermutations(unsigned int n)
{
	::libmaus::autoarray::AutoArray<unsigned int> AA(n,false); unsigned int * const A = AA.get();
	for ( unsigned int i = 0; i < n; ++i ) A[i] = i;
		
	// iterate over all permutations of length n
	do
	{
		::libmaus::wavelet::WaveletTree< ::libmaus::rank::ERank222B, unsigned int > W(A,n);
		
		#if 0
		std::cerr << "checking ";
		for ( unsigned int i = 0; i < n; ++i )
			std::cerr << A[i] << ";";
		std::cerr << std::endl;
		#endif
			
		// check all intervals and limit numbers
		for ( unsigned int i = 0; i <= n; ++i )
			for ( unsigned int j = 0; j < i; ++j )
			{
				// std::cerr << "checking [" << j << "," << i << ")" << std::endl;
				
				for ( unsigned int k = 0; k < n; ++k )
				{
					unsigned int const p0 = smallestLargerEqual(A,j,i,k);
					unsigned int const p1 = W.rnv(j,i,k);
					
					// std::cerr << p0 << "\t" << p1 << std::endl;
					
					if ( p0 != p1 )
						return false;
				}	
			}
	} while ( std::next_permutation(A,A+n) );
	
	return true;
}

unsigned int largestSmallerEqual(unsigned int const * const A, unsigned int left, unsigned int right, unsigned int refpos)
{
	unsigned int p = std::numeric_limits<unsigned int>::max();
	
	for ( unsigned int i = left; i < right; ++i )
		if ( A[i] <= refpos )
		{
			if ( p == std::numeric_limits<unsigned int>::max() )
				p = A[i];
			else
				p = std::max(p,A[i]);
		}
	return p;
}

bool checkRPVPermutations(unsigned int n)
{
	::libmaus::autoarray::AutoArray<unsigned int> AA(n,false); unsigned int * const A = AA.get();
	for ( unsigned int i = 0; i < n; ++i ) A[i] = i;
		
	// iterate over all permutations of length n
	do
	{
		::libmaus::wavelet::WaveletTree< ::libmaus::rank::ERank222B, unsigned int > W(A,n);
		
		#if 0
		std::cerr << "checking ";
		for ( unsigned int i = 0; i < n; ++i )
			std::cerr << A[i] << ";";
		std::cerr << std::endl;
		#endif
			
		// check all intervals and limit numbers
		for ( unsigned int i = 0; i <= n; ++i )
			for ( unsigned int j = 0; j < i; ++j )
			{
				// std::cerr << "checking [" << j << "," << i << ")" << std::endl;
				
				for ( unsigned int k = 0; k < n; ++k )
				{
					unsigned int const p0 = largestSmallerEqual(A,j,i,k);
					unsigned int const p1 = W.rpv(j,i,k);
					
					// std::cerr << k << "\t" << p0 << "\t" << p1 << std::endl;
					
					if ( (p0 != p1) )
						return false;
				}	
			}
	} while ( std::next_permutation(A,A+n) );
	
	return true;
}

bool checkRPVPermutationsQuick(unsigned int n)
{
	::libmaus::autoarray::AutoArray<unsigned int> AA(n,false); unsigned int * const A = AA.get();
	for ( unsigned int i = 0; i < n; ++i ) A[i] = i;
		
	// iterate over all permutations of length n
	do
	{
		::libmaus::wavelet::QuickWaveletTree< ::libmaus::rank::ERank222B, unsigned int > W(A,n);
		
		#if 0
		std::cerr << "checking ";
		for ( unsigned int i = 0; i < n; ++i )
			std::cerr << A[i] << ";";
		std::cerr << std::endl;
		#endif
			
		// check all intervals and limit numbers
		for ( unsigned int i = 0; i <= n; ++i )
			for ( unsigned int j = 0; j < i; ++j )
			{
				// std::cerr << "checking [" << j << "," << i << ")" << std::endl;
				
				for ( unsigned int k = 0; k < n; ++k )
				{
					unsigned int const p0 = largestSmallerEqual(A,j,i,k);
					unsigned int const p1 = W.rpv(j,i,k);
					
					// std::cerr << k << "\t" << p0 << "\t" << p1 << std::endl;
					
					if ( (p0 != p1) )
						return false;
				}	
			}
	} while ( std::next_permutation(A,A+n) );
	
	return true;
}

bool checkRNVPermutationsQuick(unsigned int n)
{
	::libmaus::autoarray::AutoArray<unsigned int> AA(n,false); unsigned int * const A = AA.get();
	for ( unsigned int i = 0; i < n; ++i ) A[i] = i;
		
	// iterate over all permutations of length n
	do
	{
		::libmaus::wavelet::QuickWaveletTree< ::libmaus::rank::ERank222B, unsigned int > W(A,n);
		W.print(std::cerr,0,n);
		W.print(std::cerr,0,n,0);
		
		#if 0
		std::cerr << "checking ";
		for ( unsigned int i = 0; i < n; ++i )
			std::cerr << A[i] << ";";
		std::cerr << std::endl;
		#endif
			
		// check all intervals and limit numbers
		for ( unsigned int i = 0; i <= n; ++i )
			for ( unsigned int j = 0; j < i; ++j )
			{
				// std::cerr << "checking [" << j << "," << i << ")" << std::endl;
				
				for ( unsigned int k = 0; k < n; ++k )
				{
					unsigned int const p0 = smallestLargerEqual(A,j,i,k);
					unsigned int const p1 = W.rnv(j,i,k);
					
					// std::cerr << k << "\t" << p0 << "\t" << p1 << std::endl;
					
					if ( (p0 != p1) )
						return false;
				}	
			}
	} while ( std::next_permutation(A,A+n) );
	
	return true;
}

bool checkAccessPermutationsQuick(unsigned int n)
{
	::libmaus::autoarray::AutoArray<unsigned int> AA(n,false); unsigned int * const A = AA.get();
	for ( unsigned int i = 0; i < n; ++i ) A[i] = i;
		
	// iterate over all permutations of length n
	do
	{
		::libmaus::wavelet::QuickWaveletTree< ::libmaus::rank::ERank222B, unsigned int > W(A,n);
		
		#if 0
		std::cerr << "checking ";
		for ( unsigned int i = 0; i < n; ++i )
			std::cerr << A[i] << ";";
		std::cerr << std::endl;
		#endif
			
		// check all intervals and limit numbers
		for ( unsigned int i = 0; i < n; ++i )
			assert ( A[i] == W.access(i) );
	} while ( std::next_permutation(A,A+n) );
	
	return true;
}

bool checkRmqPermutations(unsigned int n)
{
	::libmaus::autoarray::AutoArray<unsigned int> AA(n,false); unsigned int * const A = AA.get();
	for ( unsigned int i = 0; i < n; ++i ) A[i] = i;
		
	// iterate over all permutations of length n
	do
	{
		::libmaus::wavelet::WaveletTree< ::libmaus::rank::ERank222B, unsigned int > W(A,n);
		
		#if 0
		std::cerr << "checking ";
		for ( unsigned int i = 0; i < n; ++i )
			std::cerr << A[i] << ";";
		std::cerr << std::endl;
		#endif
			
		// check all intervals and limit numbers
		for ( unsigned int i = 0; i <= n; ++i )
			for ( unsigned int j = 0; j < i; ++j )
			{
				unsigned int const min0 = rmq(A,j,i);
				unsigned int const min1 = W.rmq(j,i);
				unsigned int const max0 = rmqi(A,j,i);
				unsigned int const max1 = W.rmqi(j,i);
				
				if ( min0 != min1 )
				{
					std::cerr << "j=" << j << " i=" << i << " A=" << min0 << " W=" << min1 << std::endl;
				}
				if ( max0 != max1 )
				{
					std::cerr << "j=" << j << " i=" << i << " A=" << max0 << " W=" << max1 << std::endl;
				}
				
				if ( ! (min0 == min1) && (max0 == max1) )
					return false;
			}
		
		for ( unsigned int i = 0; i  < n; ++i )
			if ( W[i] != A[i] )
				return false;

	} while ( std::next_permutation(A,A+n) );
	
	return true;
}

bool checkRmqPermutationsQuick(unsigned int n)
{
	::libmaus::autoarray::AutoArray<unsigned int> AA(n,false); unsigned int * const A = AA.get();
	for ( unsigned int i = 0; i < n; ++i ) A[i] = i;
		
	// iterate over all permutations of length n
	do
	{
		::libmaus::wavelet::QuickWaveletTree< ::libmaus::rank::ERank222B, unsigned int > W(A,n);
		
		#if 0
		std::cerr << "checking ";
		for ( unsigned int i = 0; i < n; ++i )
			std::cerr << A[i] << ";";
		std::cerr << std::endl;
		#endif
			
		// check all intervals and limit numbers
		for ( unsigned int i = 0; i <= n; ++i )
			for ( unsigned int j = 0; j < i; ++j )
			{
				unsigned int const min0 = rmq(A,j,i);
				unsigned int const min1 = W.rmq(j,i);
				unsigned int const max0 = rmqi(A,j,i);
				unsigned int const max1 = W.rmqi(j,i);
				
				if ( min0 != min1 )
				{
					std::cerr << "j=" << j << " i=" << i << " A=" << min0 << " W=" << min1 << std::endl;
				}
				if ( max0 != max1 )
				{
					std::cerr << "j=" << j << " i=" << i << " A=" << max0 << " W=" << max1 << std::endl;
				}
				
				if ( ! (min0 == min1) && (max0 == max1) )
					return false;
			}
		
		for ( unsigned int i = 0; i  < n; ++i )
			if ( W[i] != A[i] )
				return false;

	} while ( std::next_permutation(A,A+n) );
	
	return true;
}

void waveletTreePrint()
{
	unsigned int A[] = { 0, 6, 3, 2, 5, 1, 4, 7 };
	unsigned int const n = sizeof(A)/sizeof(A[0]);
	::libmaus::wavelet::WaveletTree< ::libmaus::rank::ERank222B, unsigned int > W(A,n);
	
	std::cerr << "---" << std::endl;
	W.print(std::cerr);
	std::cerr << "---" << std::endl;
	W.print(std::cerr, 2, 6);
	std::cerr << "---" << std::endl;
	W.print(std::cerr, 2, 6, 4);
	std::cerr << "---" << std::endl;
	std::cerr << "rnv(2,6,4) = " << W.rnv(2,6,4) << std::endl;
}

template<typename iter>
void randomSequence(iter A, uint64_t const n)
{
	for ( uint64_t i = 0; i < n; ++i )
		A[i] = rand() % 33;
}

template<typename type>
::libmaus::autoarray::AutoArray<type> randomSequence(uint64_t const n)
{
	::libmaus::autoarray::AutoArray<type> A(n,false);
	randomSequence(A.get(),n);
	return A;
}

#include <map>

void waveletTreeRankSelectRandom(uint64_t const n)
{
	typedef uint32_t symtype;
	
	::libmaus::autoarray::AutoArray<symtype> A = randomSequence<symtype>(n);
	::libmaus::wavelet::WaveletTree< ::libmaus::rank::ERank222B, symtype > W(A.get(),n);
	::libmaus::wavelet::QuickWaveletTree< ::libmaus::rank::ERank222B, symtype > Q(A.get(),n);
	
	std::map< uint32_t, unsigned int> M;
	
	for ( uint64_t i = 0; i < n; ++i )
	{
		M[ A[i] ] ++;
		
		assert ( W.rank ( A[i] , i ) == M [ A[i] ] );
		assert ( W.select ( A[i], M[A[i]] - 1 ) == i );
		assert ( Q.rank ( A[i] , i ) == M [ A[i] ] );
		assert ( Q.select ( A[i], M[A[i]] - 1 ) == i );
	}
}

void waveletTreeRankSelect()
{
	unsigned int A[] = { 9, 9, 9, 1, 0, 3, 2, 4, 7, 6, 8, 5, 11 };
	unsigned int const n = sizeof(A)/sizeof(A[0]);
	::libmaus::wavelet::WaveletTree< ::libmaus::rank::ERank222B, unsigned int > W(A,n);
	::libmaus::wavelet::QuickWaveletTree< ::libmaus::rank::ERank222B, unsigned int > Q(A,n);
	
	for ( unsigned int i = 0; i < n; ++i )
		std::cerr << A[i] << ";";
	std::cerr << std::endl;

	for ( unsigned int i = 0; i < n; ++i )
		std::cerr << W.rank(0,i) << ";";
	std::cerr << std::endl;

	for ( unsigned int i = 0; i < n; ++i )
	{
		std::cerr << Q.rank(0,i) << ";";
		assert ( W.rank(0,i) == Q.rank(0,i) );
	}
	std::cerr << std::endl;
	
	for ( unsigned int i = 0; i < n; ++i )
		std::cerr << W.rank(1,i) << ";";
	std::cerr << std::endl;

	for ( unsigned int i = 0; i < n; ++i )
	{
		std::cerr << Q.rank(1,i) << ";";
		assert ( W.rank(1,i) == Q.rank(1,i) );
	}
	std::cerr << std::endl;
	
	for ( unsigned int i = 0; i < n; ++i )
		std::cerr << W.rank(2,i) << ";";
	std::cerr << std::endl;
	for ( unsigned int i = 0; i < n; ++i )
		std::cerr << W.rank(3,i) << ";";
	std::cerr << std::endl;
	for ( unsigned int i = 0; i < n; ++i )
		std::cerr << W.rank(4,i) << ";";
	std::cerr << std::endl;
	for ( unsigned int i = 0; i < n; ++i )
		std::cerr << W.rank(7,i) << ";";
	std::cerr << std::endl;
	for ( unsigned int i = 0; i < n; ++i )
		std::cerr << W.rank(6,i) << ";";
	std::cerr << std::endl;
	for ( unsigned int i = 0; i < n; ++i )
		std::cerr << W.rank(8,i) << ";";
	std::cerr << std::endl;
	for ( unsigned int i = 0; i < n; ++i )
		std::cerr << W.rank(5,i) << ";";
	std::cerr << std::endl;
	for ( unsigned int i = 0; i < n; ++i )
		std::cerr << W.rank(9,i) << ";";
	std::cerr << std::endl;
	for ( unsigned int i = 0; i < n; ++i )
		std::cerr << W.rank(50,i) << ";";
	std::cerr << std::endl;
	
	for ( unsigned int i = 0; i < 12; ++i )
		for ( unsigned int j = 0; j < 5; ++j )
		{
			std::cerr << "select(" << i << "," << j << ")=" << W.select(i,j) << std::endl;
			
			assert ( W.select(i,j) == Q.select(i,j) ) ; 
		}
	
	std::cerr << "array: ";
	for ( unsigned int i = 0; i < n; ++i )
		std::cerr << W[i] << ";";
	std::cerr << std::endl;
}

void waveletTreeCheckRMQ()
{
	unsigned int A[] = { 9, 9, 9, 1, 0, 3, 2, 4, 7, 6, 8, 5, 11 };
	unsigned int const n = sizeof(A)/sizeof(A[0]);
	::libmaus::wavelet::WaveletTree< ::libmaus::rank::ERank222B, unsigned int > W(A,n);
	
	for ( unsigned int i = 0; i < n; ++i )
		std::cerr << A[i] << "(" << W[i] << ")" << ";";
	std::cerr << std::endl;
	
	W.rmqi(1,3);
	
	#if 1
	bool rmqok = true;
	
	for ( unsigned int i = 0; rmqok && i < n; ++i )
		for ( unsigned int j = 0; rmqok && j < i; ++ j )
		{
			unsigned int const min0 = rmq(A,j,i);
			unsigned int const min1 = W.rmq(j,i);
			unsigned int const max0 = rmqi(A,j,i);
			unsigned int const max1 = W.rmqi(j,i);
			
			if ( min0 != min1 )
			{
				std::cerr << "j=" << j << " i=" << i << " A=" << min0 << " W=" << min1 << std::endl;
			}
			if ( max0 != max1 )
			{
				std::cerr << "j=" << j << " i=" << i << " A=" << max0 << " W=" << max1 << std::endl;
			}
			
			rmqok = rmqok && (min0 == min1) && (max0 == max1);	
		}
	
	std::cerr << "ok: " << rmqok << std::endl;
	#endif
}

#include <sstream>

void callWaveletTreeRankSelectRandom(unsigned int const q)
{
	for ( unsigned int i = 0; i < q; ++i )
	{
		uint64_t const n = (rand() % 16384)+24;
		waveletTreeRankSelectRandom(n);
		std::cerr << "\r                            \r" << i << "/" << q;
	}
	std::cerr << std::endl;
}

void elias()
{
	std::vector< uint8_t > V;
	std::back_insert_iterator< std::vector<uint8_t> > backit ( V ) ;
	::libmaus::bitio::BitWriterVector8 BWV8( backit );
	
	BWV8.writeElias2(243);
	BWV8.flush();
	
	uint64_t qq = 0;
	std::cerr << "elias2: " << ::libmaus::bitio::readElias2( V.begin(), qq ) << std::endl;
}

unsigned int numbits(unsigned int n)
{
	unsigned int c = 0;
	while ( n )
	{
		c++;
		n >>= 1;
	}
	
	return c;
}

void checkStreams8(unsigned int const n)
{
	std::ostringstream faststr, slowerstr;
	::libmaus::bitio::FastWriteBitWriterStream8 fast(faststr);
	::libmaus::bitio::BitWriterStream8 slower(slowerstr);
		
	std::vector<unsigned int> V;
	std::vector<unsigned int> B;
	
	for ( unsigned int i = 0; i < n; ++i )
	{
		unsigned int const bits = rand() % 24;
		unsigned int const mask = (1ul << bits) - 1;
		V.push_back( rand() & mask );
		B.push_back( bits );
	}

	clock_t bef_fast = clock();	
	for ( unsigned int i = 0; i < n; ++i )
		fast.write(V[i], B[i]);
	fast.flush();
	faststr.flush();
	clock_t aft_fast = clock();
	
	clock_t bef_slow = clock();
	for ( unsigned int i = 0; i < n; ++i )
		slower.write(V[i], B[i]);
	slower.flush();
	slowerstr.flush();
	clock_t aft_slow = clock();

	assert ( faststr.str() == slowerstr.str() );
	
	std::cerr << "slow: " << aft_slow-bef_slow << " fast: " << aft_fast-bef_fast << std::endl;
}

void checkStreams64(unsigned int const n)
{
	std::vector < uint64_t > faststr, slowerstr;

	std::back_insert_iterator< std::vector<uint64_t> > fastbackit ( faststr ) ;
	std::back_insert_iterator< std::vector<uint64_t> > slowerbackit ( slowerstr ) ;
	
	std::ostringstream sostr; ::libmaus::bitio::OutputBuffer< uint64_t > output(10*1024, sostr); ::libmaus::bitio::OutputBufferIterator<uint64_t> outit(output);
	
	::libmaus::bitio::FastWriteBitWriterVector64 fast(fastbackit);
	::libmaus::bitio::BitWriterVector64 slower(slowerbackit);
	::libmaus::bitio::FastWriteBitWriterBuffer64 fastbuf(outit);

	std::cerr << "checking " << ::libmaus::util::Demangle::demangle(fast) << " and " <<
		::libmaus::util::Demangle::demangle(slower) << std::endl;
		
	std::vector<uint64_t> V;
	std::vector<unsigned int> B;
	
	for ( unsigned int i = 0; i < n; ++i )
	{
		unsigned int const bits = rand() % 24;
		uint64_t const mask = (1ULL << bits) - 1;
		V.push_back( rand() & mask );
		B.push_back( bits );
	}
	
	clock_t bef_fast = clock();	
	for ( unsigned int i = 0; i < n; ++i )
		fast.write(V[i], B[i]);
	fast.flush();
	clock_t aft_fast = clock();
	
	clock_t bef_slow = clock();
	for ( unsigned int i = 0; i < n; ++i )
		slower.write(V[i], B[i]);
	slower.flush();
	clock_t aft_slow = clock();

	clock_t bef_buf = clock();	
	for ( unsigned int i = 0; i < n; ++i )
		fastbuf.write(V[i], B[i]);
	fastbuf.flush();
	output.flush();
	clock_t aft_buf = clock();
	
	assert ( faststr == slowerstr );
	
	std::cerr << "slow: " << aft_slow-bef_slow << " fast: " << aft_fast-bef_fast << " buf: " << aft_buf-bef_buf << std::endl;
	
	unsigned int totalbits = std::accumulate(B.begin(),B.end(),0);
	unsigned int totalwords = (totalbits + 63)/64;
	::libmaus::autoarray::AutoArray<uint64_t> inbuf(totalwords,false);
	memcpy(inbuf.get(),sostr.str().c_str(),totalwords*8);
	
	uint64_t p = 0;
	for ( unsigned int i = 0; i < n; ++i )
	{
		assert ( ::libmaus::bitio::readBinary(inbuf.get(),p,B[i]) == V[i] );
	}
}

void waveletTreeSmallerLargerRandom(uint64_t const n)
{
	typedef uint16_t symtype;

	::libmaus::autoarray::AutoArray<symtype> A = randomSequence<symtype>(n);
	::libmaus::wavelet::WaveletTree< ::libmaus::rank::ERank222B, symtype > W(A.get(),n);
	
	#if 0
	std::cerr << "sequence: ";
	for ( uint64_t i = 0; i < n; ++i )
		std::cerr << A[i] << ";";
	std::cerr << std::endl;
	#endif
	
	for ( uint64_t i = 0; i < 10000; ++i )
	{
		uint64_t left = rand() % n;
		uint64_t right = rand() % n;
		if ( right < left )
			std::swap(left,right);
		uint64_t sym = rand() & ((1ull << W.getB())-1);
		
		uint64_t refsmaller = 0, reflarger = 0;
		
		for ( uint64_t j = left; j < right; ++j )
		{
			if ( A[j] < sym )
				refsmaller++;
			if ( A[j] > sym )
				reflarger++;
		}
		
		uint64_t smaller, larger;
		
		W.smallerLarger(sym,left,right,smaller,larger);

		#if 0
		std::cerr << "left=" << left << " right=" << right << " sym=" << sym 
			<< " refsmaller=" << refsmaller << " smaller=" << smaller 
			<< " reflarger=" << reflarger << " larger=" << larger 
			<< std::endl;
		#endif
		
		assert ( smaller == refsmaller );
		assert ( larger == reflarger );
	}
}

bool checkSortedSymbols(unsigned int n)
{
	::libmaus::autoarray::AutoArray<unsigned int> AA(n,false); unsigned int * const A = AA.get();
	for ( unsigned int i = 0; i < n; ++i ) A[i] = (rand() % 4);
		
	// iterate over all permutations of length n
	::libmaus::wavelet::WaveletTree< ::libmaus::rank::ERank222B, unsigned int > W(A,n);
	
	std::vector<unsigned int> S(AA.get(), AA.get()+n);
	std::sort(S.begin(),S.end());
	
	for ( unsigned int i = 0; i < S.size(); ++ i )
	{
		unsigned int sortsym = W.sortedSymbol(i);
		
		if ( S[i] != sortsym )
		{
			std::cerr << "A[]={";
			for ( unsigned int j = 0; j < n; ++j )
				std::cerr << A[j] << ";";
			std::cerr << "}, i=" << i << " S[i]=" << S[i] << " sortsym=" << sortsym << std::endl;
		
			assert ( false );
			return false;
		}
	}
	
	return true;
}

bool checkRangeCountPermutations(unsigned int n)
{
	::libmaus::autoarray::AutoArray<unsigned int> AA(n,false); unsigned int * const A = AA.get();
	for ( unsigned int i = 0; i < n; ++i ) A[i] = i;
	
	// iterate over all permutations of length n
	while ( std::next_permutation(A,A+n) )
	{
		::libmaus::wavelet::WaveletTree< ::libmaus::rank::ERank222B, unsigned int > W(A,n);
		
		for ( unsigned int i = 0; i < n; ++i )
			for ( unsigned int j = i; j < n; ++j )
			{
				std::map<unsigned int,unsigned int> M;
				for ( unsigned k = i; k < j; ++k )
					M [ A[k] ]++;

				::libmaus::autoarray::AutoArray< std::pair < unsigned int, uint64_t > > N = W.rangeCount(i,j);
				std::map<unsigned int, unsigned int> M2;
				for ( unsigned int k = 0; k < N.getN(); ++k )
					M2 [ N[k].first ] = N[k].second;
				
				if ( M != M2 )
				{
					std::cerr << "Failure, A=";
					
					for ( unsigned int k = 0; k < n; ++k )
						std::cerr << A[k] << ";";
					std::cerr  << " [" << i << "," << j << ")" << std::endl;
					
					std::cerr << "M=";
					for ( std::map<unsigned int,unsigned int>::const_iterator ita = M.begin(); ita != M.end(); ++ita )
						std::cerr << "(" << ita->first << "," << ita->second << ")";
					std::cerr << std::endl;
					std::cerr << "M2=";
					for ( std::map<unsigned int,unsigned int>::const_iterator ita = M2.begin(); ita != M2.end(); ++ita )
						std::cerr << "(" << ita->first << "," << ita->second << ")";
						
					std::cerr << " " << N.getN() << std::endl;
					std::cerr << std::endl;
				
					assert ( M == M2 );
				}
			}
	}
	
	return true;
}

bool checkRangeCountRand(unsigned int const n, unsigned int const it)
{
	::libmaus::autoarray::AutoArray<unsigned int> AA(n,false); unsigned int * const A = AA.get();
	for ( unsigned int i = 0; i < n; ++i ) A[i] = rand() % 256;
	
	for ( unsigned int z = 0; z < it; ++z )
	{
		::libmaus::wavelet::WaveletTree< ::libmaus::rank::ERank222B, unsigned int > W(A,n);
		
		for ( unsigned int i = 0; i < n; ++i )
			for ( unsigned int j = i; j < n; ++j )
			{
				std::map<unsigned int,unsigned int> M;
				for ( unsigned k = i; k < j; ++k )
					M [ A[k] ]++;

				::libmaus::autoarray::AutoArray< std::pair < unsigned int, uint64_t > > N = W.rangeCount(i,j);
				std::map<unsigned int, unsigned int> M2;
				for ( unsigned int k = 0; k < N.getN(); ++k )
					M2 [ N[k].first ] = N[k].second;
				
				if ( M != M2 )
				{
					std::cerr << "Failure, A=";
					
					for ( unsigned int k = 0; k < n; ++k )
						std::cerr << A[k] << ";";
					std::cerr  << " [" << i << "," << j << ")" << std::endl;
					
					std::cerr << "M=";
					for ( std::map<unsigned int,unsigned int>::const_iterator ita = M.begin(); ita != M.end(); ++ita )
						std::cerr << "(" << ita->first << "," << ita->second << ")";
					std::cerr << std::endl;
					std::cerr << "M2=";
					for ( std::map<unsigned int,unsigned int>::const_iterator ita = M2.begin(); ita != M2.end(); ++ita )
						std::cerr << "(" << ita->first << "," << ita->second << ")";
						
					std::cerr << " " << N.getN() << std::endl;
					std::cerr << std::endl;
				
					assert ( M == M2 );
				}
			}
	}
	
	return true;
}

void checkRandomRnvGeneric()
{
	for ( unsigned int zz = 0; zz < 1000; ++zz )
	{
		unsigned int const n = 20;
		std::vector < unsigned int > VV(n);
		for ( unsigned int i = 0; i < n; ++i )
			VV[i] = rand() % 64;
		::libmaus::wavelet::WaveletTree< ::libmaus::rank::ERank222B, uint64_t > W(VV.begin(),n);
		
		for ( unsigned int i = 0; i < n; ++i )
			for ( unsigned int j = i; j < n; ++j )
			{
				for ( unsigned int k = 0; k < (1ull<<W.getB()); ++k )		
				{
					uint64_t val = W.rnvGeneric(i,j,k);
					
					uint64_t valval = std::numeric_limits<uint64_t>::max();
					for ( unsigned int z = i; z < j; ++z )
						if ( W[z] >= k && W[z] < valval )
							valval = W[z];
					
					// std::cerr << "k=" << k << " val=" << val << " valval=" << valval << std::endl;
					assert ( val == valval );
				}
			}
	}
	
	std::cerr << "Random checks  for rnvGeneric ok." << std::endl;

}

void testEnumerateSymbols(unsigned int const n, unsigned int const cnt)
{
	::libmaus::autoarray::AutoArray<unsigned int> AA(n,false); unsigned int * const A = AA.get();
		
	// iterate over all permutations of length n
	for ( uint64_t zzz = 0; zzz < cnt; ++zzz )
	{
		for ( unsigned int i = 0; i < n; ++i ) A[i] = rand() % 16;
		typedef ::libmaus::wavelet::WaveletTree< ::libmaus::rank::ERank222B, unsigned int > tree_type;
		tree_type W(A,n);
		typedef tree_type::symbol_type symbol_type;

		for ( unsigned int x = 0; x <= n; ++x )
			for ( unsigned int y = x; y <= n; ++y )
			{
				std::map < symbol_type, uint64_t > slow = W.enumerateSymbolsInRangeSlow(x,y);
				std::map < symbol_type, uint64_t > fast = W.enumerateSymbolsInRange(x,y);
				assert ( slow == fast );
			}
	}	
}

#include <libmaus/random/Random.hpp>
#include <libmaus/timing/RealTimeClock.hpp>

void testCacheLineRank()
{
	// srand(time(0));
	srand(32);
	
	for ( uint64_t z = 0; z < 1; ++z )
	{
		//typedef ::libmaus::rank::CacheLineRank8 clr_type;
		typedef ::libmaus::rank::ImpCacheLineRank clr_type;
		// clr_type CLR(1024ull*1024ull*1024ull);
		//uint64_t const gb = 4ull;
		//uint64_t const mb = gb*1024ull;
		uint64_t const mb = 8;
		uint64_t const bb = mb*1024ull*1024ull;
		uint64_t const bits = bb * 8ull;
		clr_type CLR(bits);
		
		std::cerr << "Filling vector...";
		::libmaus::timing::RealTimeClock frtc;
		frtc.start();
		clr_type::WriteContext wcontext = CLR.getWriteContext();
		for ( uint64_t i = 0; i < CLR.n/16; ++i )
		{
			uint16_t word = rand() & 0xFFFF;
			for ( uint64_t j = 0; j < 16; ++j, word>>=1 )
				wcontext.writeBit(word&1);
			
			if ( (i & (16*1024*1024-1)) == 0 )
				std::cerr << "(" << static_cast<double>(i+1)/(CLR.n/16) << ")";
		}
		wcontext.flush();
		std::cerr << "done, time " << (CLR.n / frtc.getElapsedSeconds())/(1024ull*1024ull) << "M/s" << std::endl;
		::libmaus::rank::ImpCacheLineRankSelectAdapter IA(CLR);

		std::cerr << "Copying vector...";
		frtc.start();
		::libmaus::autoarray::AutoArray<uint64_t> D( (CLR.n+63)/64, false );
		for ( uint64_t i = 0; i < CLR.n; ++i )
		{
			::libmaus::bitio::putBit(D.get(),i,CLR[i]);

			if ( ( i & (16*1024*1024-1) ) == 0 )
				std::cerr << "(" << static_cast<double>(i+1)/CLR.n << ")";
		}
		std::cerr << "done, time " << (CLR.n / frtc.getElapsedSeconds())/(1024ull*1024ull) << "M/s" << std::endl;
		
		std::cerr << "Checking vector...";
		frtc.start();
		for ( uint64_t i = 0; i < CLR.n; ++i )
		{
			assert ( CLR[i]  ==  ::libmaus::bitio::getBit(D.get(),i) );

			if ( ( i & (16*1024*1024-1) ) == 0 )
				std::cerr << "(" << static_cast<double>(i+1)/CLR.n << ")";
		}
		std::cerr << "done, time " << (CLR.n / frtc.getElapsedSeconds())/(1024ull*1024ull) << "M/s" << std::endl;
		
		std::cerr << "Checking external...";
		std::ostringstream eostr;
		::libmaus::rank::ImpCacheLineRank::WriteContextExternal::unique_ptr_type wce =
			::libmaus::rank::ImpCacheLineRank::WriteContextExternal::construct(eostr,CLR.n);
		for ( uint64_t i = 0; i < CLR.n; ++i )
			wce->writeBit(CLR[i]);
		wce->flush();
		std::istringstream eistr(eostr.str());
		::libmaus::rank::ImpCacheLineRank EICLR(eistr);
		assert ( EICLR.n == CLR.n );
		for ( uint64_t i = 0; i < CLR.n; ++i )
		{
			assert ( EICLR[i] == CLR[i] );
			assert ( EICLR.rank1(i) == CLR.rank1(i) );
		}
		std::cerr << "done." << std::endl;
		
		#define CACHECHECKSELECT
		
		std::cerr << "Checking rank/select...";
		uint64_t s = 0;
		uint64_t s0 = 0;
		for ( uint64_t i = 0; i < CLR.n; ++i )
		{
			uint64_t const b = CLR[i] ? 1 : 0;
			#if defined(CACHECHECKSELECT)
			if ( b )
			{
				assert ( CLR.select1(s) == i );
				assert ( IA.select1(s) == i );
				// assert ( CLR.select1(s) == CLR.select1Fast(s) );
			}
			else
				assert ( CLR.select0(s0) == i );
			#endif
			s += b;
			s0 += 1-b;
			
			bool const rok = (CLR.rank1(i) == s);
			if ( ! rok )
			{
				std::cerr << "Failure for i=" << i << " expected " << s << " got " << CLR.rank1(i) << std::endl;
				assert ( rok );
			}
			bool const rok0 = (CLR.rank0(i) == s0);
			if ( ! rok0 )
			{
				std::cerr << "Failure for i=" << i << " expected " << s0 << " got " << CLR.rank0(i) << std::endl;
				assert ( rok0 );
			}
			
			if ( ( i & (16*1024*1024-1) ) == 0 )
				std::cerr << "(" << static_cast<double>(i+1)/CLR.n << ")";
		}
		std::cerr << "done." << std::endl;

		std::cerr << "Setting up erank...";
		frtc.start();
		::libmaus::rank::ERank222B erank(D.get(), D.size()*64);
		std::cerr << "done, time " << (CLR.n / frtc.getElapsedSeconds())/(1024ull*1024ull) << "M/s" << std::endl;
		
		std::cerr << "Setting up random probe vector...";
		frtc.start();
		uint64_t const rankprobes = 16*1024*1024;
		::libmaus::autoarray::AutoArray<uint64_t> P(rankprobes,false);
		for ( uint64_t i = 0; i < rankprobes; ++i )
			P [ i ] = ::libmaus::random::Random::rand64() % CLR.n;
		std::cerr << "done, time " << (rankprobes / frtc.getElapsedSeconds())/(1024ull*1024ull) << "M/s" << std::endl;

		unsigned int const loops = 50;			

		std::cerr << "Running probes on CLR...";
		::libmaus::timing::RealTimeClock rtc; 
		rtc.start();
		uint64_t v = 0;
		for ( unsigned int l = 0; l < loops; ++l )
		{
			std::cerr << "(" << l+1;
			for ( uint64_t i = 0; i < P.size(); ++i )
			{
				v = CLR.rank1(P[i]);
			}
			std::cerr << ")";
		}
		double const CLRtime = rtc.getElapsedSeconds();
		std::cerr << "done." << std::endl;

		std::cerr << "Running probes on erank...";
		rtc.start();
		for ( unsigned int l = 0; l < loops; ++l )
		{
			std::cerr << "(" << l+1;
			for ( uint64_t i = 0; i < P.size(); ++i )
			{
				v = erank.rank1(P[i]);
			}
			std::cerr << ")";
		}
		double const eranktime = rtc.getElapsedSeconds();
		std::cerr << "done." << std::endl;
		
		uint64_t const queries = loops * P.size();
		std::cerr << "mega queries/s: CLR=" << (queries/CLRtime)/(1024.0*1024.0) << " erank=" << (queries/eranktime)/(1024.0*1024.0) << " v=" << v << std::endl;			
	}
	std::cerr << std::endl;
}

void testRnvGenericQuick()
{
	unsigned int const V[] = { 1,1,1,4,4,3,3,5,3,7,6,6,6,6,6,6,6,6,6,6,6,6,6,6};
	unsigned int const n = sizeof(V)/sizeof(V[0]);
	::libmaus::wavelet::QuickWaveletTree< ::libmaus::rank::ERank222B, uint64_t > W(V,n);

	for ( unsigned int i = 0; i < n; ++i )
		for ( unsigned int j = i; j < n; ++j )
		{
			for ( unsigned int k = 0; k < (1ull<<W.getB()); ++k )		
			{
				uint64_t val = W.rnvGeneric(i,j,k);
				
				uint64_t valval = std::numeric_limits<uint64_t>::max();
				for ( unsigned int z = i; z < j; ++z )
					if ( W[z] >= k && W[z] < valval )
						valval = W[z];
				
				if ( val != valval )
					std::cerr << "k=" << k << " val=" << val << " valval=" << valval << std::endl;
				assert ( val == valval );
			}
		}
}

void testRangeCountMax()
{
	unsigned int const V[] = { 1,1,1,3,3,3,7,6,6,6,6,6,6,6,6,6,6,6,6,6,6};
	unsigned int const n = sizeof(V)/sizeof(V[0]);
	::libmaus::wavelet::WaveletTree< ::libmaus::rank::ERank222B, unsigned int > W(V,n);
	std::vector < std::pair<unsigned int,uint64_t> > U = W.rangeCountMax(0,n,2);
	
	for ( unsigned int i = 0; i < U.size(); ++i )
	{
		std::cerr << U[i].first << "\t" << U[i].second << std::endl;
	}
}

void testAccessPermutationsQuick()
{
	std::cerr << "Testing access on permutations (QuickWT)...";
	for ( uint64_t i = 0; i <= 10; ++i )
	{
		std::cerr << "(" << i;
		checkAccessPermutationsQuick(i);
		std::cerr << ")";
	}
	std::cerr << "done." << std::endl;
}

void testRMQPermutationsQuick()
{
	clock_t bef_quick_rmq = clock();
	bool quickrmqok = true;
	for ( unsigned int i = 0; quickrmqok && i < 10; ++i )
	{
		std::cerr << "checking rmq for all permutations of length " << i << "... (Quick)";
		quickrmqok = quickrmqok && checkRmqPermutationsQuick(i);
		std::cerr << "done." << std::endl;
	}
	std::cerr << "Quick RMQ tests: " << (quickrmqok?"ok":"failed") << " clocks " << (clock()-bef_quick_rmq) << std::endl;
}

void testEnumSymbols()
{
	std::cerr << "Testing symbol enumeration...";	
	testEnumerateSymbols ( 16 , 32 );
	std::cerr << "done." << std::endl;
}

#include <libmaus/lf/LF.hpp>

void testMultiRankCacheLF()
{
	srand(time(0));
	// std::string const s = "fischersfritzefischtfrischefische";
	::libmaus::autoarray::AutoArray<uint8_t> s(64*1024+11,false);
	for ( uint64_t i = 0; i < s.size(); ++i )
		s[i] = rand() & 7;
	
	::libmaus::lf::MultiRankCacheLF MRCL(s.begin(),s.size());
	
	std::map<uint64_t,uint64_t> R;
	for ( uint64_t i = 0; i < s.size(); ++i )
	{
		if ( R.find(s[i]) == R.end() )
			R[s[i]] = 0;

		assert ( MRCL[i] == s[i] );
		assert ( R.find(s[i])->second == MRCL.rankm1(s[i],i) );
		R[s[i]]++;
	}
}

#include <libmaus/rmq/CartesianTree.hpp>

template<typename value_type>
std::ostream & printBitVector(std::ostream & out, value_type const v)
{
	for ( value_type mask = 1ull << (8*sizeof(v)-1); mask; mask >>= 1)
		out << (((v&mask)!=0) ? '(' : ')');
	out << std::endl;
	uint64_t i = 0;
	for ( value_type mask = 1ull << (8*sizeof(v)-1); mask; mask >>= 1, i++)
		out << (i%10);
	out << std::endl;
	return out;
}

void checkBPS()
{
	#if 0
	// ::libmaus::bitio::BitVector::testLSB();
	uint8_t const v = 16*1+8+4;
	printBitVector(std::cerr,v);
	BalancedParenthesesBase BP;
	std::cerr << "findClose(" << 4 << ")=" 
		<< static_cast<int>(BalancedParenthesesBase::singleFindClose(v,4)) << "\t" << BP.wordClose(v,4)
		<< "\t" << BalancedParenthesesBase::naiveFindClose(&v,4)
		<< std::endl;
	std::cerr << "findOpen(" << 7 << ")=" 
		<< static_cast<int>(BalancedParenthesesBase::singleFindOpen(v,7)) << "\t" 
		<< BP.wordOpen(v,7) 
		<< "\t"
		<< BalancedParenthesesBase::naiveFindOpen(&v,7)
		<< std::endl;
	std::cerr << "findOpen(" << 6 << ")=" << static_cast<int>(BalancedParenthesesBase::singleFindOpen(v,6)) << "\t" << BP.wordOpen(v,6) << std::endl;
	std::cerr << "findClose(" << 3 << ")=" << static_cast<int>(BalancedParenthesesBase::singleFindClose(v,3)) << "\t" << BP.wordClose(v,3) << std::endl;
	#endif

	// set up random cartesian tree of size 1M		
	::libmaus::autoarray::AutoArray<uint64_t> A(1024*1024);
	srand(time(0));
	for ( uint64_t i = 0; i < A.size(); ++i )
		A[i] = ::libmaus::random::Random::rand64();
	uint64_t const an = A.size();
	::libmaus::rmq::CartesianTree<uint64_t> CT(A.begin(),an);
	
	::libmaus::bitio::IndexedBitVector::unique_ptr_type PUUB = CT.bpsVector();
	BalancedParentheses BP(PUUB);
}

#include <libmaus/rank/RunLengthBitVectorStream.hpp>
#include <libmaus/rank/RunLengthBitVector.hpp>
#include <libmaus/rank/RunLengthBitVectorGenerator.hpp>

void testrl(std::vector<bool> const & B)
{
	
	std::stringstream ostr;
	std::stringstream indexstr;
	libmaus::rank::RunLengthBitVectorGenerator RLBVG(ostr,indexstr,B.size(),256);
	
	for ( uint64_t i = 0; i < B.size(); ++i )
		RLBVG.putbit(B[i]);
	uint64_t const size = RLBVG.flush();
	
	assert ( size == ostr.str().size() );
	// std::cerr << "size=" << size << " str=" << ostr.str().size() << std::endl;
	
	#if 1
	{
		ostr.clear();
		ostr.seekg(0,std::ios::beg);
		libmaus::rank::RunLengthBitVectorStream RLBV(ostr);
		
		if ( RLBV.n != B.size() )
		{
			std::cerr << B.size() << "\t" << RLBV.n << std::endl;
		}
		
		assert ( RLBV.n == B.size() );
		
		uint64_t r1 = 0;
		for ( uint64_t i = 0; i < B.size(); ++i )
		{
			assert ( r1 == RLBV.rankm1(i) );
			if ( B[i] )
				r1++;
			// std::cerr << i << "\t" << B[i] << "\t" << RLBV.get(i) << "\t" << r1 << "\t" << RLBV.rank1(i) << std::endl;
			assert ( B[i] == RLBV[i] );
			assert ( r1 == RLBV.rank1(i) );
		}
	}
	#endif

	{
		ostr.clear();
		ostr.seekg(0,std::ios::beg);
		libmaus::rank::RunLengthBitVector RLBVint(ostr);

		uint64_t r1 = 0;
		for ( uint64_t i = 0; i < B.size(); ++i )
		{
			assert ( r1 == RLBVint.rankm1(i) );
			if ( B[i] )
				r1++;
			// std::cerr << i << "\t" << B[i] << "\t" << RLBV.get(i) << "\t" << r1 << "\t" << RLBV.rank1(i) << std::endl;
			assert ( B[i] == RLBVint[i] );
			assert ( r1 == RLBVint.rank1(i) );
			unsigned int sym;
			
			assert ( r1 == RLBVint.inverseSelect1(i,sym) );
		}
	}
}

void testrl()
{
	{
	std::vector<bool> B;
	B.push_back(0);
	B.push_back(1);
	B.push_back(0);
	B.push_back(0);
	B.push_back(1);
	B.push_back(1);
	B.push_back(1);
	B.push_back(0);
	B.push_back(0);
	B.push_back(1);
	B.push_back(1);
	
	testrl(B);
	}
	
	{
		srand(5);
		for ( uint64_t z = 0; z < 1024; ++z )
		{
			std::vector<bool> B;
			uint64_t const r = (rand() % (32*1024))+1;
			std::cerr << "r=" << r << std::endl;
			for ( uint64_t i = 0; i < r; ++i )
				B.push_back(rand() & 1);
			testrl(B);
		}
	
	}
}

int main()
{
	initRand();
	
	{
		char s[] = { 0,1,1,0,1,1,2,0,1,0,1,1,2,1,0,0,1,0,0,0,1,1,1,0};
		uint64_t const n = sizeof(s)/sizeof(s[0]);
		
		{
			libmaus::rank::FastRank<uint8_t,uint32_t> R(&s[0],n);
			std::map<char,uint64_t> M;
			for ( uint64_t i = 0; i < n; ++i )
			{
				M[s[i]]++;
				for ( uint64_t j = 0; j < 256; ++j )
				{
					int64_t const expect = M[j];
					int64_t const got = R.rank(j,i);
					
					if ( expect != got )
					{
						std::cerr << "i=" << i << " j=" << j << " expected " << expect << " got " << got << std::endl;
						assert ( expect == got );
					}
				}
			}
		}
		{
			libmaus::rank::FastRank<uint8_t,uint32_t> R(&s[0],n,2);
			std::map<char,uint64_t> M;
			for ( uint64_t i = 0; i < n; ++i )
			{
				M[s[i]]++;
				for ( uint64_t j = 0; j < 256; ++j )
				{
					int64_t const expect = M[j];
					int64_t const got = R.rank(j,i);
					
					if ( expect != got )
					{
						std::cerr << "i=" << i << " j=" << j << " expected " << expect << " got " << got << std::endl;
						assert ( expect == got );
					}
				}
			}
		}
	}
	
	exit(0);
	
	testrl();
	checkE2Append(1024*1024);
	callWaveletTreeRankSelectRandom(128);
	waveletTreeSmallerLargerRandom(10);
	testCacheLineRank();
	testRnvGenericQuick();
	testAccessPermutationsQuick();
	testEnumSymbols();
	waveletTreeRankSelect();
	callWaveletTreeRankSelectRandom(16*1024);

	waveletTreeRankSelect();
	waveletTreeCheckRMQ();

	checkBPS();
	checkStreams8(1000000);
	checkStreams64(10000000);

	testCacheLineRank();
	checkRank();
	

	for ( unsigned int n = 1; n < 10; ++n )
		checkRangeCountPermutations ( n );
	
	checkRangeCountRand ( 20, 1000 );
		
	for ( unsigned int i = 0; i < 256; ++i )
		checkSortedSymbols(31);

	std::cerr << "Checking wavelet tree rank/select with random sequences" << std::endl;
	

	clock_t bef_rmq = clock();
	bool rmqok = true;
	for ( unsigned int i = 0; rmqok && i < 10; ++i )
	{
		std::cerr << "checking rmq for all permutations of length " << i << "...";
		rmqok = rmqok && checkRmqPermutations(i);
		std::cerr << "done." << std::endl;
	}
	std::cerr << "RMQ tests: " << (rmqok?"ok":"failed") << " clocks " << (clock()-bef_rmq) << std::endl;

	for ( unsigned int i = 0; i <= 8; ++i )
	{
		double const cps = 1.0 / CLOCKS_PER_SEC;
		std::cerr << "Checking QuickWaveletTree::rpv for all permutations of length " << i << "...";
		clock_t bef = clock();
		checkRPVPermutationsQuick(i);
		clock_t aft = clock();
		std::cerr << "done." << " clocks " << (aft-bef)*cps << std::endl;
	}
	for ( unsigned int i = 0; i <= 8; ++i )
	{
		double const cps = 1.0 / CLOCKS_PER_SEC;
		std::cerr << "Checking WaveletTree::rpv for all permutations of length " << i << "...";
		clock_t bef = clock();
		checkRPVPermutations(i);
		clock_t aft = clock();
		std::cerr << "done." << " clocks " << (aft-bef)*cps << std::endl;
	}

	clock_t bef_rpv = clock();
	bool rpvok = true;
	for ( unsigned int i = 0; rpvok && i < 10; ++i )
	{
		std::cerr << "checking rpv for all permutations of length " << i << "...";
		rpvok = rpvok && checkRPVPermutations(i);
		std::cerr << "done." << std::endl;
	}
	std::cerr << "RPV tests: " << (rpvok?"ok":"failed") << " clocks " << (clock()-bef_rpv) << std::endl;

	clock_t bef_rnv = clock();
	bool rnvok = true;
	for ( unsigned int i = 0; rnvok && i < 10; ++i )
	{
		std::cerr << "checking rnv for all permutations of length " << i << "...";
		rnvok = rnvok && checkRNVPermutations(i);
		std::cerr << "done." << std::endl;
	}
	std::cerr << "RNV tests: " << (rnvok?"ok":"failed") << " clocks " << (clock()-bef_rnv) << std::endl;

	checkRandomRnvGeneric();

}
