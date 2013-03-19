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

#include <libmaus/parallel/TerminatableSynchronousReorderSet.hpp>
#include <iostream>

void testSynchronousReorderSet()
{
	try
	{
		::libmaus::parallel::TerminatableSynchronousReorderSet<uint64_t> TSH;
		TSH.enque(0,5);
		TSH.enque(1,3);
		TSH.enque(2,4);
		TSH.enque(3,2);
		TSH.enque(4,7);
		TSH.terminate();
		
		while ( true )
		{
			try
			{
				uint64_t const v = TSH.deque();	
				std::cerr << v << std::endl;
			}
			catch(std::exception const &ex)
			{
				std::cerr << ex.what() << std::endl;
				break;
			}
		}
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
	}
}

int main(/* int argc, char * argv[] */)
{
	testSynchronousReorderSet();
}
