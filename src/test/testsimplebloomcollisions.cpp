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

#include <libmaus/util/SimpleBloomFilter.hpp>

void testSimpleBloomCollisions()
{
	uint64_t const n = 500ull*1000ull*1000ull;
	::libmaus::util::SimpleBloomFilter::unique_ptr_type SBF(::libmaus::util::SimpleBloomFilter::construct(n,0.1)); //(16,28 /* log */);

	uint64_t col = 0;	
	for ( uint64_t i = 0; i < 16*1024; ++i )
	{
		uint64_t const v = ::libmaus::random::Random::rand64() & 0xFFFFul;
		if ( SBF->insert(v) )
		{
			col++;
		}
	}
	
	std::cerr << "col=" << col << std::endl;
}

int main(/*int argc, char * argv[]*/)
{
	testSimpleBloomCollisions();
}
