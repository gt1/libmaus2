/*
    libmaus2
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

#include <libmaus2/bitio/BitVectorInput.hpp>
#include <libmaus2/bitio/BitVectorOutput.hpp>

#include <libmaus2/bitio/CompactArray.hpp>
#include <libmaus2/bitio/SwapWordBitBlock.hpp>
#include <libmaus2/bitio/CompactSparseArray.hpp>
#include <libmaus2/bitio/CompactOffsetArray.hpp>
#include <libmaus2/bitio/CompactOffsetOneArray.hpp>
#include <libmaus2/bitio/getBits.hpp>
#include <libmaus2/bitio/swapBitBlocksByReversal.hpp>
#include <libmaus2/bitio/BitVector.hpp>
#include <cstdlib>
#include <ctime>

template<typename value_type>
std::vector<bool> arrayToBitVector(value_type const * A, uint64_t const numwords)
{
	uint64_t const bitsperval = 8*sizeof(value_type);
	uint64_t const totalbits = numwords * bitsperval;
	uint64_t p = 0;
	std::vector<bool> B;
	B.resize(totalbits);
	
	for ( unsigned int i = 0; i < numwords; ++i )
	{
		value_type const v = A[i];
		for ( 
			value_type mask = static_cast<value_type>(1ul) << (bitsperval-1);
			mask;
			mask >>= 1
		)
			B[p++] = ((v & mask) != 0);
	}
	
	return B;
}

void putBitsVector(std::vector<bool> & B, uint64_t offset, unsigned int numbits, uint64_t v)
{
	uint64_t mask = static_cast<uint64_t>(1ull) << (numbits-1);
	
	for ( unsigned int i = 0; i < numbits; ++i )
		B[offset + i] = (v & (mask>>i)) != 0;
}

#include <iostream>

uint64_t rand64()
{
	uint64_t u[8];
	for ( uint64_t i = 0; i < sizeof(u)/sizeof(u[0]); ++i )
		u[i] = rand() & 0xFF;
	uint64_t v = 0;
	for ( uint64_t i = 0; i < sizeof(u)/sizeof(u[0]); ++i )
	{
		v <<= 8;
		v |= u[i];
	}
	return v;
}

void testGetPut()
{
	std::cerr << "Testing getBits/putBits..." << std::endl;

	typedef uint16_t Atype;
	unsigned int const vecsize = 10;
	Atype A[vecsize];
	for ( uint64_t i = 0; i < sizeof(A)/sizeof(A[0]); ++i )
		A[i] = 0;

	srand(time(0));

	uint64_t bitops = 0;
	for ( unsigned int numbits = 0; numbits <= 32; ++numbits )
	{
		std::cerr << "numbits=" << numbits << std::endl;
		for ( unsigned int offset = 0; offset < 33; ++offset )	
		{
			for ( unsigned int i = 0; i < 10000; ++i )
			{
				unsigned int value = rand();
				value &= libmaus2::math::lowbits(numbits);
				
				std::vector<bool> B0 = arrayToBitVector(&A[0],vecsize);
				putBitsVector(B0,offset,numbits,value);
				
				libmaus2::bitio::putBits(&A[0], offset, numbits, value);
			
				assert ( libmaus2::bitio::getBits(&A[0], offset, numbits) == value );

				std::vector<bool> B1 = arrayToBitVector(&A[0],vecsize);
				assert ( B0 == B1 );
				
				bitops++;
			}
		}
	}
	
	std::cerr << "bitops " << static_cast<double>(bitops) << std::endl;
}

void testGetPut64()
{
	std::cerr << "Testing 64 getBits/putBits..." << std::endl;

	typedef uint64_t Atype;
	unsigned int const vecsize = 10;
	Atype A[vecsize];
	for ( uint64_t i = 0; i < sizeof(A)/sizeof(A[0]); ++i )
		A[i] = 0;

	srand(time(0));

	uint64_t bitops = 0;
	for ( unsigned int numbits = 0; numbits <= 64; ++numbits )
	{
		std::cerr << "numbits=" << numbits << std::endl;
		for ( unsigned int offset = 0; offset <= 64; ++offset )	
		{
			for ( unsigned int i = 0; i < 10000; ++i )
			{
				uint64_t value = rand64();
				value &= libmaus2::math::lowbits(numbits);

				std::vector<bool> B0 = arrayToBitVector(&A[0],vecsize);
				putBitsVector(B0,offset,numbits,value);
				
				libmaus2::bitio::putBits(&A[0], offset, numbits, value);
			
				assert ( libmaus2::bitio::getBits(&A[0], offset, numbits) == value );

				std::vector<bool> B1 = arrayToBitVector(&A[0],vecsize);
				assert ( B0 == B1 );
				
				bitops++;
			}
		}
	}
	
	std::cerr << "bitops " << static_cast<double>(bitops) << std::endl;
}

void sortCheck()
{
#if defined(__GNUC__) && ( __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 8) ) && (!defined(__clang__))
	std::cerr << "Testing sort using CompactArrayIterator...";
	
	srand(time(0));

	for ( unsigned int i = 0; i < 1000; ++i )
	{
		unsigned int n = (rand() % 1024) + 1;
		unsigned int b = (rand() % 26) /* + 1 */;
		libmaus2::bitio::CompactArray C(n,b);
		std::vector<uint64_t> V;
		
		for ( unsigned int i = 0; i < C.n; ++i )
		{
			uint64_t v = rand() % (1ull << C.getB());
			C.set(i, v);
			V.push_back(v);
		}

		#if 0
		std::cerr << "Pre ";
		for ( unsigned int i = 0; i < C.n; ++i )
			std::cerr << C.get(i) << ";";
		std::cerr << std::endl;
		#endif

		libmaus2::bitio::CompactArray::iterator ita(&C);
		libmaus2::bitio::CompactArray::iterator itb(&C); itb += C.n;
		std::sort ( ita, itb );
		std::sort ( V.begin(), V.end() );
		
		#if 0
		std::cerr << "Post ";
		for ( unsigned int i = 0; i < C.n; ++i )
			std::cerr << C.get(i) << ";";
		std::cerr << std::endl;
		#endif
		
		for ( unsigned int i = 1; i < C.n; ++i )
			assert ( C.get(i-1) <= C.get(i) );
		for ( unsigned int i = 0; i < C.n; ++i )
			assert ( C.get(i) == V[i] );
	}	
	std::cerr << "done." << std::endl;
