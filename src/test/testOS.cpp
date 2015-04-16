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

#include <libmaus2/util/SaturatingCounter.hpp>
#include <libmaus2/select/ESelect222B.hpp>
#include <libmaus2/bitio/CompactArray.hpp>
#include <libmaus2/bitio/SignedCompactArray.hpp>
#include <libmaus2/bitio/BitVector.hpp>
#include <libmaus2/suffixsort/SAISUtils.hpp>
#include <libmaus2/suffixsort/SuccinctFactorList.hpp>
#include <libmaus2/suffixsort/OSInduce.hpp>
#include <libmaus2/suffixsort/SAIS_Mori.hpp>
#include <libmaus2/select/ESelect222B.hpp>
#include <cassert>

template<typename text_type, typename sa_type>
struct ImplicitBWT
{
	text_type const & text;
	sa_type const & SA;
	uint64_t const n;
	
	ImplicitBWT(
		text_type const & rtext,
		sa_type const & rSA,
		uint64_t const rn
		)
	: text(rtext), SA(rSA), n(rn)
	{
	
	}
	
	uint64_t operator[](uint64_t const i) const
	{
		uint64_t const p = SA[i];

		if ( p )
			return text[p-1];
		else
			return text[n-1];
	}
};

template<typename input_array_type>
::libmaus2::bitio::CompactArray::unique_ptr_type bwtOSHash(
	input_array_type const & C, 
	bool const verbose, 
	unsigned int const level = 1
)
{
	uint64_t const n = C.size();
	uint64_t const sentinel = (1ull<<C.getB());
	uint64_t const smallthres = 
		::libmaus2::suffixsort::SuccinctFactorListContext< ::libmaus2::bitio::CompactArray::const_iterator >::computeSmallThresPreLog(n,C.getB());
		
	if ( verbose )
		std::cerr << "\n\n\n(+++bwtOSHash: n=" << n << ",b=" << C.getB() << ")";

	// iterator for symbols		
	typedef typename input_array_type::const_iterator compact_const_it;
	compact_const_it it = C.begin();

	::libmaus2::timing::RealTimeClock rtc;
	if ( verbose )
	{
		std::cerr << "(computing s* positions...";
		rtc.start();
	}

	::libmaus2::bitio::IndexedBitVector::unique_ptr_type pstype = ::libmaus2::suffixsort::computeSast(C.begin(),C.size());
	::libmaus2::bitio::IndexedBitVector & stype = *pstype;
	
	if ( verbose )
	{
		::libmaus2::util::Histogram sdifhist;
		uint64_t lastspos = 0;
		for ( uint64_t i = stype.next1(1); i < n; i = stype.next1(i+1) )
		{
			assert ( stype.get(i) );
			sdifhist ( i - lastspos );
			lastspos = i;
		}
		
		sdifhist.print(std::cerr);
	}

	if ( verbose )
		std::cerr << "time=" << rtc.getElapsedSeconds() << ")" << std::endl;

	::libmaus2::suffixsort::SuccinctFactorList< typename input_array_type::const_iterator > sastsais(it,n,C.getB(),sentinel);

	::libmaus2::timing::RealTimeClock lirtc;
	uint64_t saisz0rank = 0;


	if ( level >= 1 )
	{
		#if defined(COMPUTE_HASH_NOLOOKUP)
		if ( verbose )
			std::cerr << "Computing reduced string...";
		::libmaus2::timing::RealTimeClock redrtc; redrtc.start();
		typedef uint32_t hash_type;
		::libmaus2::suffixsort::HashReducedString<hash_type>::unique_ptr_type HRS = ::libmaus2::suffixsort::computeReducedStringHash< 
			input_array_type,hash_type>(C,stype,smallthres,verbose);
		::libmaus2::bitio::CompactArray::unique_ptr_type & hreduced = HRS->hreduced;
		if ( verbose )
		{
			std::cerr << "done, time " << redrtc.getElapsedSeconds() << " length=" << hreduced->size() << " bps=" << hreduced->getB() << std::endl;
			std::cerr << "Size of decode table " << HRS->namedict->byteSize() << std::endl;
			std::cerr << "Size of reduced string " << HRS->hreduced->byteSize() << std::endl;
		}
		typedef ::libmaus2::bitio::CompactArray sub_array_type;
		::libmaus2::bitio::CompactArray::const_iterator hreducedit(hreduced.get());
		uint64_t const subsize = hreduced->size();
		#else
		typedef uint32_t hash_type;
		typename ::libmaus2::suffixsort::HashLookupTable<hash_type>::unique_ptr_type HRS =
			::libmaus2::suffixsort::computeReducedStringHashLookupTable<input_array_type,hash_type>(C,stype,smallthres,verbose);
		typename ::libmaus2::suffixsort::HashLookupTable<hash_type>::const_iterator hreducedit = HRS->begin();
		uint64_t const subsize = HRS->size();
		typedef ::libmaus2::suffixsort::HashLookupTable<hash_type> sub_array_type;
		#endif

		if ( verbose )
			std::cerr << "Computing SA of hash reduced string...";
			
		::libmaus2::timing::RealTimeClock hrrtc; hrrtc.start();


		// #define BWT_OS_SAIS_COMPACT

		typedef ::libmaus2::autoarray::AutoArray<int32_t,::libmaus2::autoarray::alloc_type_c> sa_array_type;
		sa_array_type SAhreduced(subsize, false);

		::libmaus2::suffixsort::saisxx<
			typename ::libmaus2::suffixsort::HashLookupTable<hash_type>::const_iterator,
			int32_t *,
			int32_t
			>(hreducedit, SAhreduced.begin(), static_cast<int32_t>(subsize), HRS->K + 1 ) ;

		if ( verbose )
			std::cerr << "done, time " << hrrtc.getElapsedSeconds() << std::endl;
		
		// hreduced.reset();
		
		uint64_t const memfrac = 128;
		uint64_t const rpagemod = SAhreduced.getPageMod();
		uint64_t const memmod = std::max( (SAhreduced.byteSize()+memfrac-1)/memfrac, rpagemod );
		uint64_t const pagemod = (rpagemod > memmod) ? rpagemod : (((memmod+rpagemod-1)/rpagemod)*rpagemod);

		if ( verbose )
			std::cerr << "(Constructing list of sorted S* type suffixes from SAIS sorting...";
					
		lirtc.start();
		// uint64_t const r0shift = stype[0] ? 0 : 1;
		uint64_t const pages = ( SAhreduced.size() + pagemod - 1 ) / pagemod;

		#if defined(COMPUTE_HASH_NOLOOKUP)
		ImplicitBWT< sub_array_type, sa_array_type > hbwt(*hreduced,SAhreduced,subsize);
		#else
		ImplicitBWT< sub_array_type, sa_array_type > hbwt(*HRS,SAhreduced,subsize);
		#endif

		for ( int64_t page = static_cast<int64_t>(pages)-1; page >= 0; --page )
		{
			int64_t const low = std::min ( (static_cast<uint64_t>(page) * pagemod), SAhreduced.size() );
			int64_t const high = std::min ( low + pagemod, SAhreduced.size() );
			
			if ( verbose )
				std::cerr << "[" << low << "," << high << ")";
			
			for ( int64_t i = static_cast<int64_t>(high)-1; i >= low; --i )
			{
				// bwt character
				uint64_t const hbwtchar = hbwt[i];
				
				if ( hbwtchar )
				{
					bool const col = (*(HRS->namedict->SubColVec))[ hbwtchar ];
					
					if ( col )
					{
						uint64_t const p0 = HRS->namedict->getColPos(hbwtchar);
						uint64_t const p1 = stype.next1(p0+1)+1;
						sastsais.push(p0,p1);
					}
					else
					{
						std::pair<uint64_t,unsigned int> const ncword = HRS->namedict->getNonColWord(hbwtchar);					
						sastsais.pushExplicitWord(ncword.first,ncword.second);
					}				
				}
				else
				{
					saisz0rank = i;
				}		
			}
			
			SAhreduced.resize(low);
		}
			
		if ( verbose )
			std::cerr << "done, time " << lirtc.getElapsedSeconds() << ")";

		#if defined(BWT_OS_SAIS_COMPACT)
		PSAhreduced.reset();
		#endif
	}
	else
	{
		if ( verbose )
			std::cerr << "Computing reduced string...";
		::libmaus2::timing::RealTimeClock redrtc; redrtc.start();
		typedef uint32_t hash_type;
		::libmaus2::suffixsort::HashReducedString<hash_type>::unique_ptr_type HRS = ::libmaus2::suffixsort::computeReducedStringHash< 
			input_array_type,hash_type>(C,stype,smallthres,verbose);
		::libmaus2::bitio::CompactArray::unique_ptr_type & hreduced = HRS->hreduced;
		if ( verbose )
		{
			std::cerr << "done, time " << redrtc.getElapsedSeconds() << " length=" << hreduced->size() << " bps=" << hreduced->getB() << std::endl;
			std::cerr << "Size of decode table " << HRS->namedict->byteSize() << std::endl;
			std::cerr << "Size of reduced string " << HRS->hreduced->byteSize() << std::endl;
		}


		// recursion
		::libmaus2::bitio::CompactArray::unique_ptr_type subbwt = bwtOSHash(*hreduced, verbose, level+1);

		for ( int64_t i = static_cast<int64_t>(subbwt->size())-1; i >= 0; --i )
		{
			// bwt character
			uint64_t const hbwtchar = (*subbwt)[i];
			
			if ( hbwtchar )
			{
				bool const col = (*(HRS->namedict->SubColVec))[ hbwtchar ];
				
				if ( col )
				{
					uint64_t const p0 = HRS->namedict->getColPos(hbwtchar);
					uint64_t const p1 = stype.next1(p0+1)+1;
					sastsais.push(p0,p1);
				}
				else
				{
					std::pair<uint64_t,unsigned int> const ncword = HRS->namedict->getNonColWord(hbwtchar);					
					sastsais.pushExplicitWord(ncword.first,ncword.second);
				}				
			}
			else
			{
				saisz0rank = i;
			}		
		}
	}
	
	if ( verbose )
	{
		std::cerr << "Reversing...";
		lirtc.start();
	}
	sastsais.reverse();
	if ( verbose )
	{
		std::cerr << "done, time " << lirtc.getElapsedSeconds()/2 << std::endl;	
	}
	
	
	if ( verbose )
		std::cerr << "Rank of position 0 among S* suffixes is " << saisz0rank << std::endl;

	uint64_t const prepeak = ::libmaus2::autoarray::AutoArray_peakmemusage;
	
	if ( verbose )
	{
		std::cerr << "Inducing BWT from sorted S* type suffixes...";
		lirtc.start();
	}
	::libmaus2::bitio::CompactArray::unique_ptr_type sastsaisbwt = 
		::libmaus2::suffixsort::induce< ::libmaus2::suffixsort::mode_induce >(
			it,n,C.getB(),sentinel,sastsais,saisz0rank,!stype[0],verbose);
	if ( verbose )
		std::cerr << "done, time " << lirtc.getElapsedSeconds() << std::endl;

	uint64_t const postpeak = ::libmaus2::autoarray::AutoArray_peakmemusage;
	
	if ( verbose )
	{
		std::cerr << "level " << level << "Inducing added " 
			<< static_cast<uint64_t>(postpeak-prepeak) 
			<< std::endl;
	}

	return UNIQUE_PTR_MOVE(sastsaisbwt);
}

