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
#include <iostream>
#include <cassert>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <libmaus/suffixsort/GapArrayByte.hpp>

void testGapArrayByte()
{
	uint64_t const gsize = 329114;
	libmaus::autoarray::AutoArray<uint32_t> GG(gsize);
	for ( uint64_t i = 0; i < gsize; ++i )
		GG[i] = 1;
	
	for ( uint64_t i = 0; i < 32; ++i )
		GG[libmaus::random::Random::rand64() % gsize] = 257;
	
	uint64_t const numthreads = 32;
	
	libmaus::suffixsort::GapArrayByte GAB(gsize,16,numthreads,"tmp");

	#if defined(_OPENMP)
	#pragma omp parallel for num_threads(numthreads)
	#endif
	for ( uint64_t i = 0; i < gsize; ++i )
		for ( uint64_t j = 0; j < GG[i]; ++j )
			if ( GAB(i) )
				GAB(i,omp_get_thread_num());

	GAB.flush();
	
	uint64_t offset = 0;
	libmaus::suffixsort::GapArrayByteDecoder::unique_ptr_type pdec(GAB.getDecoder(offset));
	libmaus::autoarray::AutoArray<uint32_t> TT(48);
	
	while ( offset != gsize )
	{
		uint64_t const rest = gsize - offset;
		uint64_t const tocopy = std::min(rest,TT.size());
		pdec->decode(TT.begin(),tocopy);
		
		for ( uint64_t i = 0; i < tocopy; ++i )
			assert (
				TT[i] == GG[offset+i]
			);
		
		offset += tocopy;
	}
}

int main()
{
	try
	{
		srand(time(0));
		testGapArrayByte();
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