#endif
}

void bubbleSortCheck()
{
	srand(time(0));

	for ( unsigned int i = 0; i < 1000; ++i )
	{
		unsigned int n = (rand() % 1024) + 1;
		unsigned int b = (rand() % 26) /* + 1 */;
		libmaus2::bitio::CompactArray C(n,b);
		std::vector<uint64_t> V;
		
		for ( unsigned int i = 0; i < C.n; ++i )
		{
			uint64_t v = rand() % (1ull << C.getB());
			C.set(i, v);
			V.push_back(v);
		}

		#if 0
		std::cerr << "Pre ";
		for ( unsigned int i = 0; i < C.n; ++i )
			std::cerr << C.get(i) << ";";
		std::cerr << std::endl;
		#endif

		if ( n > 1 )
		{
			bool sorted;
			
			do
			{
				sorted = true;
				libmaus2::bitio::CompactArray::iterator ita(&C);
				libmaus2::bitio::CompactArray::iterator itb(&C); itb += 1;
				
				for ( uint64_t j = 0; j < n-1; ++j, ++ita, ++itb )
				{
					uint64_t v0 = *ita;
					uint64_t v1 = *itb;
					
					if ( v0 > v1 )
					{
						sorted = false;
						std::swap(v0,v1);
					}
					
					(*ita) = v0;
					(*itb) = v1;
					
					assert ( (*ita) <= (*itb) );
				}
			} while ( ! sorted );
		}

		std::sort ( V.begin(), V.end() );
		
		#if 0
		std::cerr << "Post ";
		for ( unsigned int i = 0; i < C.n; ++i )
			std::cerr << C.get(i) << ";";
		std::cerr << std::endl;
		#endif
		
		for ( unsigned int i = 1; i < C.n; ++i )
			assert ( C.get(i-1) <= C.get(i) );
		for ( unsigned int i = 0; i < C.n; ++i )
			assert ( C.get(i) == V[i] );
	}	
}

