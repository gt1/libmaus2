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
#include <iostream>
#include <cassert>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <libmaus/timing/RealTimeClock.hpp>
#include <libmaus/gamma/GammaEncoder.hpp>
#include <libmaus/gamma/GammaDecoder.hpp>

template<typename T>
struct VectorPut : public std::vector<T>
{
	VectorPut() {}
	void put(T const v)
	{
		std::vector<T>::push_back(v);
	}
};

struct CountPut
{
	uint64_t cnt;

	CountPut()
	: cnt(0)
	{
	
	}
	
	template<typename T>
	void put(T const)
	{
		++cnt;	
	}
};

template<typename T>
struct VectorGet
{
	typename std::vector<T>::const_iterator it;
	
	VectorGet(typename std::vector<T>::const_iterator rit)
	: it(rit)
	{}
	
	uint64_t get()
	{
		return *(it++);
	}
};

void testRandom(unsigned int const n)
{
	srand(time(0));
	std::vector<uint64_t> V(n);
	for ( uint64_t i = 0; i <n; ++i )
		V[i] = rand();

	::libmaus::timing::RealTimeClock rtc; 
	
	rtc.start();
	CountPut CP;
	::libmaus::gamma::GammaEncoder< CountPut > GCP(CP);	
	for ( uint64_t i = 0; i < n; ++i )
		GCP.encode(V[i]);
	GCP.flush();
	double const cencsecs = rtc.getElapsedSeconds();
	std::cerr << "[V] count encoded " << n << " numbers in time " << cencsecs 
		<< " rate " << (n / cencsecs)/(1000*1000) << " m/s"
		<< " output words " << CP.cnt
		<< std::endl;
	
	VectorPut<uint64_t> VP;
	rtc.start();
	::libmaus::gamma::GammaEncoder< VectorPut<uint64_t> > GE(VP);	
	for ( uint64_t i = 0; i < n; ++i )
		GE.encode(V[i]);
	GE.flush();
	double const encsecs = rtc.getElapsedSeconds();
	std::cerr << "[V] encoded " << n << " numbers to dyn growing vector in time " << encsecs 
		<< " rate " << (n / encsecs)/(1000*1000) << " m/s"
		<< std::endl;

	rtc.start();
	VectorGet<uint64_t> VG(VP.begin());
	::libmaus::gamma::GammaDecoder < VectorGet<uint64_t> > GD(VG);
	bool ok = true;
	for ( uint64_t i = 0; ok && i < n; ++i )
	{
		uint64_t const v = GD.decode();
		ok = ok && (v==V[i]);
		if ( ! ok )
		{
			std::cerr << "expected " << V[i] << " got " << v << std::endl;
		}
	}
	double const decsecs = rtc.getElapsedSeconds();
	std::cerr << "[V] decoded " << n << " numbers in time " << decsecs
		<< " rate " << (n / decsecs)/(1000*1000) << " m/s"
		<< std::endl;
	
	if ( ok )
	{
		std::cout << "Test of gamma coding with " << n << " random numbers ok." << std::endl;
	}
	else
	{
		std::cout << "Test of gamma coding with " << n << " random numbers failed." << std::endl;
	}
}

void testLow()
{
	unsigned int const n = 127;
	VectorPut<uint64_t> VP;
	::libmaus::gamma::GammaEncoder< VectorPut<uint64_t> > GE(VP);	
	for ( uint64_t i = 0; i < n; ++i )
		GE.encode(i);
	GE.flush();
	
	VectorGet<uint64_t> VG(VP.begin());
	::libmaus::gamma::GammaDecoder < VectorGet<uint64_t> > GD(VG);
	bool ok = true;
	for ( uint64_t i = 0; ok && i < n; ++i )
		ok = ok && ( GD.decode() == i );

	if ( ok )
	{
		std::cout << "Test of gamma coding with numbers up to 126 ok." << std::endl;
	}
	else
	{
		std::cout << "Test of gamma coding with numbers up to 126 failed." << std::endl;
	}
}

#include <libmaus/gamma/GammaGapEncoder.hpp>
#include <libmaus/huffman/IndexDecoderDataArray.hpp>
#include <libmaus/gamma/GammaGapDecoder.hpp>
#include <libmaus/util/TempFileRemovalContainer.hpp>

