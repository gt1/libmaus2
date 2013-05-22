/**
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
**/
#include <libmaus/lz/BgzfDeflateBase.hpp>

#include <libmaus/lz/GzipHeader.hpp>
#include <libmaus/util/GetFileSize.hpp>
#include <libmaus/lz/Deflate.hpp>
#include <libmaus/lz/Inflate.hpp>
#include <libmaus/lz/BgzfInflateStream.hpp>
#include <libmaus/lz/BgzfDeflate.hpp>

void testBgzfRandom()
{
	srand(time(0));
	::libmaus::autoarray::AutoArray<uint8_t> R(16*1024*1024,false);
	for ( uint64_t i = 0; i < R.size(); ++i )
		R[i] = rand() % 256;
	std::ostringstream zostr;
	::libmaus::lz::BgzfDeflate<std::ostream> bdeflr(zostr);
	for ( uint64_t i = 0; i < R.size(); ++i )
		bdeflr.put(R[i]);
	bdeflr.addEOFBlock();
	std::istringstream ristr(zostr.str());
	::libmaus::lz::BgzfInflateStream rSW(ristr);
	int c = 0;
	uint64_t rp = 0;
	while ( (c=rSW.get()) >= 0 )
	{
		assert ( rp < R.size() );
		assert ( c == R[rp++] );
	}
	assert ( rp == R.size() );
}

void testBgzfMono()
{
	srand(time(0));
	::libmaus::autoarray::AutoArray<uint8_t> R(16*1024*1024,false);
	for ( uint64_t i = 0; i < R.size(); ++i )
		R[i] = 'a';
	std::ostringstream zostr;
	::libmaus::lz::BgzfDeflate<std::ostream> bdeflr(zostr);
	for ( uint64_t i = 0; i < R.size(); ++i )
		bdeflr.put(R[i]);
	bdeflr.addEOFBlock();
	std::istringstream ristr(zostr.str());
	::libmaus::lz::BgzfInflateStream rSW(ristr);
	int c = 0;
	uint64_t rp = 0;
	while ( (c=rSW.get()) >= 0 )
	{
		assert ( rp < R.size() );
		assert ( c == R[rp++] );
	}
	assert ( rp == R.size() );
}

#include <libmaus/lz/BgzfInflateParallelStream.hpp>
#include <libmaus/lz/BgzfDeflate.hpp>
#include <libmaus/lz/BgzfDeflateParallel.hpp>