void stableSortCheck()
{
#if defined(__GNUC__) && ( (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 5)) && (! defined(__clang__)) )
	srand(time(0));

	for ( unsigned int i = 0; i < 1000; ++i )
	{
		unsigned int n = (rand() % 1024) + 1;
		unsigned int b = (rand() % 26) + 1;
		libmaus2::bitio::CompactArray C(n,b);
		
		for ( unsigned int i = 0; i < C.n; ++i )
			C.set(i, rand() % (1ull << C.getB()) );

		#if 0
		std::cerr << "Pre ";
		for ( unsigned int i = 0; i < C.n; ++i )
			std::cerr << C.get(i) << ";";
		std::cerr << std::endl;
		#endif

		libmaus2::bitio::CompactArray::iterator ita(&C);
		libmaus2::bitio::CompactArray::iterator itb(&C); itb += C.n;
		std::stable_sort ( ita, itb );
		
		#if 0
		std::cerr << "Post ";
		for ( unsigned int i = 0; i < C.n; ++i )
			std::cerr << C.get(i) << ";";
		std::cerr << std::endl;
		#endif
		
		for ( unsigned int i = 1; i < C.n; ++i )
			assert ( C.get(i-1) <= C.get(i) );
	}	
#endif
}

void sortSparseCheck()
{
#if defined(__GNUC__) && (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 8))  && (! defined(__clang__))
	srand(time(0));

	for ( unsigned int i = 0; i < 1000; ++i )
	{
		#if 0
		unsigned int n = (rand() % 1024) + 1;
		unsigned int b = (rand() % 26) + 1;
		#endif
		
		unsigned int n = (rand() % 16) + 1;
		unsigned int b = (rand() % 4) + 1;

		unsigned int k = b + (rand() % 16);
		unsigned int o = k - b;
		
		// std::cerr << "b=" << b << " k=" << k << " o=" << o << std::endl;
		
		libmaus2::bitio::CompactArray D(n,k);
		libmaus2::bitio::CompactSparseArray C(D.D,n,b,o,k);
		
		for ( unsigned int i = 0; i < C.n; ++i )
		{
			uint64_t v = rand() % (1ull << C.b);
			C.set(i, v );
			assert ( C.get(i) == v );
		}

		#if 0
		std::cerr << "Pre ";
		for ( unsigned int i = 0; i < C.n; ++i )
			std::cerr << C.get(i) << ";";
		std::cerr << std::endl;
		#endif

		libmaus2::bitio::CompactSparseArray::iterator ita = C.begin();
		libmaus2::bitio::CompactSparseArray::iterator itb = C.end();
		std::sort ( ita, itb );
		
		#if 0
		std::cerr << "Post ";
		for ( unsigned int i = 0; i < C.n; ++i )
			std::cerr << C.get(i) << ";";
		std::cerr << std::endl;
		#endif
		
		for ( unsigned int i = 1; i < C.n; ++i )
			assert ( C.get(i-1) <= C.get(i) );
	}	
#endif
}