#include <libmaus2/bitio/CompactArray.hpp>
#include <libmaus2/bitio/BitVector.hpp>
#include <libmaus2/suffixsort/SAISUtils.hpp>
#include <libmaus2/suffixsort/SuccinctFactorList.hpp>
#include <libmaus2/suffixsort/OSInduce.hpp>

::libmaus2::bitio::CompactArray::unique_ptr_type bwtOS(::libmaus2::bitio::CompactArray const & C, bool const verbose)
{
	uint64_t const n = C.size();
	uint64_t const b = C.getB();
	uint64_t const sentinel = (1ull<<b);

	typedef ::libmaus2::bitio::CompactArray::const_iterator compact_const_it;
	compact_const_it it = C.begin();

	if ( verbose )
		std::cerr << "computing s* positions...";

	::libmaus2::autoarray::AutoArray < uint64_t > Shist, Lhist;
	uint64_t maxk = 0;
	::libmaus2::bitio::IndexedBitVector::unique_ptr_type pstype = ::libmaus2::suffixsort::computeSast(C.begin(),n,Shist,Lhist,maxk);
	::libmaus2::bitio::IndexedBitVector & stype = *pstype;
	
	if ( verbose )
		std::cerr << "done." << std::endl;

	if ( verbose )
		std::cerr << "creating list of unsorted s* substrings...";
	::libmaus2::suffixsort::SuccinctFactorList< ::libmaus2::bitio::CompactArray::const_iterator > sastpre(it,n,b,sentinel);
	uint64_t pleft = 0;
	for ( uint64_t i = 1; i < n; ++i )
		if ( stype[i] )
		{
			sastpre.push ( pleft, i+1 );
			pleft = i;
		}
	if ( verbose )
	{
		std::pair < uint64_t, uint64_t > eihist = sastpre.getStorageHistogram();
		std::cerr << "done, byte size " << sastpre.byteSize() 
			<< " storage: explicit=" << eihist.first << " implicit=" << eihist.second
			<< std::endl;
		::libmaus2::util::Histogram::unique_ptr_type sizehist = sastpre.sizeHistogram();
		sizehist->print(std::cerr);
	}

	if ( verbose )
		std::cerr << "Running induce on unsorted s*type substrings...";
	::libmaus2::timing::RealTimeClock unrtc; unrtc.start();
	::libmaus2::bitio::CompactArray::unique_ptr_type reduced =
		::libmaus2::suffixsort::induce< ::libmaus2::suffixsort::mode_sort >(it,n,b,sentinel,sastpre,0,!stype[0],verbose);
	if ( verbose )
		std::cerr << "done, time " << unrtc.getElapsedSeconds() << " length " << reduced->size() 
			<< " bits per symbol " << reduced->getB() << std::endl;

	if ( verbose )
		std::cerr << "Computing SA of reduced string...";
		
	::libmaus2::timing::RealTimeClock rrtc; rrtc.start();
	::libmaus2::bitio::CompactArray::const_iterator reducedit(reduced.get());

	typedef ::libmaus2::autoarray::AutoArray<int32_t,::libmaus2::autoarray::alloc_type_c> sa_array_type;
	typedef sa_array_type::unique_ptr_type sa_array_ptr_type;
	uint64_t const subsize = reduced->size();
	sa_array_ptr_type SAreduced(new sa_array_type(subsize, false));
	uint64_t subk = 0;
	for ( uint64_t i = 0; i < reduced->size(); ++i )
		subk = std::max(subk,reduced->get(i));

	::libmaus2::suffixsort::saisxx<
		::libmaus2::bitio::CompactArray::const_iterator,
		int32_t *,
		int32_t
		>(reducedit, SAreduced->begin(), static_cast<int32_t>(subsize), subk+1 ) ;

	// ::libmaus2::suffixsort::SAIS::sa_array_ptr_type SAreduced = ::libmaus2::suffixsort::SAIS::SA_IS_Compact(reducedit,reduced->size(),verbose);



	if ( verbose )
		std::cerr << "done, time " << rrtc.getElapsedSeconds() << std::endl;

	if ( verbose )
		std::cerr << "Constructing list of sorted S* type suffixes from SAIS sorting...";

	rrtc.start();
	::libmaus2::suffixsort::SuccinctFactorList< ::libmaus2::bitio::CompactArray::const_iterator > sastsais(it,n,b,sentinel);
	uint64_t r0shift = stype[0] ? 0 : 1;
	for ( uint64_t i = 0; i < SAreduced->size(); ++i )
	{
		uint64_t const p = SAreduced->get(i);

		if ( p != 0 )
		{
			uint64_t const p1 = stype.select1(p-r0shift) + 1;
			uint64_t const p0 = (p-r0shift) ? stype.prev1(p1-2) : 0;
			sastsais.push(p0,p1);
		}
	}
	
	if ( verbose )
		std::cerr << "done, time " << rrtc.getElapsedSeconds() << std::endl;
	
	uint64_t saisz0rank = 0;
	for ( uint64_t i = 0; i < SAreduced->size(); ++i )
		if ( ! SAreduced->get(i) )
			saisz0rank = i;

	if ( verbose )
		std::cerr << "Rank of position 0 among S* suffixes is " << saisz0rank << std::endl;

	if ( verbose )
		std::cerr << "Inducing BWT from sorted S* type suffixes...";
	rrtc.start();
	::libmaus2::bitio::CompactArray::unique_ptr_type sastsaisbwt = ::libmaus2::suffixsort::induce< ::libmaus2::suffixsort::mode_induce >(
		it,n,b,sentinel,sastsais,saisz0rank,!stype[0],verbose);
	if ( verbose )
		std::cerr << "done, time " << rrtc.getElapsedSeconds() << std::endl;

	return UNIQUE_PTR_MOVE(sastsaisbwt);
}


