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

#include <libmaus2/aio/AsynchronousBufferReader.hpp>
#include <libmaus2/aio/AsynchronousWriter.hpp>
#include <libmaus2/aio/LineSplittingPosixFdOutputStream.hpp>
#include <libmaus2/aio/LinuxStreamingPosixFdOutputStream.hpp>
#include <libmaus2/aio/PosixFdInputStream.hpp>
#include <libmaus2/aio/PosixFdOutputStream.hpp>
#include <libmaus2/timing/RealTimeClock.hpp>
#include <libmaus2/aio/InputStreamInstance.hpp>

#include <vector>
#include <map>
#include <cmath>

#include <libmaus2/aio/PosixFdInputOutputStream.hpp>

#include <libmaus2/aio/InputStreamInstance.hpp>

void testPosixFdInput()
{
	::libmaus2::autoarray::AutoArray<unsigned char> A = libmaus2::util::GetFileSize::readFile("configure");
	libmaus2::aio::PosixFdInputStream PFIS("configure",64*1024);
	
	PFIS.clear();
	PFIS.seekg(0,std::ios::end);
	assert ( static_cast<int64_t>(PFIS.tellg()) == static_cast<int64_t>(A.size()) );
	PFIS.clear();
	PFIS.seekg(0,std::ios::beg);
	uint64_t const inc = 127;
	
	for ( uint64_t i = 0; i <= A.size(); i += std::min(inc,A.size()-i) )
	{
		PFIS.clear();
		PFIS.seekg(i,std::ios::beg);
	
		int c = -1;
		uint64_t p = i;
		while ( ( c = PFIS.get() ) != std::istream::traits_type::eof() )
		{
			assert ( c == A[p++] );
		}
		
		assert ( p == A.size() );
	
		// if ( (i & (1024-1)) == 0 )
			std::cerr << "i=" << i << std::endl;
			
		if ( i == A.size() )
			break;			
	}	
}

#include <libmaus2/aio/MemoryFileContainer.hpp>
#include <libmaus2/aio/MemoryInputOutputStream.hpp>
#include <libmaus2/aio/MemoryOutputStream.hpp>

template<typename stream_type, typename output_stream_type, typename input_stream_type>
void testInputOutput()
{
	{
		stream_type PFIOS("t",std::ios::in|std::ios::out|std::ios::binary|std::ios::trunc);
		PFIOS.put('a');
		std::cerr << PFIOS.tellp() << std::endl;
		PFIOS.seekg(0,std::ios::beg);
		std::cerr << (char)PFIOS.get() << std::endl;
		PFIOS.seekg(0);
		std::cerr << (char)PFIOS.get() << std::endl;
		
		PFIOS.seekp(0,std::ios::beg);
		PFIOS.put('b');
		std::cerr << PFIOS.tellp() << std::endl;
		PFIOS.seekg(0,std::ios::beg);
		std::cerr << (char)PFIOS.get() << std::endl;
		PFIOS.seekg(0);
		std::cerr << (char)PFIOS.get() << std::endl;

		PFIOS.seekp(0,std::ios::beg);
		PFIOS.put('c');
		PFIOS.put('d');
		std::cerr << PFIOS.tellp() << std::endl;
		PFIOS.seekg(0);
		int c;
		while ( (c=PFIOS.get()) != std::iostream::traits_type::eof() )
		{
			std::cerr << (char) c << std::endl;
		}

		PFIOS.clear();
		
		std::cerr << "---" << std::endl;
		
		PFIOS.seekp(2,std::ios::beg);
		PFIOS.put('e');
		PFIOS.put('f');
		PFIOS.put('g');
		std::cerr << PFIOS.tellp() << std::endl;
		PFIOS.seekg(0);
		// int c;
		while ( (c=PFIOS.get()) != std::iostream::traits_type::eof() )
		{
			std::cerr << (char) c << std::endl;
		}
		
		PFIOS.clear();
	}
	
	std::cerr << "-----" << std::endl;

	{
		stream_type PFIOS("t",std::ios::in|std::ios::out|std::ios::binary);

		int c;
		while ( (c=PFIOS.get()) != std::iostream::traits_type::eof() )
		{
			std::cerr << (char) c << std::endl;
		}
	}
	
	std::cerr << std::string("---") << std::endl;

	{
		stream_type PFIOS("t",std::ios::in|std::ios::out|std::ios::binary);

		PFIOS.seekg(0,std::ios::end);
		
		int64_t const l = PFIOS.tellg();

		for ( int64_t i = 0; i < l; ++i )
		{
			PFIOS.seekp(-(i+1),std::ios::end);
			PFIOS.put('a'+i);
		}
		
		PFIOS.seekg(0);
		int c;
		while ( (c=PFIOS.get()) != std::iostream::traits_type::eof() )
		{
			std::cerr << (char) c << std::endl;
		}
		
		PFIOS.clear();
	}

	std::cerr << std::string("---") << std::endl;
	
	{
		output_stream_type PFOS("t");
		PFOS.put('1');
		PFOS.put('2');
		PFOS.put('3');
		PFOS.put('4');
		PFOS.put('5');
	}

	{
		stream_type PFIOS("t",std::ios::in|std::ios::out|std::ios::binary);

		int c;
		while ( (c=PFIOS.get()) != std::iostream::traits_type::eof() )
		{
			std::cerr << (char) c << std::endl;
		}
	}

	std::cerr << std::string("***") << std::endl;

	{
		input_stream_type PFIOS("t");

		int c;
		while ( (c=PFIOS.get()) != std::iostream::traits_type::eof() )
		{
			std::cerr << (char) c << std::endl;
		}
	}
}