void sortStableSparseCheck()
{
#if (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 5)) && (! defined(__clang__))
	srand(time(0));

	for ( unsigned int i = 0; i < 1000; ++i )
	{
		unsigned int n = (rand() % 1024) + 1;
		unsigned int b = (rand() % 26) + 1;

		unsigned int k = b + (rand() % 16);
		unsigned int o = k - b;
		
		// std::cerr << "b=" << b << " k=" << k << " o=" << o << std::endl;
		
		libmaus2::bitio::CompactArray D(n,k);
		libmaus2::bitio::CompactSparseArray C(D.D,n,b,o,k);
		
		for ( unsigned int i = 0; i < C.n; ++i )
		{
			uint64_t v = rand() % (1ull << C.b);
			C.set(i, v );
			assert ( C.get(i) == v );
		}

		#if 0
		std::cerr << "Pre ";
		for ( unsigned int i = 0; i < C.n; ++i )
			std::cerr << C.get(i) << ";";
		std::cerr << std::endl;
		#endif

		libmaus2::bitio::CompactSparseArray::iterator ita = C.begin();
		libmaus2::bitio::CompactSparseArray::iterator itb = C.end();
		std::stable_sort ( ita, itb );
		
		#if 0
		std::cerr << "Post ";
		for ( unsigned int i = 0; i < C.n; ++i )
			std::cerr << C.get(i) << ";";
		std::cerr << std::endl;
		#endif
		
		for ( unsigned int i = 1; i < C.n; ++i )
			assert ( C.get(i-1) <= C.get(i) );
	}	
#endif
}

void testBlockSwap()
{
	std::cerr << "Testing bit block swaps...";

	uint64_t const numwords = 32;
	uint64_t const numbits = 8*sizeof(uint64_t)*numwords;
	
	libmaus2::autoarray::AutoArray<uint64_t> A0(numwords);
	libmaus2::autoarray::AutoArray<uint64_t> A1(numwords);
	
	for ( uint64_t z = 0; z < 100000; ++z )
	{
		for ( uint64_t i = 0; i < numwords; ++i )
			A0[i] = A1[i] = rand64();
	
		uint64_t const offset = rand64() % (numbits+1);
		uint64_t const ll = numbits-offset;
		uint64_t const l0 = rand64() % (ll+1);
		uint64_t const lr = numbits-(offset+l0);
		uint64_t const l1 = rand64() % (lr+1);
					
		/* std::cerr << "numbits=" << numbits << " offset=" << offset << " l0=" << l0 << " l1=" << l1 << std::endl; */

		libmaus2::bitio::swapBitBlocksByReversal(A0.get(), offset, l0, l1);
		libmaus2::bitio::swapBitBlocks(A1.get(), offset, l0, l1);
		
		for ( uint64_t i = 0; i < numwords; ++i )
			assert ( A0[i] == A1[i] );
	}
	
	std::cerr << "done." << std::endl;
}

void testAutoArraySampling()
{
	uint64_t const n = 8*1024;
	::libmaus2::autoarray::AutoArray<uint64_t> A(n);
	for ( uint64_t i = 0; i < n; ++i )
		A[i] = i;

	#if 0
	for ( uint64_t i = 0; i < A.getN(); ++i )
		std::cerr << A[i] << ";";
	std::cerr << std::endl;
	#endif
	
	std::ostringstream ostr;
	A.serialize(ostr);
	
	std::istringstream istr(ostr.str());
	std::ostringstream sostr;
	
	::libmaus2::autoarray::AutoArray<uint64_t>::sampleCopy(istr,sostr,64);
	
	std::istringstream sistr(sostr.str());
	::libmaus2::autoarray::AutoArray<uint64_t> S;
	S.deserialize(sistr);
	
	for ( uint64_t i = 0; i < S.getN(); ++i )
		std::cerr << S[i] << ";";
	std::cerr << std::endl;

}

#include <libmaus2/util/GetFileSize.hpp>
#include <libmaus2/timing/RealTimeClock.hpp>
#include <libmaus2/sorting/SerialRadixSort64.hpp>
#include <libmaus2/sorting/sorting.hpp>

struct Projector
{
	inline uint64_t operator()(uint64_t v) const
	{
		return v;
	}	
};

