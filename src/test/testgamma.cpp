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
		push_back(v);
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

int main()
{
	testLow();
	testRandom(256*1024*1024);
}