#include <libmaus2/aio/MemoryOutputStreamBuffer.hpp>
#include <libmaus2/aio/MemoryInputStreamBuffer.hpp>
#include <libmaus2/aio/MemoryInputStream.hpp>
#include <libmaus2/aio/FileRemoval.hpp>

int main(int argc, char * argv[])
{
	{
	libmaus2::aio::InputStreamInstance in("configure");
	int c = -1;
	while ( (c=in.get()) != std::istream::traits_type::eof() )
		std::cout.put(c);
	
	// return 0;
	}
	
	// testInputOutput<libmaus2::aio::PosixFdInputOutputStream>();
	testInputOutput<libmaus2::aio::MemoryInputOutputStream,libmaus2::aio::MemoryOutputStream,libmaus2::aio::MemoryInputStream>();
	
	return 0;

	{
		libmaus2::aio::LineSplittingPosixFdOutputStream LSOUT("split",4,32);
		for ( uint64_t i = 0; i < 17; ++i )
		{
			LSOUT << "line_" << i << "\n";
		}
	}

	{
		libmaus2::aio::LineSplittingPosixFdOutputStream LSOUT("nosplit",4,32);
	}
		
	{
		std::string const fn = "nonexistant.test.file";
		std::string const text1 = "Hello world.";
		std::string const text2 = "new text";
		
		{
			libmaus2::aio::PosixFdOutputStream PFOS(fn);
			PFOS << text1;
			PFOS.flush();
		}
		
		{
			libmaus2::aio::InputStreamInstance CIS(fn);
			libmaus2::autoarray::AutoArray<char> C(text1.size());
			CIS.read(C.begin(),C.size());
			assert ( CIS.get() < 0 );
			assert ( strncmp ( text1.c_str(), C.begin(), C.size() ) == 0 );
		}

		{
			libmaus2::aio::PosixFdOutputStream PFOS(fn);
			PFOS << text2;
			PFOS.flush();
		}
		
		{
			libmaus2::aio::InputStreamInstance CIS(fn);
			libmaus2::autoarray::AutoArray<char> C(text2.size());
			CIS.read(C.begin(),C.size());
			assert ( CIS.get() < 0 );
			assert ( strncmp ( text2.c_str(), C.begin(), C.size() ) == 0 );
		}
		
		remove(fn.c_str());
	}

	{
		std::string const fn = "nonexistant.test.file";
		std::string const text1 = "Hello world.";
		std::string const text2 = "new text";
		
		{
			libmaus2::aio::LinuxStreamingPosixFdOutputStream PFOS(fn);
			PFOS << text1;
			PFOS.flush();
		}
		
		{
			libmaus2::aio::InputStreamInstance CIS(fn);
			libmaus2::autoarray::AutoArray<char> C(text1.size());
			CIS.read(C.begin(),C.size());
			assert ( CIS.get() < 0 );
			assert ( strncmp ( text1.c_str(), C.begin(), C.size() ) == 0 );
		}

		{
			libmaus2::aio::LinuxStreamingPosixFdOutputStream PFOS(fn);
			PFOS << text2;
			PFOS.flush();
		}
		
		{
			libmaus2::aio::InputStreamInstance CIS(fn);
			libmaus2::autoarray::AutoArray<char> C(text2.size());
			CIS.read(C.begin(),C.size());
			assert ( CIS.get() < 0 );
			assert ( strncmp ( text2.c_str(), C.begin(), C.size() ) == 0 );
		}
		
		remove(fn.c_str());
	}
	
	// test putback buffer in PosixFdInputStream
	{
		std::string const fn = "configure";
		uint64_t const fs = libmaus2::util::GetFileSize::getFileSize(fn);
		libmaus2::autoarray::AutoArray<char> A(fs,false);
		
		{
			libmaus2::aio::InputStreamInstance CIS(fn);
			CIS.read(A.begin(),fs);
		}
		
		uint64_t const putbacksize = 2048;
		
		if ( fs >= putbacksize )
		{
			libmaus2::aio::PosixFdInputStream PFIS(fn,64*1024,putbacksize);
			
			for ( uint64_t z = 0; z < (fs-putbacksize+1); ++z )
			{
				for ( uint64_t i = 0 ; i < putbacksize; ++i )
				{
					int const c = PFIS.get();
					assert ( c >= 0 );
					assert ( c == A[z+i] );
				}
				
				PFIS.clear();
				
				for ( int64_t i = putbacksize-1; i >= 1; --i )
					PFIS.putback(A[z+i]);
			}
		}
	}

	testPosixFdInput();
	
	if ( argc < 3 )
	{
		std::cerr << "usage: " << argv[0] << " <in0> <in1> ... <out>" << std::endl;
		return EXIT_FAILURE;
	}
	
	try
	{
		::libmaus2::timing::RealTimeClock rtc;
		rtc.start();

		std::list<std::string> filenames;
		for ( int i = 1; i+1 < argc; ++i )
			filenames.push_back(argv[i]);

		// ::libmaus2::aio::AsynchronousBufferReader ABR(argv[1],16,1ull << 18);	
		::libmaus2::aio::AsynchronousBufferReaderList ABR(filenames.begin(),filenames.end(),16,1ull << 18);	
		::libmaus2::aio::AsynchronousWriter AW(argv[argc-1]);	
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
			
		testPosixFdInput();
	}
	catch(std::exception const & ex)
	{
		std::cerr << "exception: " << ex.what() << std::endl;
	}
}