#if 0
static int cmpuint64(const void *p1, const void *p2)
{
	uint64_t const v1 = *(reinterpret_cast<uint64_t const *>(p1));
	uint64_t const v2 = *(reinterpret_cast<uint64_t const *>(p2));
	
	if ( v1 < v2 )
		return -1;
	else if ( v1 > v2 )
		return 1;
	else
		return 0;
}
#endif

struct UIntProjector
{
	typedef uint64_t value_type;
	
	#if defined(QUICKSORT_DEBUG)
	template<typename type>
	static value_type proj(type & A, unsigned int i)
	{
		return A[i];
	}
	#endif
	
	template<typename type>
	static void swap(type & A, unsigned int i, unsigned int j)
	{
		std::swap(A[i],A[j]);
	}

	template<typename type>
	static bool comp(type & A, unsigned int i, unsigned int j)
	{
		return A[i] < A[j];
	}
};

#include <libmaus2/bitio/MarkerFastWriteBitWriter.hpp>

void testMarkerBitIO()
{
	std::ostringstream ostr;
	std::ostream_iterator<uint8_t> ostrit(ostr);
	libmaus2::bitio::MarkerFastWriteBitWriterStream8 W(ostrit);
	
	uint64_t const n = 16*1024*1024;
	
	for ( uint64_t i = 0; i < n; ++i )
		W.write( i & 0xFF, 8 );
	W.flush();
	
	std::cerr << "size of stream is " << ostr.str().size() << std::endl;
	
	libmaus2::timing::RealTimeClock rtc; rtc.start();
	uint64_t const loop = 32;
	std::istringstream istr(ostr.str());

	for ( uint64_t l = 0; l < loop; ++l )
	{
		istr.clear();
		istr.seekg(0);
		libmaus2::bitio::MarkerStreamBitInputStream R(istr);
	
		for ( uint64_t i = 0; i < n; ++i )
		{
			//std::cout << R.read(8) << std::endl;
			assert ( R.read(8) == (i & 0xFF) );
		}
	}
	
	double const secs = rtc.getElapsedSeconds();
	
	std::cerr << "MarkerStreamBitInputStream decoding speed " << (static_cast<double>(loop*n)/(1024.0*1024.0)) / secs << " MB/s" << std::endl;
}

void testMarkerBitIO4()
{
	std::ostringstream ostr;
	libmaus2::aio::SynchronousGenericOutput<uint32_t> SGO(ostr,8*1024);
	libmaus2::bitio::MarkerFastWriteBitWriterBuffer32Sync W(SGO);
	
	uint64_t const n = 16*1024*1024;
	
	for ( uint64_t i = 0; i < n; ++i )
		W.write( i & 0xFF, 8 );
	W.flush();
	
	std::cerr << "size of stream is " << ostr.str().size() << std::endl;
	
	#if 0
	libmaus2::timing::RealTimeClock rtc; rtc.start();
	uint64_t const loop = 32;
	std::istringstream istr(ostr.str());

	for ( uint64_t l = 0; l < loop; ++l )
	{
		istr.clear();
		istr.seekg(0);
		libmaus2::bitio::MarkerStreamBitInputStream R(istr);
	
		for ( uint64_t i = 0; i < n; ++i )
		{
			//std::cout << R.read(8) << std::endl;
			assert ( R.read(8) == (i & 0xFF) );
		}
	}
	
	double const secs = rtc.getElapsedSeconds();
	
	std::cerr << "MarkerStreamBitInputStream decoding speed " << (static_cast<double>(loop*n)/(1024.0*1024.0)) / secs << " MB/s" << std::endl;
	#endif
}