int main(int argc, char *argv[])
{
	#if 1
	{
		::libmaus::lz::BgzfDeflateParallel BDP(std::cout,16,128);
		
		while ( std::cin )
		{
			libmaus::autoarray::AutoArray<char> B(16384);
			std::cin.read(B.begin(),B.size());
			int64_t const r = std::cin.gcount();
			
			BDP.write(B.begin(),r);
		}
		
		BDP.flush();
		std::cout.flush();
	}
	
	return 0;
	#endif

	#if 0
	{
		libmaus::lz::BgzfInflateParallel BIP(std::cin /* ,4,16 */);
		uint64_t c = 0;
		uint64_t b = 0;
		uint64_t d = 0;
		libmaus::timing::RealTimeClock rtc; rtc.start();
		libmaus::autoarray::AutoArray<uint8_t> adata(64*1024,false);
		
		while ( (d=BIP.read(reinterpret_cast<char *>(adata.begin()),adata.size())) != 0 )
		{
			b += d;
			if ( ++c % (16*1024) == 0 )
			{
				std::cerr << c << "\t" << b/(1024.0*1024.0*1024.0) << "\t" << static_cast<double>(b)/(1024.0*1024.0*rtc.getElapsedSeconds()) << " MB/s" << std::endl;
			}
		}
		
		std::cerr << c << "\t" << b/(1024.0*1024.0*1024.0) << "\t" << static_cast<double>(b)/(1024.0*1024.0*rtc.getElapsedSeconds()) << " MB/s" << std::endl;
		std::cerr << "decoded " << b << " bytes in " << rtc.getElapsedSeconds() << " seconds." << std::endl;
	}

	return 0;
	#endif

	std::cerr << "Testing random data on bgzf...";
	testBgzfRandom();
	std::cerr << "done." << std::endl;

	std::cerr << "Testing mono...";	
	testBgzfMono();
	std::cerr << "done." << std::endl;

	::libmaus::lz::BgzfDeflate<std::ostream> bdefl(std::cout);
	char const * str = "Hello, world.\n";
	bdefl.write(reinterpret_cast<char const *>(str),strlen(str));
	bdefl.flush();
	bdefl.write(reinterpret_cast<char const *>(str),strlen(str));
	bdefl.flush();
	bdefl.addEOFBlock();
	return 0;
	
	::libmaus::lz::BgzfInflateStream SW(std::cin);

	::libmaus::autoarray::AutoArray<char> BB(200,false);	
	while ( SW.read(BB.begin(),BB.size()) != 0 )
	{
	
	}

	if ( argc < 2 )
		return EXIT_FAILURE;
	
	
	return 0;
	
	#if 0
	::libmaus::lz::GzipHeader GZH(argv[1]);
	return 0;
	#endif

	std::ostringstream ostr;
	::libmaus::autoarray::AutoArray<uint8_t> message = ::libmaus::util::GetFileSize::readFile(argv[1]);
	
	std::cerr << "Deflating message of length " << message.size() << "...";
	::libmaus::lz::Deflate DEFL(ostr);
	DEFL.write ( reinterpret_cast<char const *>(message.begin()), message.size() );
	DEFL.flush();
	std::cerr << "done." << std::endl;
	
	std::cerr << "Checking output...";
	std::istringstream istr(ostr.str());
	::libmaus::lz::Inflate INFL(istr);
	int c;
	uint64_t i = 0;
	while ( (c=INFL.get()) >= 0 )
	{
		assert ( c == message[i] );
		i++;
	}
	std::cerr << "done." << std::endl;
	
	// std::cerr << "Message size " << message.size() << std::endl;
	
	std::string testfilename = "test";
	::libmaus::lz::BlockDeflate BD(testfilename);
	BD.write ( message.begin(), message.size() );
	BD.flush();
	
	uint64_t const decpos = message.size() / 3;
	::libmaus::lz::BlockInflate BI(testfilename,decpos);
	::libmaus::autoarray::AutoArray<uint8_t> dmessage (message.size(),false);
	uint64_t const red = BI.read(dmessage.begin()+decpos,dmessage.size());
	assert ( red == dmessage.size()-decpos );
	
	std::cerr << "(";
	for ( uint64_t i = decpos; i < message.size(); ++i )
		assert ( message[i] == dmessage[i] );
	std::cerr << ")\n";
	
	std::string shortmes1("123456789");
	std::string shortmes2("AA");
	std::string shortmes3("BB");
	std::string shortmes4("CC");
	
	std::string textfile1("test1");
	std::string textfile2("test2");
	std::string textfile3("test3");
	std::string textfile4("test4");
	
	::libmaus::lz::BlockDeflate BD1(textfile1);
	BD1.write ( reinterpret_cast<uint8_t const *>(shortmes1.c_str()), shortmes1.size() );
	BD1.flush();

	::libmaus::lz::BlockDeflate BD2(textfile2);
	BD2.write ( reinterpret_cast<uint8_t const *>(shortmes2.c_str()), shortmes2.size() );
	BD2.flush();

	::libmaus::lz::BlockDeflate BD3(textfile3);
	BD3.write ( reinterpret_cast<uint8_t const *>(shortmes3.c_str()), shortmes3.size() );
	BD3.flush();

	::libmaus::lz::BlockDeflate BD4(textfile4);
	BD4.write ( reinterpret_cast<uint8_t const *>(shortmes4.c_str()), shortmes4.size() );
	BD4.flush();
	
	std::vector < std::string > filenames;
	filenames.push_back(textfile1);
	filenames.push_back(textfile2);
	filenames.push_back(textfile3);
	filenames.push_back(textfile4);
	
	for ( uint64_t j = 0; j <= 15; ++j )
	{
		::libmaus::lz::ConcatBlockInflate CBI(filenames,j);

		for ( uint64_t i = 0; i < j; ++i )
			std::cerr << ' ';
		for ( uint64_t i = 0; i < CBI.n-j; ++i )
			std::cerr << (char)CBI.get();
		std::cerr << std::endl;
	}
		
	return 0;
}