#include <libmaus2/bitio/BitVector.hpp>
#include <libmaus2/suffixsort/OSInduce.hpp>
#include <libmaus2/suffixsort/SAISUtils.hpp>
#include <libmaus2/suffixsort/SuccinctFactorList.hpp>
#include <libmaus2/suffixsort/divsufsort.hpp>
#include <libmaus2/suffixsort/SkewSuffixSort.hpp>
#include <libmaus2/util/GetFileSize.hpp>
#include <libmaus2/bitio/CompactArray.hpp>
#include <libmaus2/math/bitsPerNum.hpp>
#include <libmaus2/timing/RealTimeClock.hpp>
#include <libmaus2/aio/SynchronousGenericInput.hpp>
#include <libmaus2/wavelet/toWaveletTreeBits.hpp>
#include <libmaus2/util/IncreasingList.hpp>
#include <libmaus2/util/Histogram.hpp>
#include <libmaus2/fastx/FastAReader.hpp>
#include <libmaus2/fastx/FastQReader.hpp>
#include <libmaus2/hashing/hash.hpp>
#include <libmaus2/suffixsort/SAIS_Mori.hpp>
#include <libmaus2/select/ESelect222B.hpp>

#include <iostream>
#include <algorithm>
#include <numeric>

#include <libmaus2/lcp/LCP.hpp>
#include <libmaus2/lcp/OracleLCP.hpp>

