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
#include <libmaus/util/ArgInfo.hpp>

#include <libmaus/bitio/FastWriteBitWriter.hpp>
#include <libmaus/bitio/ArrayDecode.hpp>

void testArrayDecode()
{
	for ( unsigned int b = 0; b < 16; ++b )
	{
		// std::cerr << "b=" << b << std::endl;
		for ( uint64_t n = 0; n <= 256; ++n )
		{
			::libmaus::autoarray::AutoArray<uint8_t> A( (n*b + 7)/8, false );
			::libmaus::bitio::FastWriteBitWriter FWB(A.get());
		
			for ( uint64_t i = 0; i < n; ++i )
				FWB.write ( i % (1ull<<b) , b );
			FWB.flush();
		
			::libmaus::autoarray::AutoArray<uint8_t> O(n,false);
			::libmaus::bitio::ArrayDecode::decodeArray(A.begin(),O.begin(),n,b);
		
			// std::cerr << "n=" << n << std::endl;
			for ( uint64_t i = 0; i < n; ++i )
				assert ( O[i] == i%(1ull<<b) );
		}
	}
}


int main()
{
	testArrayDecode();
}
