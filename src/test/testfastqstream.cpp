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

/* int main(int argc, char * argv[]) */
int main()
{
	try
	{

		/*
		 * test libmaus::fastx::StreamFastQReaderWrapper class by 
		 * reading a gzip compressed FastQ file and outputting it
		 * in uncompressed FastQ format
		 */
		libmaus::lz::BufferedGzipStream GZ(std::cin);
		libmaus::fastx::StreamFastQReaderWrapper reader(GZ);
		libmaus::fastx::StreamFastQReaderWrapper::pattern_type pattern;
		while ( reader.getNextPatternUnlocked(pattern) )
			std::cout << pattern;	
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
