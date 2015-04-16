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

#include <libmaus2/aio/AsynchronousBufferReader.hpp>
#include <libmaus2/aio/AsynchronousWriter.hpp>
#include <libmaus2/aio/SynchronousFastReaderBase.hpp>
#include <libmaus2/timing/RealTimeClock.hpp>

#include <vector>
#include <map>
#include <cmath>

void testSynchronousFastReaderBase(int argc, char ** argv)
{
	std::vector <std::string> filenames;
	for ( int i = 1; i < argc; ++i )
		filenames.push_back(argv[i]);

	uint64_t const l = ::libmaus2::util::GetFileSize::getFileSize(filenames);

	for ( uint64_t j = 0; j < l; ++j )
	{
		::libmaus2::aio::SynchronousFastReaderBase SFRB(filenames,1,2,j);
		int c = -1;
		
		while ( (c=SFRB.getNextCharacter()) != -1 )
		{
			std::cout.put(c);
		}
		
		std::cout << std::endl;
	}
}

int main(int argc, char * argv[])
{
	testSynchronousFastReaderBase(argc,argv);
}
