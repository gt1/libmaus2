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

#include <libmaus/sorting/SortingBufferedOutputFile.hpp>

int main()
{
	std::string const filename = "tmpfile";
	typedef uint64_t data_type;
	// typedef std::greater<data_type> order_type;
	typedef std::less<data_type> order_type;
	typedef libmaus::sorting::SortingBufferedOutputFile<data_type,order_type> sbof_type;
	sbof_type SBOF(filename,16);
	
	for ( uint64_t i = 0; i < 256; ++i )
		SBOF.put(i % 31);
	
	sbof_type::merger_ptr_type MRB(SBOF.getMerger());
	data_type v;
	data_type pre = 0;
	while ( MRB->getNext(v) )
	{
		assert ( v >= pre );
		std::cout << v << ";";
		pre = v;
	}
	std::cout << std::endl;

	remove(filename.c_str());
}
