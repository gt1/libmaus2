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
#include <cstdlib>
#include <libmaus/util/ArgInfo.hpp>
#include <libmaus/fastx/QReorder.hpp>

void printBits(uint64_t const i)
{
	for ( uint64_t m = 1ull << 63; m; m >>= 1 )
		if ( i & m )
			std::cerr << '1';
		else
			std::cerr << '0';
	
	std::cerr << std::endl;
}

void testReorder4()
{
	unsigned int const k = 2;
	uint64_t const fraglen = 2;
	
	for ( uint64_t z = 0; z < 16; ++z )
	{
		libmaus::fastx::QReorderTemplate4Base<k> Q(
			fraglen+((z&1)!=0),
			fraglen+((z&2)!=0),
			fraglen+((z&4)!=0),
			fraglen+((z&8)!=0)
		);
	
		for ( uint64_t i = 0; i < (1ull<<((Q.l0+Q.l1+Q.l2+Q.l3)*Q.k)); ++i )
		{			
			assert ( i == Q.front01i(Q.front01(i)) );
			assert ( i == Q.front02i(Q.front02(i)) );
			assert ( i == Q.front03i(Q.front03(i)) );
			assert ( i == Q.front12i(Q.front12(i)) );
			assert ( i == Q.front13i(Q.front13(i)) );
			assert ( i == Q.front23i(Q.front23(i)) );
		}
	}
}

uint64_t slowComp(uint64_t a, uint64_t b, unsigned int k, unsigned int l)
{	
	unsigned int d = 0;
	uint64_t const mask = libmaus::math::lowbits(k);
	
	for ( unsigned int i = 0; i < l; ++i, a >>= k, b >>= k )
		 if ( (a & mask) != (b & mask) )
		 {
		 	d++;
		 }
		 
	return d;
}

std::vector<uint64_t> filter(
	std::vector<uint64_t> const & V, 
	uint64_t const q,
	unsigned int const k,
	unsigned int const l,
	unsigned int const maxmis
)
{
	std::vector<uint64_t> O;
	for ( uint64_t i = 0; i < V.size(); ++i )
		if ( slowComp(V[i],q,k,l) <= maxmis )
			O.push_back(V[i]);
	std::sort(O.begin(),O.end());
	return O;
}

void testQReorder4Set()
{
	srand(5);
	std::vector<uint64_t> V;
	unsigned int const l = 7;
	unsigned int const k = 2;
	uint64_t const bmask = (1ull<<(l*k))-1;
	
	for ( uint64_t i = 0; i < 32; ++i )
		V.push_back( rand() & bmask );

	libmaus::fastx::QReorder4Set<k,uint64_t> QR(l,V.begin(),V.end(),2);
	libmaus::fastx::AutoArrayWordPutObject<uint64_t> AAWPO;
	for ( uint64_t i = 0; i < V.size(); ++i )
	{
		QR.search(V[i],0,AAWPO);
		assert ( AAWPO.p );
		
		// std::cerr << V[i] << "\t" << AAWPO.p << std::endl;
		
		if ( AAWPO.p )
			for ( uint64_t j = 0; j < AAWPO.p; ++j )
				assert ( AAWPO.A[j] == V[i] );

		std::vector<uint64_t> O = filter(V,V[i] /* q */,k,l,0);
		assert ( O == std::vector<uint64_t>(AAWPO.A.begin(),AAWPO.A.begin()+AAWPO.p) );
	}

	for ( uint64_t i = 0; i < V.size(); ++i )
	{
		for ( unsigned int j = 0; j < (l*k); ++j )
		{
			uint64_t const q = V[i] ^ (1ull << j);
			QR.search(q,1,AAWPO);
			assert ( AAWPO.p );

			std::vector<uint64_t> O = filter(V,q,k,l,1);
			assert ( O == std::vector<uint64_t>(AAWPO.A.begin(),AAWPO.A.begin()+AAWPO.p) );
		}
	}

	for ( uint64_t i = 0; i < V.size(); ++i )
	{
		for ( unsigned int j = 0; j < (l*k); ++j )
			for ( unsigned int z = 0; z < (l*k); ++z )
			{
				uint64_t const q = (V[i] ^ (1ull << j)) ^ (1ull << z);
			
				QR.search(q,2,AAWPO);
				assert ( AAWPO.p );

				std::vector<uint64_t> O = filter(V,q,k,l,2);
				assert ( O == std::vector<uint64_t>(AAWPO.A.begin(),AAWPO.A.begin()+AAWPO.p) );
				// std::cerr << "yes, " << O.size() << std::endl;
			}
	}
}

int main()
{
	try
	{		
		testQReorder4Set();
		return EXIT_SUCCESS;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}

