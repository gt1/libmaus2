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

#include <libmaus/fastx/StreamFastQReader.hpp>
#include <libmaus/lz/BufferedGzipStream.hpp>
#include <libmaus/fastx/GzipStreamFastQReader.hpp>
#include <libmaus/fastx/GzipFileFastQReader.hpp>
#include <libmaus/parallel/LockedBool.hpp>
#include <libmaus/util/ArgInfo.hpp>

void decodeGzipFastqBlocks(std::string const & filename, std::string const & indexfilename)
{
	libmaus::aio::CheckedInputStream FICIS(indexfilename);
	std::vector < libmaus::fastx::FastInterval > FIV = 
		libmaus::fastx::FastInterval::deserialiseVector(FICIS);
		
	for ( uint64_t i = 0; i < FIV.size(); ++i )
	{
		std::cerr << FIV[i] << std::endl;
			
		libmaus::fastx::GzipFileFastQReader reader(filename,FIV[i]);
		libmaus::fastx::GzipFileFastQReader::pattern_type pattern;

		while ( reader.getNextPatternUnlocked(pattern) )
			std::cout << pattern;		
	}
}


std::string getNameBase(std::string const & s)
{
	uint64_t d = s.size();
	for ( uint64_t i = 0; i < s.size(); ++i )
		if ( s[i] == '/' )
			d = i;
	
	return s.substr(0,d);
}

void countReadsGzipFastqBlocks(std::string const & filename, std::string const & indexfilename)
{
	libmaus::aio::CheckedInputStream FICIS(indexfilename);
	std::vector < libmaus::fastx::FastInterval > FIV = 
		libmaus::fastx::FastInterval::deserialiseVector(FICIS);

	libmaus::parallel::LockedBool ok(true);

	#if defined(_OPENMP)
	#pragma omp parallel for schedule(dynamic,1)
	#endif
	for ( int64_t i = 0; i < static_cast<int64_t>(FIV.size()); ++i )
	{
		if ( ok.get() )
		{
			// std::cerr << FIV[i] << std::endl;			
			libmaus::fastx::GzipFileFastQReader reader(filename,FIV[i]);
			libmaus::fastx::GzipFileFastQReader::pattern_type pattern;

			uint64_t cnt = 0;
			std::string prevname;
			bool nok = true;
			while ( nok && reader.getNextPatternUnlocked(pattern) )
			{
				++cnt;

				if ( cnt % 2 == 0 )
				{
					if ( getNameBase(prevname) != getNameBase(pattern.sid) )
					{
						nok = false;
					}
				}
				
				prevname = pattern.sid;				
			}
			
			if ( i % 1024 == 0 )
				std::cerr << FIV[i] << std::endl;
			if ( cnt != FIV[i].high - FIV[i].low )
			{
				std::cerr << cnt << "\t" << FIV[i].high - FIV[i].low << std::endl;
				ok.set(false);
			}
			if ( cnt % 2 != 0 )
			{
				std::cerr << cnt << " is uneven " << std::endl;
				ok.set(false);			
			}
			if ( ! nok )
			{
				std::cerr << "name fail " << std::endl;
				ok.set(false);						
			}
		}
	}
	
	if ( ! ok.get() )
		std::cerr << "failed." << std::endl;
	else
		std::cerr << "Ok." << std::endl;
}

void decodeGzipFastQStream(std::istream & in, std::ostream & out)
{
	libmaus::fastx::GzipStreamFastQReader reader(in);
	libmaus::fastx::GzipStreamFastQReader::pattern_type pattern;
	while ( reader.getNextPatternUnlocked(pattern) )
		out << pattern;	
}

void decodeGzipFastqBlocksByParameters(int argc, char * argv[])
{
	libmaus::util::ArgInfo const arginfo(argc,argv);		
	std::string const filename = arginfo.stringRestArg(0);
	std::string const indexfilename = arginfo.stringRestArg(1);
	decodeGzipFastqBlocks(filename,indexfilename);
}

#include <libmaus/lz/BgzfInflate.hpp>
#include <libmaus/fastx/FastQBgzfWriter.hpp>

void testNextStart()
{
	libmaus::fastx::FastQBgzfWriter writer("index.id",1024,std::cout);
	
	char input[] = "@A/1\nACGT\n+\nHHHH\n@AAA/2\nTGCAT\n+plus\nHHHHH\n";
	std::istringstream istr(input);
	libmaus::fastx::StreamFastQReaderWrapper fqin(istr);
	libmaus::fastx::StreamFastQReaderWrapper::pattern_type pattern;
		
	while ( fqin.getNextPatternUnlocked(pattern) )
	{
		// std::cerr << pattern;
		writer.put(pattern);
	}
		
	// writer.testPrevStart();
	writer.testNextStart();
}

int main(int argc, char * argv[])
{
	try
	{
		libmaus::util::ArgInfo const arginfo(argc,argv);
		
		testNextStart();
		
		for ( uint64_t i = 0; i < arginfo.restargs.size(); ++i )
		{
			std::string const filename = arginfo.stringRestArg(i);
			countReadsGzipFastqBlocks(filename,filename+".idx");
		}
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