#if 1
#define TESTSINGLE
//#define TESTBINARY
//#define TESTRANDOM
//#define TESTFILE
#define TESTFILEFA
#endif


::libmaus2::bitio::CompactArray::unique_ptr_type bwtDivSufSortCompact(::libmaus2::bitio::CompactArray const & C, bool const verbose = false)
{
	typedef ::libmaus2::bitio::CompactArray::const_iterator text_const_iterator;
	typedef ::libmaus2::bitio::CompactArray::iterator text_iterator;
	typedef ::libmaus2::bitio::SignedCompactArray::const_iterator sa_const_iterator;
	typedef ::libmaus2::bitio::SignedCompactArray::iterator sa_iterator;
	
	uint64_t const bitwidth = 64;
	typedef ::libmaus2::suffixsort::DivSufSort<bitwidth,text_iterator,text_const_iterator,sa_iterator,sa_const_iterator> sort_type;

	uint64_t const n = C.size();
	uint64_t const b = C.getB();
	::libmaus2::bitio::SignedCompactArray SA(n, ::libmaus2::math::bitsPerNum(n) + 1 );

	if ( verbose )
		std::cerr << "Running divsufsort...";
	::libmaus2::timing::RealTimeClock drtc; drtc.start();
	sort_type::divsufsort ( C.begin(), SA.begin(), n );
	if ( verbose )
		std::cerr << "done, time " << drtc.getElapsedSeconds() << std::endl;
	
	::libmaus2::bitio::CompactArray::unique_ptr_type BWT(new ::libmaus2::bitio::CompactArray(n,b));
	
	for ( uint64_t i = 0; i < n; ++i )
		if ( SA.get(i) )
			BWT -> set ( i, C.get(SA.get(i)-1) );
		else
			BWT -> set ( i, C.get(n-1) );
	
	return UNIQUE_PTR_MOVE(BWT);
}

