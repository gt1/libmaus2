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
#include <libmaus/lz/SimpleCompressedInputBlockConcat.hpp>

#include <libmaus/lz/SimpleCompressedInputBlock.hpp>
#include <libmaus/lz/SimpleCompressedOutputStream.hpp>
#include <libmaus/lz/ZlibCompressorObjectFactory.hpp>
#include <libmaus/lz/ZlibDecompressorObjectFactory.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/util/TempFileRemovalContainer.hpp>
#include <libmaus/aio/CheckedOutputStream.hpp>
#include <sstream>

void testInputBlock()
{
	std::ostringstream ostr;
	libmaus::lz::ZlibCompressorObjectFactory compfact;
	libmaus::lz::SimpleCompressedOutputStream<std::ostream> compstr(ostr,compfact);
	libmaus::autoarray::AutoArray<char> B(64*1024,false);
	
	while ( std::cin )
	{
		std::cin.read(B.begin(),B.size());
		if ( std::cin.gcount() )
			compstr.write(B.begin(),std::cin.gcount());
	}
	compstr.flush();
	ostr.flush();
	
	std::istringstream istr(ostr.str());
	libmaus::lz::ZlibDecompressorObjectFactory decompfact;
	libmaus::lz::SimpleCompressedInputBlock block(decompfact);
	
	while ( block.readBlock(istr) && ! block.eof )
	{
		bool const ok = block.uncompressBlock();
		assert ( ok );
		
		std::cout.write(reinterpret_cast<char const *>(block.O.begin()),block.uncompsize);
	}
}

void testConcatInputBlock()
{
	libmaus::autoarray::AutoArray<char> data = libmaus::autoarray::AutoArray<char>::readFile("configure");
	uint64_t const tparts = 10;
	uint64_t const partsize = (data.size()+tparts-1)/tparts;
	uint64_t const parts = (data.size()+partsize-1)/partsize;
	libmaus::lz::ZlibCompressorObjectFactory zcfact;
	uint64_t low = 0;
	std::vector<libmaus::lz::SimpleCompressedStreamNamedInterval> intervals;
	
	std::vector<std::string> filenames;
	for ( uint64_t i = 0; i < parts; ++i )
	{
		uint64_t const rest = (data.size()-low);
		uint64_t const blocksize = std::min(partsize,rest);
		uint64_t const high = low + blocksize;
	
		std::ostringstream fnostr;
		fnostr << "tmp_" << std::setw(6) << std::setfill('0') << i;
		std::string const fn = fnostr.str();
		libmaus::util::TempFileRemovalContainer::addTempFile(fn);
		libmaus::aio::CheckedOutputStream COS(fn);
		libmaus::lz::SimpleCompressedOutputStream<std::ostream> zout(COS,zcfact);
		zout.put(0);
		std::pair<uint64_t,uint64_t> pre = zout.getOffset();
		zout.write(data.begin()+low,blocksize);
		std::pair<uint64_t,uint64_t> post = zout.getOffset();
		zout.put(0);
		zout.flush();
		COS.flush();
		
		low = high;
		
		intervals.push_back(libmaus::lz::SimpleCompressedStreamNamedInterval(std::pair<uint64_t,uint64_t>(0,0),std::pair<uint64_t,uint64_t>(0,0),"/dev/nullz"));
		intervals.push_back(libmaus::lz::SimpleCompressedStreamNamedInterval(pre,post,fn));
		intervals.push_back(libmaus::lz::SimpleCompressedStreamNamedInterval(std::pair<uint64_t,uint64_t>(0,0),std::pair<uint64_t,uint64_t>(0,0),"/dev/nullz"));
	}

	intervals.push_back(libmaus::lz::SimpleCompressedStreamNamedInterval(std::pair<uint64_t,uint64_t>(0,0),std::pair<uint64_t,uint64_t>(0,0),"/dev/nullz"));
	
	libmaus::lz::ZlibDecompressorObjectFactory zdfact;
	libmaus::lz::SimpleCompressedInputBlockConcatBlock block(zdfact);
	libmaus::lz::SimpleCompressedInputBlockConcat conc(intervals);
	
	do
	{
		conc.readBlock(block);
		block.uncompressBlock();
		
		std::cout.write(reinterpret_cast<char const *>(block.O.begin()),block.uncompsize);
		
	} while ( ! block.eof );
}

int main()
{
	try
	{
		// testInputBlock();
		testConcatInputBlock();		
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
