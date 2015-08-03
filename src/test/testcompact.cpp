/*
    libmaus2
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
#include <libmaus2/bitio/CompactDecoderBuffer.hpp>
#include <libmaus2/util/TempFileRemovalContainer.hpp>
#include <libmaus2/bitio/CompactArrayWriter.hpp>

void testctz()
{
	for ( uint64_t i = 1, j = 0; i; i <<=1, ++j )
		assert ( ::libmaus2::bitio::Ctz::ctz(i) == j );
}

void testcompact()
{
	std::string const fn("tmpfile");
	#if 0
	std::string const fn2("tmpfile2");
	std::string const fnm("tmpfile.merged");
	#endif

	::libmaus2::util::TempFileRemovalContainer::setup();
	::libmaus2::util::TempFileRemovalContainer::addTempFile(fn);
	
	uint64_t n = 1024*1024;
	unsigned int const b = 3;
	::libmaus2::bitio::CompactArray CA(n,b);
	::libmaus2::bitio::CompactArrayWriter CAW(fn,n,b);
	srand(time(0));
	for ( uint64_t i = 0; i < n; ++i )
	{
		CA.set(i,rand() & ((1ull<<b)-1));
		CAW.put(CA.get(i));
	}
	CAW.flush();
	#if 0
	::libmaus2::aio::CheckedOutputStream COS(fn);
	CA.serialize(COS);
	COS.flush();
	COS.close();
	#endif
	
	::libmaus2::aio::InputStreamInstance CIS(fn);
	std::cerr << "compact file size is " << ::libmaus2::util::GetFileSize::getFileSize(CIS) << std::endl;
	assert ( static_cast< ::std::streampos > (CIS.tellg()) == static_cast< ::std::streampos >(0) );
	assert ( CIS.get() >= 0 );
	
	::libmaus2::bitio::CompactDecoderWrapper W(fn,4096);
	
	W.seekg(0,std::ios::end);
	int64_t const fs = W.tellg();
	W.seekg(0,std::ios::beg);
	W.clear();
	
	assert ( fs == static_cast<int64_t>(n) );
	
	std::cerr << "n=" << n << " fs=" << fs << std::endl;
	
	for ( uint64_t i = 0; i < n; ++i )
	{
		assert ( W.tellg() == static_cast< ::std::streampos >(i) );
		int const v = W.get();
		assert ( v == static_cast<int>(CA[i]) );
		// std::cerr << static_cast<int>(W.get()) << " " << CA[i] << std::endl;
	}
	
	for ( uint64_t i = 0; i < n; i += (rand() % 256) )
	{
		W.clear();
		W.seekg(i);
		
		std::cerr << "seek to " << W.tellg() << std::endl;
		
		for ( uint64_t j = i; j < n; ++j )
		{
			assert ( W.tellg() == static_cast< ::std::streampos >(j) );
			int const v = W.get();
			assert ( v == static_cast<int>(CA[j]) );
		}
		
		uint64_t ii = n-i;
		W.clear();
		W.seekg(ii);

		for ( uint64_t j = ii; j < n; ++j )
		{
			assert ( W.tellg() == static_cast< ::std::streampos >(j) );
			int const v = W.get();
			assert ( v == static_cast<int>(CA[j]) );
		}
	}
}

int main(/* int argc, char * argv[] */)
{
	try
	{
		testcompact();
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
	}
}
