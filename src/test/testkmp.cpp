/**
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
**/

#include <libmaus/util/KMP.hpp>
#include <libmaus/random/Random.hpp>
#include <libmaus/util/GetFileSize.hpp>
#include <libmaus/util/ArgInfo.hpp>

void testLazyFailureFunctionRandom(std::string const & pattern)
{
	::libmaus::autoarray::AutoArray<int64_t> BP = ::libmaus::util::KMP::BEST_PREFIX(pattern.begin(),pattern.size());
	::libmaus::random::Random::setup();
	
	for ( uint64_t i = 0; i < 16*1024*1024; ++i )
	{
		std::istringstream pistr(pattern);
		::libmaus::util::KMP::BestPrefix<std::istream> DBP(pistr,pattern.size());
		
		std::vector<uint64_t> probes;
		for ( uint64_t j = 0; j < 2*pattern.size(); ++j )
			probes.push_back( ::libmaus::random::Random::rand64() % (BP.size()) );
	
		#if 0
		for ( uint64_t i = 0; i < probes.size(); ++i )
			std::cerr <<  BP[probes[i]] << ";";
		std::cerr << std::endl;
		#endif
		
		for ( uint64_t i = 0; i <= pattern.size(); ++i )
		{
			assert ( DBP[probes[i]] == BP[probes[i]] );
			#if 0
			std::cerr << DBP[probes[i]] << ";";
			#endif
		}
		#if 0
		std::cerr << std::endl;
		#endif
	}
}

void testSimpleDynamic()
{
	std::string const needle = "needlene";
	std::string const haystack = "find the need in the haystack";
	
	std::pair<uint64_t, uint64_t> P = ::libmaus::util::KMP::PREFIX_SEARCH(
		needle.begin(),needle.size(),
		haystack.begin(),haystack.size());
		
	std::cerr << "result: " <<
		haystack.substr(P.first,P.second) << std::endl;

	std::istringstream needlestr(needle);
	::libmaus::util::KMP::BestPrefix<std::istream> BP(needlestr,needle.size());
	::libmaus::util::KMP::BestPrefix<std::istream>::BestPrefixXAdapter xadapter = BP.getXAdapter();
	
	for ( uint64_t i = 0; i < needle.size(); ++i )
		std::cerr << xadapter[i];
	std::cerr << std::endl;

	std::string const thestack = "somewhere in needleneedlene";
	std::istringstream thestackstr(thestack);

	std::pair<uint64_t, uint64_t> Q = 
		::libmaus::util::KMP::PREFIX_SEARCH_INTERNAL_RESTRICTED(xadapter,needle.size(),BP,thestackstr,thestack.size(),
			thestack.size()-needle.size());
			
	std::cerr << "Q result: " <<
		thestack.substr(Q.first,Q.second) << " position " << Q.first << " of " << thestack.size() << " patlen " << needle.size()  << std::endl;
		
	std::cerr << thestack << std::endl;
	std::cerr << "pre " << thestack.substr(0,Q.first) << std::endl;
	std::cerr << "suf " << thestack.substr(Q.first+Q.second) << std::endl;
}

void findSplitCommon(
	std::string const & filename,
	uint64_t const low,
	uint64_t const high,
	int64_t middle = -1
)
{
	if ( middle < 0 )
		middle = (high+low)/2;

	// uint64_t const p = n/2;
	uint64_t const n = high-low;
	uint64_t const m = high-middle;
	
	std::ifstream istrn(filename.c_str(),std::ios::binary);
	istrn.seekg(low);
	std::ifstream istrm(filename.c_str(),std::ios::binary);
	istrm.seekg(middle);
	
	// dynamically growing best prefix table
	::libmaus::util::KMP::BestPrefix<std::istream> BP(istrm,m);
	// adapter for accessing pattern in BP
	::libmaus::util::KMP::BestPrefix<std::istream>::BestPrefixXAdapter xadapter = BP.getXAdapter();
	// call KMP adaption
	std::pair<uint64_t, uint64_t> Q = ::libmaus::util::KMP::PREFIX_SEARCH_INTERNAL_RESTRICTED(xadapter,m,BP,istrn,n,n-m);
	
	uint64_t const showlen = std::min(Q.second,static_cast<uint64_t>(20));	
	std::cerr << "low=" << low << " Q.second=" << Q.second << " Q.first=" << Q.first << " middle=" << middle << " m=" << m << " pref=" << 
		std::string(BP.x.begin(),BP.x.begin()+showlen) 
		<< std::endl;
}