::libmaus2::bitio::CompactArray::unique_ptr_type bwtDivSufSort(::libmaus2::bitio::CompactArray const & C, bool const verbose = false)
{
	if ( C.n < (1ull << 31) )
	{
		typedef ::libmaus2::bitio::CompactArray::const_iterator text_const_iterator;
		typedef ::libmaus2::bitio::CompactArray::iterator text_iterator;
		typedef int32_t const * sa_const_iterator;
		typedef int32_t * sa_iterator;
		
		uint64_t const bitwidth = 64;
		typedef ::libmaus2::suffixsort::DivSufSort<bitwidth,text_iterator,text_const_iterator,sa_iterator,sa_const_iterator> sort_type;

		uint64_t const n = C.size();
		uint64_t const b = C.getB();
		::libmaus2::autoarray::AutoArray< int32_t > SA(n,false);

		if ( verbose )
			std::cerr << "Running divsufsort...";
		::libmaus2::timing::RealTimeClock drtc; drtc.start();
		sort_type::divsufsort ( text_const_iterator(&C), SA.get(), n );
		if ( verbose )
			std::cerr << "done, time " << drtc.getElapsedSeconds() << std::endl;
		
		::libmaus2::bitio::CompactArray::unique_ptr_type BWT(new ::libmaus2::bitio::CompactArray(n,b));
		
		for ( uint64_t i = 0; i < n; ++i )
			if ( SA.get(i) )
				BWT -> set ( i, C.get(SA.get(i)-1) );
			else
				BWT -> set ( i, C.get(n-1) );
		
		return UNIQUE_PTR_MOVE(BWT);
	}
	else
	{
		typedef ::libmaus2::bitio::CompactArray::const_iterator text_const_iterator;
		typedef ::libmaus2::bitio::CompactArray::iterator text_iterator;
		typedef int64_t const * sa_const_iterator;
		typedef int64_t * sa_iterator;
		
		uint64_t const bitwidth = 64;
		typedef ::libmaus2::suffixsort::DivSufSort<bitwidth,text_iterator,text_const_iterator,sa_iterator,sa_const_iterator> sort_type;

		uint64_t const n = C.size();
		uint64_t const b = C.getB();
		::libmaus2::autoarray::AutoArray< int64_t > SA(n,false);

		if ( verbose )
			std::cerr << "Running divsufsort...";
		::libmaus2::timing::RealTimeClock drtc; drtc.start();
		sort_type::divsufsort ( text_const_iterator(&C), SA.get(), n );
		if ( verbose )
			std::cerr << "done, time " << drtc.getElapsedSeconds() << std::endl;
		
		::libmaus2::bitio::CompactArray::unique_ptr_type BWT(new ::libmaus2::bitio::CompactArray(n,b));
		
		for ( uint64_t i = 0; i < n; ++i )
			if ( SA.get(i) )
				BWT -> set ( i, C.get(SA.get(i)-1) );
			else
				BWT -> set ( i, C.get(n-1) );
		
		return UNIQUE_PTR_MOVE(BWT);
	
	}
}

