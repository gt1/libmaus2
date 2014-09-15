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

#include <libmaus/lz/GzipOutputStream.hpp>

#include <libmaus/lz/BgzfInflateDeflateParallel.hpp>
#include <libmaus/lz/BgzfInflateDeflateParallelThread.hpp>

#include <libmaus/lz/GzipHeader.hpp>
#include <libmaus/util/GetFileSize.hpp>
#include <libmaus/lz/Deflate.hpp>
#include <libmaus/lz/Inflate.hpp>
#include <libmaus/lz/BgzfInflateStream.hpp>
#include <libmaus/lz/BgzfDeflate.hpp>
#include <libmaus/lz/BgzfDeflateParallel.hpp>

#include <libmaus/lz/LineSplittingGzipOutputStream.hpp>

void testBgzfRandom()
{
	srand(time(0));
	::libmaus::autoarray::AutoArray<uint8_t> R(16*1024*1024,false);
	for ( uint64_t i = 0; i < R.size(); ++i )
		R[i] = rand() % 256;
	std::ostringstream zostr;
	::libmaus::lz::BgzfDeflate<std::ostream> bdeflr(zostr);
	// ::libmaus::lz::BgzfDeflateParallel bdeflr(zostr,8,64,-1);
	for ( uint64_t i = 0; i < R.size(); ++i )
		bdeflr.put(R[i]);
	bdeflr.addEOFBlock();
	// bdeflr.flush();
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
#include <libmaus/bambam/BamHeader.hpp>

#include <libmaus/util/ContainerGetObject.hpp>
#include <libmaus/lz/BgzfRecode.hpp>
#include <libmaus/bambam/BamHeader.hpp>

void maskBamDuplicateFlag(std::istream & in, std::ostream & out, bool const verbose = true)
{
	libmaus::timing::RealTimeClock rtc; rtc.start();
	libmaus::timing::RealTimeClock lrtc; lrtc.start();
	libmaus::lz::BgzfRecode rec(in,out);
	
	bool haveheader = false;
	uint64_t blockskip = 0;
	std::vector<uint8_t> headerstr;
	uint64_t preblocksizes = 0;
	
	/* read and copy blocks until we have reached the end of the BAM header */
	while ( (!haveheader) && rec.getBlock() )
	{
		std::copy ( rec.deflatebase.pa, rec.deflatebase.pa + rec.P.second,
			std::back_insert_iterator < std::vector<uint8_t> > (headerstr) );
		
		try
		{
			libmaus::util::ContainerGetObject< std::vector<uint8_t> > CGO(headerstr);
			libmaus::bambam::BamHeader header;
			header.init(CGO);
			haveheader = true;
			blockskip = CGO.i - preblocksizes;
		}
		catch(std::exception const & ex)
		{
			std::cerr << "[D] " << ex.what() << std::endl;
		}
	
		if ( ! haveheader )
		{
			preblocksizes += rec.P.second;
			rec.putBlock();
		}
	}

	/* parser state types and variables */
	enum parsestate { state_reading_blocklen, state_pre_skip, state_marking, state_post_skip };
	parsestate state = state_reading_blocklen;
	unsigned int blocklenred = 0;
	uint32_t blocklen = 0;
	uint32_t preskip = 0;
	uint64_t alcnt = 0;
	unsigned int const dupflagskip = 15;
	uint8_t const dupflagmask = static_cast<uint8_t>(~(4u));
			
	/* while we have alignment data blocks */
	while ( rec.P.second )
	{
		uint8_t * pa       = rec.deflatebase.pa + blockskip;
		uint8_t * const pc = rec.deflatebase.pc;

		while ( pa != pc )
			switch ( state )
			{
				/* read length of next alignment block */
				case state_reading_blocklen:
					/* if this is a little endian machine allowing unaligned access */
					#if defined(LIBMAUS_HAVE_i386)
					if ( (!blocklenred) && ((pc-pa) >= static_cast<ptrdiff_t>(sizeof(uint32_t))) )
					{
						blocklen = *(reinterpret_cast<uint32_t const *>(pa));
						blocklenred = sizeof(uint32_t);
						pa += sizeof(uint32_t);
						
						state = state_pre_skip;
						preskip = dupflagskip;
					}
					else
					#endif
					{
						while ( pa != pc && blocklenred < sizeof(uint32_t) )
							blocklen |= static_cast<uint32_t>(*(pa++)) << ((blocklenred++)*8);

						if ( blocklenred == sizeof(uint32_t) )
						{
							state = state_pre_skip;
							preskip = dupflagskip;
						}
					}
					break;
				/* skip data before the part we modify */
				case state_pre_skip:
					{
						uint32_t const skip = std::min(pc-pa,static_cast<ptrdiff_t>(preskip));
						pa += skip;
						preskip -= skip;
						blocklen -= skip;
						
						if ( ! skip )
							state = state_marking;
					}
					break;
				/* change data */
				case state_marking:
					assert ( pa != pc );
					*pa &= dupflagmask;
					state = state_post_skip;
					// intented fall through to post_skip case
				/* skip data after part we modify */
				case state_post_skip:
				{
					uint32_t const skip = std::min(pc-pa,static_cast<ptrdiff_t>(blocklen));
					pa += skip;
					blocklen -= skip;

					if ( ! blocklen )
					{
						state = state_reading_blocklen;
						blocklenred = 0;
						blocklen = 0;
						alcnt++;
						
						if ( verbose && ((alcnt & (1024*1024-1)) == 0) )
						{
							std::cerr 
								<< "[V] " << alcnt 
								<< " "
								<< (alcnt / rtc.getElapsedSeconds())
								<< " "
								<< rtc.getElapsedSeconds()
								<< " "
								<< lrtc.getElapsedSeconds()
								<< std::endl;
							
							lrtc.start();
						}
					}
					break;
				}
			}

		blockskip = 0;
		
		rec.putBlock();			
		rec.getBlock();
	}
			
	rec.addEOFBlock();
	std::cout.flush();
	
	if ( verbose )
		std::cerr << "[V] Time " << rtc.getElapsedSeconds() << " alcnt " << alcnt << std::endl;
}

#include <libmaus/lz/Lz4CompressStream.hpp>
#include <libmaus/lz/Lz4Decoder.hpp>
#include <libmaus/random/Random.hpp>

void testlz4()
{
	std::ostringstream ostr;
	
	{
		libmaus::lz::Lz4CompressStream compressor(ostr,16*1024);
		libmaus::aio::CheckedInputStream CIS("configure");
		int c;
		while ( (c=CIS.get()) > 0 )
			compressor.put(c);
		compressor.writeIndex();
	}

	libmaus::autoarray::AutoArray<char> const C = libmaus::autoarray::AutoArray<char>::readFile("configure");

	std::istringstream istr(ostr.str());
	libmaus::lz::Lz4Decoder dec(istr);
	
	{

		for ( uint64_t i = 0; i < C.size(); i += 100 )
		{
			if ( i % 16 == 0 )
				std::cerr << "i=" <<i << std::endl;
		
			int c;
			dec.clear();
			dec.seekg(i);
			uint64_t j = i;
			while ( (c=dec.get()) > 0 )
			{
				assert ( c == static_cast<uint8_t>(C[j++]) );
			}
		}
			
		uint64_t i = C.size()-1;
		int c;
		dec.clear();
		dec.seekg(i);
		uint64_t j = i;
		while ( (c=dec.get()) > 0 )
		{
			assert ( c == static_cast<uint8_t>(C[j++]) );
		}
	}
	
	libmaus::random::Random::setup(time(0));
	
	dec.clear();
	for ( uint64_t j = 0; j < 16384; ++j )
	{
		uint64_t const r = 10;
		uint64_t const p = libmaus::random::Random::rand64() % ( C.size()-r );
		
		dec.seekg(p);
		for ( uint64_t i = 0; i < r; ++i )
		{
			assert ( dec.get() == static_cast<uint8_t>(C[p+i]) );
		}
	}
}

#include <libmaus/lz/BufferedGzipStream.hpp>

void testGzip()
{
	libmaus::aio::CheckedInputStream CIS("configure");
	uint64_t t = 0;
	std::ostringstream ostr;
	{
		libmaus::lz::GzipOutputStream GZOS(ostr);
		int c = -1;
		while ( ( c = CIS.get() ) >= 0 )
			GZOS.put(c);
			
		t = GZOS.terminate();		
	}
	
	CIS.clear();
	CIS.seekg(0);
	
	assert ( t == ostr.str().size() );
	
	std::istringstream istr(ostr.str());
	libmaus::lz::BufferedGzipStream BGS(istr);
	
	int c = -1;
	while ( (c=CIS.get()) >= 0 )
	{
		int d = BGS.get();
		assert ( d == c );
	}
	assert ( BGS.get() < 0 );
}

int main(int argc, char *argv[])
{
	{
		libmaus::lz::LineSplittingGzipOutputStream LSG("gzsplit",4,17);
		
		for ( uint64_t i = 0; i < 17; ++i )
			LSG << "line_" << i << "\n";		
	}

	testGzip();
	testlz4();

	#if 0
	maskBamDuplicateFlag(std::cin,std::cout);
	return 0;
	#endif

	#if 0
	{
		libmaus::lz::BgzfInflateDeflateParallel BIDP(std::cin,std::cout,Z_DEFAULT_COMPRESSION,32,128);
		libmaus::autoarray::AutoArray<char> B(64*1024,false);
		int r;
		uint64_t t = 0;
		uint64_t last = std::numeric_limits<uint64_t>::max();
		uint64_t lcnt = 0;
		uint64_t const mod = 64*1024*1024;
		libmaus::timing::RealTimeClock rtc; rtc.start();
		libmaus::timing::RealTimeClock lrtc; lrtc.start();

		while ( (r = BIDP.read(B.begin(),B.size())) )
		{
			BIDP.write(B.begin(),r);
			
			lcnt += r;
			t += r;
			
			if ( t/mod != last/mod )
			{
				if ( isatty(STDERR_FILENO) )
					std::cerr 
						<< "\r" << std::string(60,' ') << "\r";

				std::cerr
						<< rtc.formatTime(rtc.getElapsedSeconds()) << " " << t/(1024*1024) << "MB, " << (lcnt/lrtc.getElapsedSeconds())/(1024.0*1024.0) << "MB/s";
				
				if ( isatty(STDERR_FILENO) )
					std::cerr << std::flush;
				else
					std::cerr << std::endl;
				
				lrtc.start();
				last = t;
				lcnt = 0;
			}
		}

		if ( isatty(STDERR_FILENO) )
			std::cerr 
				<< "\r" << std::string(60,' ') << "\r";

		std::cerr
				<< rtc.formatTime(rtc.getElapsedSeconds()) << " " << t/(1024*1024) << "MB, " << (t/rtc.getElapsedSeconds())/(1024.0*1024.0) << "MB/s";
				
		std::cerr << std::endl;

			
		return 0;
	}
	#endif                                                                                                                                                                            

	#if 0
	{
		::libmaus::lz::BgzfDeflateParallel BDP(std::cout,32,128,Z_DEFAULT_COMPRESSION);
		
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
		try
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
		catch(std::exception const & ex)
		{
			std::cerr << ex.what() << std::endl;
			return EXIT_FAILURE;
		}
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
	while ( SW.read(BB.begin(),BB.size()) )
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
