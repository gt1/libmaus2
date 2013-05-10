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
#include <cstdlib>
#include <ctime>

#include <libmaus/rmq/RMMTree.hpp>
#include <libmaus/random/Random.hpp>

int testImpCompactArray()
{
	libmaus::random::Random::setup();
	std::vector<uint64_t> V;
	for ( uint64_t i = 0; i < 64*1024; ++i )
		V.push_back(libmaus::random::Random::rand64() % 128);

	libmaus::util::ImpCompactNumberArray::unique_ptr_type P = UNIQUE_PTR_MOVE(
		libmaus::util::ImpCompactNumberArrayGenerator::constructFromArray(V.begin(),V.size())
	);
	libmaus::util::ImpCompactNumberArray & I = *P;
	
	std::ostringstream ostr;
	I.serialise(ostr);
	std::istringstream istr(ostr.str());
	libmaus::util::ImpCompactNumberArray::unique_ptr_type P2 = UNIQUE_PTR_MOVE(libmaus::util::ImpCompactNumberArray::load(istr));
	
	for ( libmaus::util::ImpCompactNumberArray::const_iterator ita = P2->begin(); ita != P2->end(); ++ita )
	{
		assert ( (*ita) == V[ita-P2->begin()] );
		// std::cerr << *ita << std::endl;
	}
	return 0;
}

int testRMMTree()
{
	for ( uint64_t z = 0; z < 128; ++z )
	{
		libmaus::random::Random::setup(time(0)+z);
		uint64_t const n = 3912 + libmaus::random::Random::rand64() % 32;
		std::vector<uint64_t> V;
		for ( uint64_t i = 0; i < n; ++i )
			V.push_back(libmaus::random::Random::rand64() % 128);
		libmaus::rmq::RMMTree< std::vector<uint64_t>, 5, true /* debug */> RMM(V,V.size());
	
		#if defined(_OPENMP)
		#pragma omp parallel for
		#endif
		for ( int64_t i = 0; i < static_cast<int64_t>(V.size()); ++i )
			for ( uint64_t j = i; j < V.size(); ++j )
				RMM.rmq(i,j);
				
		#if defined(_OPENMP)
		#pragma omp parallel for
		#endif
		for ( int64_t i = 0; i < static_cast<int64_t>(V.size()); ++i )
		{
			RMM.nsv(i);
			RMM.psv(i);
		}
			
		std::cerr << ".";
	}
	
	return 0;
}

int main()
{
	std::cerr << "Testing compact number array...";
	testImpCompactArray();
	std::cerr << "done." << std::endl;

	std::cerr << "Testing RMM tree...";
	testRMMTree();
	std::cerr << "done." << std::endl;
}