::libmaus2::bitio::CompactArray::unique_ptr_type stringToCompact(std::string const & s, uint64_t const pad = 0)
{
	unsigned char maxi = std::numeric_limits<unsigned char>::min();
	for ( uint64_t i = 0; i < s.size(); ++i )
		maxi = std::max(maxi,reinterpret_cast<unsigned char const *>(s.c_str())[i]);
	unsigned int b = ::libmaus2::math::bitsPerNum(maxi);
	::libmaus2::bitio::CompactArray::unique_ptr_type ptr(new ::libmaus2::bitio::CompactArray(s.begin(),s.end(),b,pad));
	return UNIQUE_PTR_MOVE(ptr);
}

#if 0
::libmaus2::bitio::CompactArray::unique_ptr_type stringToCompactMapped(std::string const & s)
{
	if ( ! s.size() )
		return ::libmaus2::bitio::CompactArray::unique_ptr_type(new ::libmaus2::bitio::CompactArray(0,0));

	::libmaus2::util::Histogram charHistogram;
	unsigned char const * u = reinterpret_cast<unsigned char const *>(s.c_str());
	for ( uint64_t i = 0; i < s.size(); ++i )
		charHistogram(u[i]);
	std::vector<uint64_t> keys = charHistogram.getKeyVector();
	::libmaus2::autoarray::AutoArray<unsigned char> U(256);
	for ( uint64_t i = 0; i < keys.size(); ++i )
	{
		assert ( keys[i] < U.size() );
		U[keys[i]] = i;
	}
	unsigned char maxi = keys.size()-1;
	unsigned int b = ::libmaus2::math::bitsPerNum(maxi);
	return ::libmaus2::bitio::CompactArray::unique_ptr_type(new ::libmaus2::bitio::CompactArray(s.begin(),s.end(),b));
}
#endif

bool testInducedBWT(::libmaus2::bitio::CompactArray const & C, bool const verbose = false)
{
	uint64_t const n = C.size();

	::libmaus2::timing::RealTimeClock rtc; rtc.start();
	::libmaus2::bitio::CompactArray::unique_ptr_type bwtos = bwtOS(C, verbose);
	double const OStime = rtc.getElapsedSeconds();
	
	rtc.start();
	::libmaus2::bitio::CompactArray::unique_ptr_type bwtdiv = bwtDivSufSort(C, verbose);
	double const divtime = rtc.getElapsedSeconds();
	
	bool ok = true;
	
	if ( verbose )
		std::cerr << "Comparing BWTs...";
	ok = ok && (bwtos->size() == bwtdiv->size());
	for ( uint64_t i = 0; ok && i < n; ++i )
		ok = ok && bwtos->get(i) == bwtdiv->get(i);
	if ( verbose )
		std::cerr << "done, result " << (ok?"ok":"FAILED") 
			<< " OS " << OStime << " div " << divtime << std::endl;
	
	return ok;
}

bool testInducedBWTHash(::libmaus2::bitio::CompactArray const & C, bool const verbose = false)
{
	::libmaus2::autoarray::AutoArray_peakmemusage = ::libmaus2::autoarray::AutoArray_memusage;
	
	uint64_t const n = C.size();

	::libmaus2::timing::RealTimeClock rtc; rtc.start();
	//::libmaus2::bitio::CompactArray::unique_ptr_type bwtos = bwtOS(C, verbose);
	::libmaus2::bitio::CompactArray::unique_ptr_type bwtos = bwtOSHash(C, verbose);
	double const OStime = rtc.getElapsedSeconds();
	
	if ( verbose )
		std::cerr << "peak mem : " << (static_cast<double>(::libmaus2::autoarray::AutoArray_peakmemusage)) / C.n << std::endl;
	
	rtc.start();
	::libmaus2::bitio::CompactArray::unique_ptr_type bwtdiv = bwtDivSufSort(C, verbose);
	double const divtime = rtc.getElapsedSeconds();
	
	bool ok = true;
	
	if ( verbose )
		std::cerr << "Comparing BWTs...";
	ok = ok && (bwtos->size() == bwtdiv->size());
	for ( uint64_t i = 0; ok && i < n; ++i )
		ok = ok && bwtos->get(i) == bwtdiv->get(i);
	if ( verbose )
		std::cerr << "done, result " << (ok?"ok":"FAILED") 
			<< " OS " << OStime << " div " << divtime << std::endl;
	
	return ok;
}

