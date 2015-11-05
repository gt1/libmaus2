/*
    libmaus2
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
#include <libmaus2/lz/SimpleCompressedInputBlockConcat.hpp>

#include <libmaus2/lz/SimpleCompressedInputBlock.hpp>
#include <libmaus2/lz/SimpleCompressedOutputStream.hpp>
#include <libmaus2/lz/ZlibCompressorObjectFactory.hpp>
#include <libmaus2/lz/ZlibDecompressorObjectFactory.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/util/TempFileRemovalContainer.hpp>
#include <libmaus2/aio/OutputStreamInstance.hpp>
#include <sstream>

void testInputBlock()
{
	std::ostringstream ostr;
	libmaus2::lz::ZlibCompressorObjectFactory compfact;
	libmaus2::lz::SimpleCompressedOutputStream<std::ostream> compstr(ostr,compfact);
	libmaus2::autoarray::AutoArray<char> B(64*1024,false);

	while ( std::cin )
	{
		std::cin.read(B.begin(),B.size());
		if ( std::cin.gcount() )
			compstr.write(B.begin(),std::cin.gcount());
	}
	compstr.flush();
	ostr.flush();

	std::istringstream istr(ostr.str());
	libmaus2::lz::ZlibDecompressorObjectFactory decompfact;
	libmaus2::lz::SimpleCompressedInputBlock block(decompfact);

	while ( block.readBlock(istr) && ! block.eof )
	{
		bool const ok = block.uncompressBlock();
		assert ( ok );

		std::cout.write(reinterpret_cast<char const *>(block.O.begin()),block.uncompsize);
	}
}

void testConcatInputBlock()
{
	libmaus2::autoarray::AutoArray<char> data = libmaus2::autoarray::AutoArray<char>::readFile("configure");
	uint64_t const tparts = 10;
	uint64_t const partsize = (data.size()+tparts-1)/tparts;
	uint64_t const parts = (data.size()+partsize-1)/partsize;
	libmaus2::lz::ZlibCompressorObjectFactory zcfact;
	uint64_t low = 0;
	std::vector<libmaus2::lz::SimpleCompressedStreamNamedInterval> intervals;

	std::vector<std::string> filenames;
	for ( uint64_t i = 0; i < parts; ++i )
	{
		uint64_t const rest = (data.size()-low);
		uint64_t const blocksize = std::min(partsize,rest);
		uint64_t const high = low + blocksize;

		std::ostringstream fnostr;
		fnostr << "tmp_" << std::setw(6) << std::setfill('0') << i;
		std::string const fn = fnostr.str();
		libmaus2::util::TempFileRemovalContainer::addTempFile(fn);
		libmaus2::aio::OutputStreamInstance COS(fn);
		libmaus2::lz::SimpleCompressedOutputStream<std::ostream> zout(COS,zcfact);
		zout.put(0);
		std::pair<uint64_t,uint64_t> pre = zout.getOffset();
		zout.write(data.begin()+low,blocksize);
		std::pair<uint64_t,uint64_t> post = zout.getOffset();
		zout.put(0);
		zout.flush();
		COS.flush();

		low = high;

		intervals.push_back(libmaus2::lz::SimpleCompressedStreamNamedInterval(std::pair<uint64_t,uint64_t>(0,0),std::pair<uint64_t,uint64_t>(0,0),"/dev/nullz"));
		intervals.push_back(libmaus2::lz::SimpleCompressedStreamNamedInterval(pre,post,fn));
		intervals.push_back(libmaus2::lz::SimpleCompressedStreamNamedInterval(std::pair<uint64_t,uint64_t>(0,0),std::pair<uint64_t,uint64_t>(0,0),"/dev/nullz"));
	}

	intervals.push_back(libmaus2::lz::SimpleCompressedStreamNamedInterval(std::pair<uint64_t,uint64_t>(0,0),std::pair<uint64_t,uint64_t>(0,0),"/dev/nullz"));

	libmaus2::lz::ZlibDecompressorObjectFactory zdfact;
	libmaus2::lz::SimpleCompressedInputBlockConcatBlock block(zdfact);
	libmaus2::lz::SimpleCompressedInputBlockConcat conc(intervals);

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
