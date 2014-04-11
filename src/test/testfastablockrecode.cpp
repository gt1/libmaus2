/*
    libmaus
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

#include <libmaus/exception/LibMausException.hpp>
#include <libmaus/fastx/StreamFastAReader.hpp>

void countFastA()
{
	libmaus::fastx::StreamFastAReaderWrapper S(std::cin);
	libmaus::fastx::StreamFastAReaderWrapper::pattern_type pat;
	uint64_t const bs = 64*1024;
	uint64_t ncnt = 0;
	uint64_t rcnt = 0;
	while ( S.getNextPatternUnlocked(pat) )
	{
		std::string const & s = pat.spattern;
		
		uint64_t low = 0;
		while ( low != s.size() )
		{
			uint64_t const high = std::min(low+bs,static_cast<uint64_t>(s.size()));
			bool none = false;
			
			for ( uint64_t i = low; i != high; ++i )
				switch ( s[i] )
				{
					case 'A':
					case 'C':
					case 'G':
					case 'T':
						break;
					default:
						none = true;
				}
				
			if ( none )
				++ncnt;
			else
				++rcnt;
				
			low = high;
		}
		
		std::cerr << "[V] " << pat.sid << std::endl;
	}

	std::cerr << "ncnt=" << ncnt << std::endl;
	std::cerr << "rcnt=" << rcnt << std::endl;
}

int main()
{
	try
	{
		countFastA();	
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}

