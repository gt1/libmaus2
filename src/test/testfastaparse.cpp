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
#include <libmaus/fastx/FastAStreamSet.hpp>
#include <libmaus/fastx/FastALineParser.hpp>
#include <libmaus/fastx/FastAMapTable.hpp>
#include <libmaus/fastx/FastALineParserLineInfo.hpp>
#include <libmaus/fastx/FastAMatchTable.hpp>
#include <libmaus/timing/RealTimeClock.hpp>
#include <libmaus/aio/PosixFdInputStream.hpp>

#include <iostream>

int main()
{
	::libmaus::fastx::FastAMatchTable FAMT;
	std::cerr << FAMT;

	libmaus::timing::RealTimeClock rtc;
	rtc.start();
	
	::libmaus::aio::PosixFdInputStream posin(STDIN_FILENO,64*1024);
	::libmaus::fastx::FastAStreamSet FASS(posin);
	libmaus::autoarray::AutoArray<char> B(64*1024,false);
	std::pair<std::string,::libmaus::fastx::FastAStream::shared_ptr_type> P;
	uint64_t bases = 0;
	
	while ( FASS.getNextStream(P) )
	{
		std::string const & id = P.first;
		::libmaus::fastx::FastAStream & basestream = *(P.second);
		
		uint64_t cnt = 0;
		while ( true )
		{
			basestream.read(B.begin(),B.size());
			uint64_t gcnt = basestream.gcount();
			cnt += gcnt;
			
			if ( !gcnt )
				break;
		}

		std::cout << id << "\t" << cnt << "\n";
		
		bases += cnt;
	}
	
	std::cerr << (bases / rtc.getElapsedSeconds()) << " bases/s " << "(" << bases << "/" << rtc.getElapsedSeconds() << ")" << std::endl;
}
