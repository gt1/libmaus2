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

#include <libmaus/btree/btree.hpp>

#include <set>

template<typename key_type, typename comp_type>
void testRandom()
{
	for ( unsigned int i = 0; i < 50; ++i )
	{
		std::set < key_type, comp_type > S;
		libmaus::btree::BTree< key_type, 32, comp_type> tree;
		
		for ( unsigned int j = 0; j < 10000; ++j )
		{
			key_type num = rand();
			S.insert(num);
			tree.insert(num);
		}
		
		for ( 
			typename std::set<key_type, comp_type>::const_iterator ita = S.begin(); 
			ita != S.end(); 
			++ita 
		)
		{
			assert ( tree.contains(*ita) );
			tree[*ita];
		}
			
		assert ( tree.size() == S.size() );
		
		std::vector < key_type > V;
		tree.toVector(V);
		
		std::vector < key_type > V2(S.begin(),S.end());
		assert ( V == V2 );
	}
}

template<typename key_type>
void testRandomType()
{
	testRandom < key_type, std::less <key_type> >();
	testRandom < key_type, std::greater <key_type> >();
}

int main()
{
	srand(time(0));
	
	testRandomType<unsigned short>();
	testRandomType<unsigned int>();
	testRandomType<uint64_t>();
}
