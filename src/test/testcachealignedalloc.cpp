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

#include <libmaus/autoarray/AutoArray.hpp>

void testCacheAlignedAlloc()
{
	#if defined(LIBMAUS_USE_ASSEMBLY) && defined(LIBMAUS_HAVE_i386)
	{
	unsigned int const linesize = ::libmaus::util::I386CacheLineSize::getCacheLineSize();
	std::cerr << "linesize = " << linesize << std::endl;
	}
	#endif

	try
	{
		::libmaus::autoarray::AutoArray<uint64_t,::libmaus::autoarray::alloc_type_memalign_cacheline> A(5);
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
	}
}

int main(/*int argc, char * argv[]*/)
{
	testCacheAlignedAlloc();
}