void testgammagap()
{
	unsigned int n = 512*1024+199481101;
	std::vector<uint64_t> V (n);
	std::vector<uint64_t> V2(n);
	for ( uint64_t i = 0; i < V.size(); ++i )
	{
		V[i] = i & 0xFFull;
		V2[i] = rand() % 0xFFull;
	}

	std::string const fn("tmpfile");
	std::string const fn2("tmpfile2");
	std::string const fnm("tmpfile.merged");

	::libmaus::util::TempFileRemovalContainer::setup();
	::libmaus::util::TempFileRemovalContainer::addTempFile(fn);
	::libmaus::util::TempFileRemovalContainer::addTempFile(fn2);
	::libmaus::util::TempFileRemovalContainer::addTempFile(fnm);

	::libmaus::gamma::GammaGapEncoder GGE(fn);
	GGE.encode(V.begin(),V.end());
	::libmaus::gamma::GammaGapEncoder GGE2(fn2);
	GGE2.encode(V2.begin(),V2.end());
	
	::libmaus::huffman::IndexDecoderData IDD(fn);
	
	::libmaus::gamma::GammaGapDecoder GGD(std::vector<std::string>(1,fn));
	
	bool ok = true;
	for ( uint64_t i = 0; i < n; ++i )
	{
		uint64_t const v = GGD.decode();
		ok = ok && (v == V[i]);
	}
	std::cout << "decoding " << (ok ? "ok" : "fail") << std::endl;

	std::vector < std::vector<std::string> > merin;
	merin.push_back(std::vector<std::string>(1,fn));
	merin.push_back(std::vector<std::string>(1,fn2));
	::libmaus::gamma::GammaGapEncoder::merge(merin,fnm);
}

#include <libmaus/gamma/GammaRLEncoder.hpp>
#include <libmaus/gamma/GammaRLDecoder.hpp>

void testgammarl()
{
	srand(time(0));
	unsigned int n = 128*1024*1024;
	unsigned int n2 = 64*1024*1024;
	std::vector<uint64_t> V (n);
	std::vector<uint64_t> V2 (n2);
	std::vector<uint64_t> Vcat;
	unsigned int const albits = 3;
	uint64_t const almask = (1ull << albits)-1;
	
	for ( uint64_t i = 0; i < V.size(); ++i )
	{
		V[i]  = rand() & almask;
		Vcat.push_back(V[i]);
	}
	for ( uint64_t i = 0; i < V2.size(); ++i )
	{
		V2[i] = rand() & almask;
		Vcat.push_back(V2[i]);
	}

	std::string const fn("tmpfile");
	std::string const fn2("tmpfile2");
	std::string const fn3("tmpfile3");
	::libmaus::util::TempFileRemovalContainer::setup();
	::libmaus::util::TempFileRemovalContainer::addTempFile(fn);
	::libmaus::util::TempFileRemovalContainer::addTempFile(fn2);
	::libmaus::util::TempFileRemovalContainer::addTempFile(fn3);

	::libmaus::gamma::GammaRLEncoder GE(fn,albits,V.size(),256*1024);	
	for ( uint64_t i = 0; i < V.size(); ++i )
		GE.encode(V[i]);
	GE.flush();

	::libmaus::gamma::GammaRLEncoder GE2(fn2,albits,V2.size(),256*1024);
	for ( uint64_t i = 0; i < V2.size(); ++i )
		GE2.encode(V2[i]);
	GE2.flush();

	#if 0
	::libmaus::huffman::IndexDecoderData IDD(fn);
	for ( uint64_t i = 0; i < IDD.numentries+1; ++i )
		std::cerr << IDD.readEntry(i) << std::endl;	
	#endif

	::libmaus::gamma::GammaRLDecoder GD(std::vector<std::string>(1,fn));
	assert ( GD.getN() == n );

	for ( uint64_t i = 0; i < n; ++i )
		assert ( GD.decode() == static_cast<int64_t>(V[i]) );
	
	uint64_t const off = n / 2 + 1031;
	::libmaus::gamma::GammaRLDecoder GD2(std::vector<std::string>(1,fn),off);
	for ( uint64_t i = off; i < n; ++i )
		assert ( GD2.decode() == static_cast<int64_t>(V[i]) );
		
	std::vector<std::string> fnv;
	fnv.push_back(fn);
	fnv.push_back(fn2);
	::libmaus::gamma::GammaRLEncoder::concatenate(fnv,fn3);
	
	for ( uint64_t off = 0; off < Vcat.size(); off += 18521 )
	{
		::libmaus::gamma::GammaRLDecoder GD3(std::vector<std::string>(1,fn3),off);	
		assert ( GD3.getN() == Vcat.size() );
		
		for ( uint64_t i = 0; i < std::min(static_cast<uint64_t>(1024ull),Vcat.size()-off); ++i )
			assert ( GD3.decode() == static_cast<int64_t>(Vcat[off+i]) );
	}

	remove ( fn.c_str() );
	remove ( fn2.c_str() );
	remove ( fn3.c_str() );	
}

int main()
{
	try
	{
		testgammarl();
		testLow();
		testRandom(256*1024*1024);
		testgammagap();
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
