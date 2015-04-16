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
#include <libmaus/util/SimpleHashMap.hpp>
#include <libmaus/util/SimpleHashSet.hpp>
#include <libmaus/util/SimpleHashMapInsDel.hpp>
#include <libmaus/util/SimpleHashSetInsDel.hpp>

void testSimpleHash()
{
	try
	{
		libmaus::util::SimpleHashMapInsDel<uint64_t,uint64_t> SHMID(0);
		
		for ( uint64_t z = 0; z < 128; ++z )
		{
			for ( uint64_t i = 0; i < 64; ++i )
				SHMID.insertExtend(i,3*i+5,.8);
			for ( uint64_t i = 0; i < 64; ++i )
				assert ( SHMID.get(i) == 3*i+5 );
			for ( uint64_t i = 0; i < 64; i += 2 )
				SHMID.erase(i);
			for ( uint64_t i = 1; i < 64; i += 2 )
				assert ( SHMID.get(i) == 3*i+5 );
			for ( uint64_t i = 0; i < 64; i += 2 )
				assert ( ! SHMID.contains(i) );
			for ( uint64_t i = 0; i < 64; i += 2 )
				SHMID.insert(i,3*i+5);
			for ( uint64_t i = 0; i < 64; ++i )
				assert ( SHMID.get(i) == 3*i+5 );
			for ( uint64_t i = 0; i < 64; ++i )
				SHMID.erase(i);
				
			assert ( SHMID.size() == 0 );
		}
			
		::libmaus::util::ExtendingSimpleHashSet<uint64_t> HH(8);
		HH.insert(5);
		assert ( HH.contains(5) );
	
		::libmaus::util::SimpleHashMap<uint64_t,uint64_t> H(16);
	
		H.insert(3,10);
		H.insert(5,25);
		H.insert(5,30);
		H.insert(3,11);
	
		std::cerr << H.get(5) << std::endl;
		std::cerr << H.get(3) << std::endl;
		std::cerr << H.get(7) << std::endl;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
	}
}

int main(/* int argc, char * argv[] */)
{
	testSimpleHash();
}
