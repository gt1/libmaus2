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

#include <libmaus2/aio/SynchronousGenericInput.hpp>
#include <libmaus2/lz/SnappyStringInputStream.hpp>
#include <libmaus2/lz/SnappyOutputStream.hpp>
#include <libmaus2/lz/SnappyOffsetFileInputStream.hpp>
#include <libmaus2/lz/SnappyInputStream.hpp>
#include <libmaus2/lz/SnappyInputStreamArray.hpp>
#include <libmaus2/lz/SnappyInputStreamArrayFile.hpp>
#include <libmaus2/lz/SnappyFileOutputStream.hpp>
#include <libmaus2/lz/SnappyFileInputStream.hpp>
#include <libmaus2/lz/SnappyDecompressorObject.hpp>
#include <libmaus2/lz/SnappyDecompressorObjectFactory.hpp>
#include <libmaus2/lz/SnappyCompressorObject.hpp>
#include <libmaus2/lz/SnappyCompressorObjectFactory.hpp>
#include <libmaus2/lz/SnappyCompress.hpp>

void testSnappy()
{
	libmaus2::autoarray::AutoArray<char> A = ::libmaus2::aio::SynchronousGenericInput<char>::readArray("configure");
	std::string const message(A.begin(),A.end()) ;
	std::string const compressed = ::libmaus2::lz::SnappyCompress::compress(message);
	std::string const uncompressed = ::libmaus2::lz::SnappyCompress::uncompress(compressed);

	std::cerr << "single block test " << ((uncompressed==message)?"ok":"FAILED")
		<< " (uncompressed " << uncompressed.size() << " compressed " << compressed.size() << ")"
		<< std::endl;
}

void testSnappyStream()
{
	libmaus2::autoarray::AutoArray<char> A = ::libmaus2::aio::SynchronousGenericInput<char>::readArray("configure");
	std::string const message(A.begin(),A.end()) ;

	std::ostringstream ostr;
	::libmaus2::lz::SnappyOutputStream<std::ostringstream> SOS(ostr,64*1024);
	SOS.write(A.begin(),A.size());
	SOS.flush();
	ostr.flush();

	std::string const & streamdata = ostr.str();
	::libmaus2::lz::SnappyStringInputStream SIS(streamdata);

	std::ostringstream comp;
	int c = -1;
	while ( (c = SIS.get()) != -1 )
		comp.put(c);


	::libmaus2::lz::SnappyStringInputStream SISB(streamdata);
	libmaus2::autoarray::AutoArray<char> B(A.size(),false);
	char * p = B.begin();
	uint64_t d;
	while ( (d = SISB.read(p,64*1024)) != 0 )
		p += d;

	assert ( std::string(B.begin(),B.end()) == message );

	std::cerr << "block stream test " << ((comp.str()==message)?"ok":"FAILED") << std::endl;
}

void testSnappyStreamDual()
{
	libmaus2::autoarray::AutoArray<char> A = ::libmaus2::aio::SynchronousGenericInput<char>::readArray("configure");
	libmaus2::autoarray::AutoArray<char> B = ::libmaus2::aio::SynchronousGenericInput<char>::readArray("configure.in");

	std::string const messageA(A.begin(),A.end()) ;
	std::string const messageB(B.begin(),B.end()) ;

	std::vector < std::string const * > messages;

	std::vector<uint64_t> offsets;
	std::ostringstream ostr;

	offsets.push_back(ostr.tellp());
	::libmaus2::lz::SnappyOutputStream<std::ostringstream> SOSA(ostr,64*1024);
	SOSA.write(messageA.c_str(),messageA.size());
	messages.push_back(&messageA);
	SOSA.flush();

	offsets.push_back(ostr.tellp());
	::libmaus2::lz::SnappyOutputStream<std::ostringstream> SOSB(ostr,64*1024);
	SOSB.write(messageB.c_str(),messageB.size());
	messages.push_back(&messageB);
	SOSB.flush();

	offsets.push_back(ostr.tellp());

	ostr.flush();

	std::istringstream istr(ostr.str());
	::libmaus2::lz::SnappyInputStreamArray SISA(istr,offsets.begin(),offsets.end());

	bool ok = true;
	for ( uint64_t i = 0; ok && i+1 < offsets.size(); ++i )
	{
		std::ostringstream lostr;

		int c = -1;
		while ( (c=SISA[i].get()) >= 0 )
			lostr.put(c);

		ok = ok && ( lostr.str() == (*(messages[i])) );
	}

	std::cerr << "multi file block stream test " << (ok?"ok":"failed") << std::endl;
}

int main()
{
	try
	{
		testSnappy();
		testSnappyStream();
		testSnappyStreamDual();
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
