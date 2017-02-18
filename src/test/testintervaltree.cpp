/*
    libmaus2
    Copyright (C) 2017 German Tischler

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
#include <libmaus2/geometry/IntervalTree.hpp>

int main()
{
	try
	{
		libmaus2::geometry::IntervalTree IT(3022);
		IT.insert(50,62);
		IT.insert(57,70);
		IT.insert(20,40);
		IT.insert(1000,1048);
		IT.enumerate(std::cerr);
		std::cerr << std::endl;

		for ( uint64_t i = 0; i < 20; ++i )
			assert ( !IT.query(i,i+1) );
		for ( uint64_t i = 20; i < 40; ++i )
			assert ( IT.query(i,i+1) );
		for ( uint64_t i = 40; i < 50; ++i )
			assert ( !IT.query(i,i+1) );
		for ( uint64_t i = 50; i < 70; ++i )
			assert ( IT.query(i,i+1) );
		for ( uint64_t i = 70; i < 1000; ++i )
			assert ( !IT.query(i,i+1) );
		for ( uint64_t i = 1000; i < 1048; ++i )
			assert ( IT.query(i,i+1) );
		for ( uint64_t i = 1048; i < 2000; ++i )
			assert ( !IT.query(i,i+1) );

		IT.reset();
		for ( uint64_t i = 0; i < 2000; ++i )
			assert ( !IT.query(i,i+1) );

		IT.enumerate(std::cerr);
		std::cerr << std::endl;
		IT.insert(500,1000);
		IT.enumerate(std::cerr);
		std::cerr << std::endl;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
	}
}
