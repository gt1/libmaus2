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

#include <libmaus/aio/AsynchronousBufferReader.hpp>
#include <libmaus/aio/AsynchronousWriter.hpp>
#include <libmaus/timing/RealTimeClock.hpp>

#include <vector>
#include <map>
#include <cmath>

int main(int argc, char * argv[])
{
	if ( argc < 3 )
	{
		std::cerr << "usage: " << argv[0] << " <in0> <in1> ... <out>" << std::endl;
		return EXIT_FAILURE;
	}

	try
	{
		::libmaus::timing::RealTimeClock rtc;
		rtc.start();

		std::list<std::string> filenames;
		for ( int i = 1; i+1 < argc; ++i )
			filenames.push_back(argv[i]);

		// ::libmaus::aio::AsynchronousBufferReader ABR(argv[1],16,1ull << 18);	
		::libmaus::aio::AsynchronousBufferReaderList ABR(filenames.begin(),filenames.end(),16,1ull << 18);	
		::libmaus::aio::AsynchronousWriter AW(argv[argc-1]);	
		uint64_t copied = 0;
		uint64_t copymeg = 0;

		std::pair < char const *, ssize_t > data;

		while ( ABR.getBuffer(data) )
		{
			AW.write( data.first, data.first+data.second );
			ABR.returnBuffer();
			copied += data.second;

			if ( copied / (1024*1024) != copymeg )
			{
				copymeg = copied / (1024*1024);
				std::cerr << "\r                                 \rCopied " << copymeg << " mb" << std::flush;
			}
		}
		std::cerr << "\r                                 \rReading of " << std::floor(static_cast<double>(copied)/(1024.0*1024.0)+0.5) << "MB done. Flushing output...";

		AW.flush();

		std::cerr << "done, real time " << rtc.getElapsedSeconds() << " seconds, rate "
			<< (copied/(1024.0*1024.0))/rtc.getElapsedSeconds()
			<< "MB/s"
			<< std::endl;
	}
	catch(std::exception const & ex)
	{
		std::cerr << "exception: " << ex.what() << std::endl;
	}
}
