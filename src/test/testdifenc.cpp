#include <iostream>
#include <libmaus/util/ArgInfo.hpp>
#include <libmaus/random/Random.hpp>
#include <libmaus/math/bitsPerNum.hpp>

int main(int argc, char * argv[])
{
	try
	{
		::libmaus::util::ArgInfo const arginfo(argc,argv);
		
		::libmaus::random::Random::setup();
		uint64_t const n = 16*1024;
		uint64_t const gaparsize = n;
		uint64_t const ins = n*32;
		
		::libmaus::autoarray::AutoArray<uint64_t> GAP(gaparsize);
		for ( uint64_t i = 0; i < ins; ++i )
		{
			uint64_t const j = ::libmaus::random::Random::rand64() % gaparsize;
			GAP [ j ] ++;
		}
		
		::libmaus::autoarray::AutoArray<uint64_t> bitcnts(64);
		
		for ( uint64_t i = 0; i < GAP.size(); ++i )
		{
			uint64_t const v = GAP[i];
			unsigned int const bits = ::libmaus::math::bitsPerNum(v);
			bitcnts[ bits ]++;
		}
		
		uint64_t bitcnthigh = 0;
		for ( uint64_t i = 0; i < 64; ++i )
			if ( bitcnts[i] )
				bitcnthigh = i;

		for ( uint64_t i = 0; i <= bitcnthigh; ++i )
			std::cerr << "bitcnts[" << i << "]=" << bitcnts[i] << std::endl;
		
		if ( GAP.size() <= 32 )
		{
			for ( uint64_t i = 0; i < GAP.size(); ++i )
				std::cerr << GAP[i] << ";";
			std::cerr << std::endl;
		}
	}
	catch(std::exception const & ex)
	{
	
	}
}

