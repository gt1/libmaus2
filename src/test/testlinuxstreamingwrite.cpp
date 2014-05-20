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

#include <libmaus/aio/LinuxStreamingPosixFdOutputStream.hpp>
#include <libmaus/aio/PosixFdOutputStream.hpp>
#include <libmaus/timing/RealTimeClock.hpp>
#include <libmaus/util/ArgInfo.hpp>

void writeStreaming(std::string const & fn, uint64_t const bufsize, uint64_t const numbufs)
{
	libmaus::aio::LinuxStreamingPosixFdOutputStream stream(fn,bufsize);
	libmaus::autoarray::AutoArray<char> B(bufsize);
	
	libmaus::timing::RealTimeClock rtc; rtc.start();
	uint64_t written = 0;
	uint64_t prevprint = 0;
	for ( uint64_t i = 0; i < numbufs; ++i )
	{
		stream.write(B.begin(),B.size());	
		written += B.size();
		
		if ( written / (128*1024*1024) != prevprint / (128*1024*1024) )
		{
			std::cerr << written/(1024*1024) << "\t" << (written/rtc.getElapsedSeconds())/(1024*1024) << std::endl;
			prevprint = written;
		}
	}
	
	stream.flush();
	// stream.close();
	
	std::cout << "streaming written " << (written / rtc.getElapsedSeconds()) / (1024*1024) << std::endl;
}

void writeNonStreaming(std::string const & fn, uint64_t const bufsize, uint64_t const numbufs)
{
	libmaus::aio::PosixFdOutputStream stream(fn,bufsize);
	libmaus::autoarray::AutoArray<char> B(bufsize);
	
	libmaus::timing::RealTimeClock rtc; rtc.start();
	uint64_t written = 0;
	uint64_t prevprint = 0;
	for ( uint64_t i = 0; i < numbufs; ++i )
	{
		stream.write(B.begin(),B.size());	
		written += B.size();

		if ( written / (128*1024*1024) != prevprint / (128*1024*1024) )
		{
			std::cerr << written/(1024*1024) << "\t" << (written/rtc.getElapsedSeconds())/(1024*1024) << std::endl;
			prevprint = written;
		}
	}
	
	stream.flush();
	// stream.close();
	
	std::cout << "non streaming written " << (written / rtc.getElapsedSeconds()) / (1024*1024) << std::endl;
}

int main(int argc, char * argv[])
{
	try
	{
		libmaus::util::ArgInfo const arginfo(argc,argv);
		std::string const fn = arginfo.restargs.at(0);
		std::string const fnstreaming = fn + ".streaming";
		std::string const fnnonstreaming = fn + ".nonstreaming";
		remove(fnstreaming.c_str());
		remove(fnnonstreaming.c_str());
		sleep(5);
		uint64_t const bufsize = 8*1024*1024;
		uint64_t const numbufs = 128;
		writeStreaming(fnstreaming,bufsize,numbufs);
		writeNonStreaming(fnnonstreaming,bufsize,numbufs);
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