int main(int argc, char * argv[])
{
	try
	{
		::libmaus::util::ArgInfo const arginfo(argc,argv);
		std::string const fn = arginfo.getRestArg<std::string>(0);
		
		// std::string const fn = "1";
		uint64_t const n = ::libmaus::util::GetFileSize::getFileSize(fn);
		uint64_t const tpacks = 128;
		uint64_t const packsize = (n+tpacks-1)/tpacks;
		uint64_t const packs = (n + packsize-1)/packsize;
		
		#if 0 && defined(_OPENMP)
		#pragma omp parallel for
		#endif
		for ( uint64_t i = 0; i < packs; ++i )
		{
			uint64_t const low = std::min(i * packsize,n);
			uint64_t const high = std::min(low+packsize,n);
			findSplitCommon(fn,low,high);
		}
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
	
	#if 0
	std::string const fn = "X";
	uint64_t const n = ::libmaus::util::GetFileSize::getFileSize(fn);
	
	uint64_t const p = n/2;
	uint64_t const m = n-p;
	
	std::ifstream istrn(fn.c_str(),std::ios::binary);
	std::ifstream istrm(fn.c_str(),std::ios::binary);
	istrm.seekg(p);
	::libmaus::util::KMP::BestPrefix<std::istream> BP(istrm,m);
	::libmaus::util::KMP::BestPrefix<std::istream>::BestPrefixXAdapter xadapter = BP.getXAdapter();

	std::pair<uint64_t, uint64_t> Q = 
		::libmaus::util::KMP::PREFIX_SEARCH_INTERNAL_RESTRICTED(
			xadapter,m,BP,
			istrn,n,
			n-m
		);
	
	uint64_t const printmax = 30;	
	if ( Q.second <= printmax )
		std::cerr 
			<< "pos " << Q.first << " len " << Q.second << " p=" << p << " m=" << m 
			<< " common prefix " << std::string(BP.x.begin(),BP.x.begin()+Q.second)
			<< std::endl;
	else
		std::cerr 
			<< "pos " << Q.first << " len " << Q.second << " p=" << p << " m=" << m 
			<< " common length " << Q.second
			<< std::endl;
	
	istrn.clear(); istrn.seekg(Q.first);
	istrm.clear(); istrm.seekg(p);
	
	for ( uint64_t i = 0; i < Q.second /* std::min(Q.second, static_cast<uint64_t>(20)) */; ++i )
	{
		int const cn = istrn.get();
		int const cm = istrm.get();
		assert ( cn == cm );
		//std::cerr.put(cn);
		//std::cerr.put(cm);
	}
	// std::cerr << std::endl;
	int const cn = istrn.get();
	int const cm = istrm.get();
	
	assert ( (cn < 0 && cm < 0) || (cn != cm) );
	
	if ( Q.second+1 <= m )
	{
		istrm.clear(); istrm.seekg(p);
		::libmaus::autoarray::AutoArray<char> cs(Q.second+1);
		istrm.read(cs.begin(),cs.size());
		std::string s(cs.begin(),cs.begin()+cs.size());
		if ( Q.second <= printmax )
		std::cerr << "extending common prefix by next character in pattern: " << s << std::endl;
		
		istrn.clear(); istrn.seekg(0);
		::libmaus::autoarray::AutoArray<char> cc(n);
		istrn.read(cc.begin(),n);
		std::string t(cc.begin(),cc.begin()+cc.size());
		
		size_t pos = 0;
		while ( pos != std::string::npos )
		{
			pos = t.find(s,pos);
			if ( pos != std::string::npos )
			{
				std::cerr << "found " << pos << std::endl;
				pos += 1;
			}
		}
	}

	return 0;

	testLazyFailureFunctionRandom("abaabaababaabab");
	testLazyFailureFunctionRandom("alabar_a_la_alabarda");
	#endif
}
