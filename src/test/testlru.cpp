/*
    libmaus2
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#include <libmaus2/lru/SparseLRUFileBunch.hpp>
#include <libmaus2/lru/LRU.hpp>
#include <libmaus2/lru/FileBunchLRU.hpp>

void testlrusparsefile(uint64_t const tparts, uint64_t const maxcur)
{
	libmaus2::autoarray::AutoArray<char> C = libmaus2::autoarray::AutoArray<char>::readFile("configure.ac");
	uint64_t bytesperpart = (C.size() + tparts-1)/tparts;
	uint64_t parts = (C.size() + bytesperpart-1)/bytesperpart;
	libmaus2::lru::SparseLRUFileBunch SLRUFB("tmpf",maxcur);

	for ( uint64_t j = 0; j < parts; ++j )
		SLRUFB.remove(j);

	for ( uint64_t i = 0; i < bytesperpart; ++i )
	{
		for ( uint64_t j = 0; j < parts; ++j )
		{
			uint64_t const p = j * bytesperpart + i;

			if ( p < C.size() )
			{
				libmaus2::aio::InputOutputStream & str = SLRUFB[j];
				str.put(C[p]);
			}
		}

		if ( i % 1024 == 0 )
			std::cerr << static_cast<double>(i)/bytesperpart << std::endl;
	}

	for ( uint64_t j = 0; j < parts; ++j )
	{
		libmaus2::aio::InputOutputStream & str = SLRUFB[j];
		str.flush();

		str.seekg(0,std::ios::end);
		str.clear();
		#if 0
		uint64_t const l = str.tellg();
		#endif
		str.seekg(0,std::ios::beg);
		str.clear();
		#if 0
		uint64_t const l2 = str.tellg();
		#endif

		for ( uint64_t i = 0; i < bytesperpart; ++i )
		{
			uint64_t const p = j * bytesperpart + i;

			if ( p < C.size() )
			{
				int const c = str.get();
				assert ( c >= 0 );
				assert ( c == C[p] );
			}
		}
	}

	for ( uint64_t j = 0; j < parts; ++j )
		SLRUFB.remove(j);

}

int main()
{
	try
	{
		libmaus2::lru::SparseLRU SLRU(4);

		std::cerr << SLRU.get(0) << std::endl;
		std::cerr << SLRU.get(1) << std::endl;
		std::cerr << SLRU.get(2) << std::endl;
		std::cerr << SLRU.get(3) << std::endl;
		std::cerr << SLRU.get(4) << std::endl;

		for ( uint64_t i = 0; i < 16*1024; ++i )
		{
			std::cerr << SLRU.get(rand()%5) << std::endl;
		}

		testlrusparsefile(5, 7);
		testlrusparsefile(5, 3);
		testlrusparsefile(5, 1);
	}
	catch(std::exception const & ex)
	{

	}
}
