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

#include <libmaus/lz/GzipHeader.hpp>
#include <libmaus/util/GetFileSize.hpp>
#include <libmaus/lz/Deflate.hpp>
#include <libmaus/lz/Inflate.hpp>


int main(int argc, char *argv[])
{
	if ( argc < 2 )
		return EXIT_FAILURE;
	
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
