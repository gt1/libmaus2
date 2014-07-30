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

#include <libmaus/hashing/hash.hpp>
#include <map>
#include <vector>
#include <algorithm>
#include <iostream>
#include <libmaus/util/SimpleHashMap.hpp>

void testHashCollisions()
{
	std::map < uint64_t, uint64_t > S;
	unsigned int const icnt = 2048;
	unsigned int const jcnt = 16;
	std::vector<uint64_t> V;
	uint64_t colcnt = 0;
	for ( unsigned int i = 0; i < icnt; ++i )
	{
		// std::cerr << "i=" << i << std::endl;
		V.clear();
		for ( unsigned int j = 0; j < jcnt; ++j )
		{
			uint64_t const val = libmaus::hashing::EvaHash::hash2Single(i,(0xb979379e)+j) & ((1ull << 16)-1);
			// std::cerr << (val & ((1ull<<16)-1)) << std::endl;
			S[val]++;
			V.push_back(val);
		}
		std::sort(V.begin(),V.end());
		bool col = false;
		for ( uint64_t j = 1; j < V.size(); ++j )
			if ( V[j-1] == V[j] )
				col = true;
		if ( col )
		{
			colcnt++;
			std::cerr << "col for i=" << i << std::endl;
			for ( uint64_t j = 0; j < V.size(); ++j )
				std::cerr << V[j] << ";";
			std::cerr << std::endl;
		}
	}
	
	uint64_t acc = 0;
	for ( std::map < uint64_t, uint64_t >::const_iterator ita = S.begin();
		ita != S.end(); ++ita )
		acc += ita->second;
	double const avg = static_cast<double>(acc)/S.size();
	
	std::cerr << "S.size()=" << S.size() << " icnt*jcnt=" << icnt*jcnt << " avg=" << avg 
		<< " colcnt=" << colcnt << std::endl;
}

int main(/* int argc, char * argv[] */)
{
	libmaus::uint::UInt<1> U;
	libmaus::util::SimpleHashMapKeyPrint< libmaus::uint::UInt<1> >::printKey(std::cerr,U);
	std::cerr << std::endl;

	libmaus::util::SimpleHashMapConstants< libmaus::uint::UInt<2> > SHMC;

	std::cerr << "unused " << SHMC.unused() << std::endl;
	
	libmaus::util::SimpleHashMap< libmaus::uint::UInt<1>, uint64_t > H(1);
	
	H.insertNonSyncExtend(libmaus::uint::UInt<1>(5), 5, 0.8 );

	testHashCollisions();
}