bool testInducedBWT(std::string const & s, bool const verbose = false)
{
	uint64_t const n = s.size();
	
	if ( verbose )
		std::cerr << "Converting string of length " << n << " to compact array...";
	::libmaus2::bitio::CompactArray::unique_ptr_type C = stringToCompact(s,1 /* pad */);
	if ( verbose )
		std::cerr << "done, bits per symbol " << C->getB() << std::endl;

	return testInducedBWT(*C,verbose);
}

bool testInducedBWTHash(std::string const & s, bool const verbose = false)
{
	uint64_t const n = s.size();
	
	if ( verbose )
		std::cerr << "Converting string of length " << n << " to compact array...";
	::libmaus2::bitio::CompactArray::unique_ptr_type C = stringToCompact(s,1 /* pad */);
	if ( verbose )
		std::cerr << "done, bits per symbol " << C->getB() << std::endl;

	return testInducedBWTHash(*C,verbose);
}

void testIndFile(std::string const fn, uint64_t const maxsize = std::numeric_limits<uint64_t>::max(), uint64_t const roffset = 0)
{
	{
	uint64_t const offset = std::min(roffset,::libmaus2::util::GetFileSize::getFileSize(fn));
	uint64_t const n = std::min(::libmaus2::util::GetFileSize::getFileSize(fn)-offset,maxsize);
	std::cerr << "Running for file " << fn << " size " << n << " offset " << offset << std::endl;
	::libmaus2::autoarray::AutoArray<char> A(n+1);
	std::ifstream istr(fn.c_str(),std::ios::binary);
	assert ( istr.is_open() );
	istr.seekg(offset,std::ios::beg);
	assert ( istr );
	istr.read( A.get(), n );
	assert ( istr );
	assert ( istr.gcount() == static_cast<int64_t>(n) );
	A[n] = 0;
	std::string const s ( A.begin(), A.end() );
	bool const ok = testInducedBWTHash(s,true);
	if ( ! ok )
		std::cerr << fn << " failed!" << std::endl;
	else
		std::cerr << fn << " ok." << std::endl;
	}
}

#include <libmaus2/fastx/FastAReader.hpp>

uint64_t getFAFileLength(std::string const fn)
{
	uint64_t n = 0;
	::libmaus2::fastx::FastAReader::unique_ptr_type FastAR(new ::libmaus2::fastx::FastAReader(fn));
	::libmaus2::fastx::FastAReader::pattern_type pattern;
	bool first = true;
	while ( FastAR->getNextPatternUnlocked(pattern) )
	{
		if ( ! first )
			n++;
		n += pattern.getPatternLength();
		first = false;
	}
	return n;
}

uint64_t getFQFileLength(std::string const fn)
{
	uint64_t n = 0;
	::libmaus2::fastx::FastQReader::unique_ptr_type FastAR(new ::libmaus2::fastx::FastQReader(fn));
	::libmaus2::fastx::FastQReader::pattern_type pattern;
	bool first = true;
	while ( FastAR->getNextPatternUnlocked(pattern) )
	{
		if ( ! first )
			n++;
		n += pattern.getPatternLength();
		first = false;
	}
	return n;
}

void readFAFile(std::string const fn, ::libmaus2::bitio::CompactArray & C)
{
	::libmaus2::fastx::FastAReader::unique_ptr_type FastAR(new ::libmaus2::fastx::FastAReader(fn));
	::libmaus2::fastx::FastAReader::pattern_type pattern;

	uint64_t j = 0;
	bool first = true;
	while ( FastAR->getNextPatternUnlocked(pattern) )
	{
		if ( ! first )
			C.set(j++,5); // splitter
		for ( uint64_t i = 0; i < pattern.getPatternLength(); ++i,++j )
			C.set ( j , 1+::libmaus2::fastx::mapChar(pattern.pattern[i]) );
		first = false;
	}
}