void testBitVectorIO()
{
	srand(5);
	libmaus2::bitio::BitVectorOutput bout0("t0");
	libmaus2::bitio::BitVectorOutput bout1("t1");

	std::vector<bool> bits;		

	uint64_t n0 = 512;
	for ( uint64_t i = 0; i < n0; ++i )
	{
		bool const bit = rand() % 2 == 1;
		bits.push_back(bit);
		bout0.writeBit(bit);
	}
	bout0.flush();

	uint64_t n1 = 1024;
	for ( uint64_t i = 0; i < n1; ++i )
	{
		bool const bit = rand() % 2 == 1;
		bits.push_back(bit);
		bout1.writeBit(bit);
	}	
	bout1.flush();	
	
	std::vector<std::string> V;
	V.push_back("t0");
	V.push_back("t1");
	libmaus2::bitio::BitVectorInput bin(V);
	
	for ( uint64_t i = 0; i < n0+n1; ++i )
	{
		bool const bit = bin.readBit();
		assert ( bit == bits[i] );
	}
	
	for ( uint64_t offset = 0; offset <= n0+n1; ++offset )
	{
		// std::cerr << "offset=" << offset << std::endl;
		libmaus2::bitio::BitVectorInput bin(V,offset);
		
		for ( uint64_t j = offset; j < n0+n1; ++j )
		{
			bool const bit = bin.readBit();
			assert ( bit == bits[j] );
		}
	}
	
	remove("t0");
	remove("t1");
}
                     
int main()
{
	try
	{
		testBitVectorIO();
		
		#if 0
		testMarkerBitIO4();		
		return 0;
		#endif
		
		testMarkerBitIO();	
		testBlockSwap();
		
		// libmaus2::bitio::CompactArrayBase::printglobaltables();
		
		if ( 0 )
		{
			uint64_t b = 5;
			uint64_t n = 1ull << b;
			uint64_t n2 = n/2;
			libmaus2::bitio::CompactArray C(n+2,b);
			C.set(n,1);
			C.set(n+1,n2);
			
			for ( uint64_t i = 0; i < n/2; ++i )
				C.set ( i, n/2+i );
			for ( uint64_t i = 0; i < n/2; ++i )
				C.set ( n/2+i, n/2-i-1 );
				
			for ( uint64_t i = 0; i < C.n; ++i )
				std::cerr << C.get(i) << ";";
			std::cerr << std::endl;

			libmaus2::bitio::CompactOffsetOneArray O(C.n,b,0/*addoffset*/,C.D);

			for ( uint64_t i = 0; i < O.n; ++i )
				std::cerr << O.get(i) << ";";
			std::cerr << std::endl;
			
			O.mergeSort();

			for ( uint64_t i = 0; i < O.n; ++i )
				std::cerr << O.get(i) << ";";
			std::cerr << std::endl;

			for ( uint64_t i = 0; i < C.n; ++i )
				std::cerr << C.get(i) << ";";
			std::cerr << std::endl;
			
			for ( uint64_t i = 0; i < C.n; ++i )
				std::cerr << libmaus2::bitio::getBit(C.D,i*b);
			std::cerr << std::endl;

			for ( uint64_t i = 0; i < C.n; ++i )
				std::cerr << O.get(i) << ";";
			std::cerr << std::endl;
			
			O.rearrange();

			for ( uint64_t i = 0; i < C.n; ++i )
				std::cerr << libmaus2::bitio::getBit(C.D,i);
			std::cerr << std::endl;

			libmaus2::bitio::CompactOffsetArray O2(n+2,b-1,n+2,C.D);

			for ( uint64_t i = 0; i < C.n; ++i )
				std::cerr << O2.get(i) << ";";
			std::cerr << std::endl;
		}
		uint64_t v = -1;
		libmaus2::bitio::CompactOffsetOneArray C(3,4/*b*/,0/*addoffset*/,&v);
		
		C.set(0,2);
		C.set(2,0);

		for ( uint64_t m = 1ULL<<63; m; m>>=1 )
			std::cerr << ((v&m)!=0);
		std::cerr << std::endl;
		
		assert ( C.get(0) == 2);
		assert ( C.get(2) == 0);

		sortSparseCheck();
		sortStableSparseCheck();
		sortCheck();
		stableSortCheck();
		testGetPut64();
		testGetPut();
		bubbleSortCheck();
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