void readFQFile(std::string const fn, ::libmaus2::bitio::CompactArray & C)
{
	::libmaus2::fastx::FastQReader::unique_ptr_type FastAR(new ::libmaus2::fastx::FastQReader(fn));
	::libmaus2::fastx::FastQReader::pattern_type pattern;

	uint64_t j = 0;
	bool first = true;
	while ( FastAR->getNextPatternUnlocked(pattern) )
	{
		if ( ! first )
			C.set(j++,5); // splitter
		for ( uint64_t i = 0; i < pattern.getPatternLength(); ++i,++j )
			C.set ( j , 1+::libmaus2::fastx::mapChar(pattern.pattern[i]) );
		first = false;
	}
}

bool testIndFileFA(std::string const fn, bool const verbose = true)
{
	if ( verbose )
		std::cerr << "Getting length of file " << fn << "...";
	uint64_t const n = getFAFileLength(fn);
	std::cerr << "done, length is " << n << std::endl;
	::libmaus2::bitio::CompactArray C(n+1,3,1); 
	std::cerr << "Reading file " << fn << " into CompactArray...";
	readFAFile(fn,C);
	C.set(n,0);
	std::cerr << "done." << std::endl;

	if ( verbose )
		std::cerr << "Testing OS on " << fn << " of length " << n << "...";
	bool const ok = testInducedBWTHash(C, verbose);
	if ( verbose )
		std::cerr << "done, result " << (ok?"ok":"FAILED") << std::endl;

	return ok;
}

bool testIndFileFQ(std::string const fn, bool const verbose = true)
{
	if ( verbose )
		std::cerr << "Getting length of file " << fn << "...";
	uint64_t const n = getFQFileLength(fn);
	std::cerr << "done, length is " << n << std::endl;
	::libmaus2::bitio::CompactArray C(n+1,3,1); 
	std::cerr << "Reading file " << fn << " into CompactArray...";
	readFQFile(fn,C);
	C.set(n,0);
	std::cerr << "done." << std::endl;

	if ( verbose )
		std::cerr << "Testing OS on " << fn << " of length " << n << "...";
	bool const ok = testInducedBWTHash(C, verbose);
	if ( verbose )
		std::cerr << "done, result " << (ok?"ok":"FAILED") << std::endl;

	return ok;
}

std::string getTerminatedBinaryString(uint64_t const l, uint64_t const i)
{
	std::string s(l+1,' ');
	for ( uint64_t j = 0; j < l; ++j )
		if ( i & (1ull<<j) )
			s[j] = 'a';
		else
			s[j] = 'b';
	s[l] = '$';

	return s;
}

void testIncudedBWTSingle(std::string const s)
{
	bool const testSlow = true;
	bool const ok = testInducedBWTHash(s,true);

	if ( testSlow )
	{
		bool const slowok = testInducedBWT(s,true);
		std::cerr << s << " result " << (ok?"ok":"failed") << " " << (slowok?"ok":"failed") << std::endl;
	}
	else
	{
		std::cerr << s << " result " << (ok?"ok":"failed") << std::endl;
	}

}

void testIncudedBWTBinary(unsigned int const l = 12)
{
	std::cerr << "Testing all binary strings of length " << l << "...";
	for ( uint64_t i = 0; i < (1ull << l); ++i )
	{
		std::string const s = getTerminatedBinaryString(l,i);	
		
		bool const ok = testInducedBWTHash(s);
		if ( ! ok )
			std::cerr << s << " result " << (ok?"ok":"failed") << std::endl;
	}
	std::cerr << "done." << std::endl;
}

void testInducedBWTRandom(uint64_t const n = 16*1024*1024, uint64_t const a = 4)
{
	std::string s(n+1,' ');
	for ( uint64_t i = 0; i < n; ++i )
		s[i] = 'a' + (rand() % a); // ;(rand() & 1) ? 'a' : 'b';
	s[n] = 0;
	
	bool const ok = testInducedBWTHash(s,true);
	if ( ! ok )
		std::cerr << s << " result " << (ok?"ok":"failed") << std::endl;
	else
		std::cerr << "ok." << std::endl;
}

int main(int argc, char * argv[])
{
	try
	{
		::libmaus2::util::ArgInfo arginfo(argc,argv);
		srand(time(0));	
		testIncudedBWTSingle("fischersfritzfischtfrischefische$");
		testIncudedBWTSingle("jmmississiippii$");
		testIncudedBWTSingle("mmississiippii$");
		testIncudedBWTSingle("ababababababab$");
		testIncudedBWTSingle("babbbabbba$");
		testIncudedBWTSingle("abbbabbbaa$");
		testIncudedBWTSingle("abbbbabbbb$");
		testIncudedBWTSingle("fischfritz$");
		testIncudedBWTSingle("babbbbbb$");
		testIncudedBWTBinary();
		testInducedBWTRandom();
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
