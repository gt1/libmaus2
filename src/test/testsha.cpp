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
#include <libmaus2/digest/DigestFactory.hpp>
#include <iostream>
#include <libmaus2/digest/Digests.hpp>
#include <libmaus2/util/ArgInfo.hpp>
#include <libmaus2/util/Demangle.hpp>
#include <libmaus2/util/GetFileSize.hpp>

#include <libmaus2/digest/sha256.h>
#include <libmaus2/digest/sha512.h>
#include <libmaus2/rank/BSwapBase.hpp>
#include <libmaus2/timing/RealTimeClock.hpp>
#include <libmaus2/random/Random.hpp>

#include <libmaus2/digest/CRC32.hpp>
#include <libmaus2/digest/CRC32C_sse42.hpp>
#include <libmaus2/digest/CRC32C.hpp>


template<typename crc>
void printCRC(uint8_t const * s, uint64_t const n, std::ostream & out)
{
	libmaus2::timing::RealTimeClock rtc; rtc.start();
	crc dig;
	dig.init();
	dig.update(s,n);
	out << libmaus2::util::Demangle::demangle<crc>() << "\t" << std::hex << dig.digestui() << std::dec;
	out << "\t" << rtc.getElapsedSeconds();
	out << std::endl;
}

template<typename crc>
void printCRCSingleByteUpdate(uint8_t const * s, uint64_t const n, std::ostream & out)
{
	libmaus2::timing::RealTimeClock rtc; rtc.start();
	crc dig;
	dig.init();
	for ( uint64_t i = 0; i < n; ++i )
		dig.update(s+i,1);
	out << libmaus2::util::Demangle::demangle<crc>() << "\t" << std::hex << dig.digestui() << std::dec;
	out << "\t" << rtc.getElapsedSeconds();
	out << std::endl;
}

void putBE64(uint8_t * p, uint64_t v)
{
	p[0] = (v>>56)&0xFF;
	p[1] = (v>>48)&0xFF;
	p[2] = (v>>40)&0xFF;
	p[3] = (v>>32)&0xFF;
	p[4] = (v>>24)&0xFF;
	p[5] = (v>>16)&0xFF;
	p[6] = (v>> 8)&0xFF;
	p[7] = (v>> 0)&0xFF;
}

void putBE128(uint8_t * p, uint64_t v)
{
	p[0] = 0;
	p[1] = 0;
	p[2] = 0;
	p[3] = 0;
	p[4] = 0;
	p[5] = 0;
	p[6] = 0;
	p[7] = 0;
	p[8] = (v>>56)&0xFF;
	p[9] = (v>>48)&0xFF;
	p[10] = (v>>40)&0xFF;
	p[11] = (v>>32)&0xFF;
	p[12] = (v>>24)&0xFF;
	p[13] = (v>>16)&0xFF;
	p[14] = (v>> 8)&0xFF;
	p[15] = (v>> 0)&0xFF;
}

typedef void (*sha2_func)(uint8_t const * text, uint32_t digest[8], uint64_t const numblocks);

void printCRCASM(uint8_t const * pa, size_t n, sha2_func const func, char const * name, std::ostream & out)
{
	libmaus2::timing::RealTimeClock rtc; rtc.start();

	// block size for sha256
	uint64_t const sha256_blocksize = 64;
	// number of blocks necessary to store data
	uint64_t const prenumblocks = (n + sha256_blocksize - 1) / sha256_blocksize;
	// rest to full block size
	uint64_t const restsize = prenumblocks*sha256_blocksize-n;
	// number of blocks (we need at least 9 bytes left, 1 for start of padding, 8 for size of message in bits)
	uint64_t const numblocks = (restsize >= 9) ? prenumblocks : (prenumblocks+1);
	// space for blocks
	::libmaus2::autoarray::AutoArray<uint8_t> B(numblocks * sha256_blocksize,false);

	// copy data
	std::copy(pa,pa+n,B.begin());
	// end "marker"
	B.at(n) = 0x80;
	// erase rest
	std::fill(B.begin()+n+1,B.end()-8,0);
	// put number of bits stored in last 8 bytes as big endian 64 bit number
	putBE64(B.end()-8,n << 3);

	// initial state for sha256
	uint32_t digest[8] =
	{
		0x6a09e667ul, 0xbb67ae85ul, 0x3c6ef372ul, 0xa54ff53aul,
		0x510e527ful, 0x9b05688cul, 0x1f83d9abul, 0x5be0cd19ul
	};

	func(B.begin(), &digest[0], numblocks);
	// sha256_avx(B.begin(), &digest[0], numblocks);

	out << name << "\t";
	for ( uint64_t i = 0; i < 8; ++i )
		out << std::hex << std::setw(8) << std::setfill('0') << (digest[i]) << std::dec;

	out << "\t" << rtc.getElapsedSeconds();

	out << std::endl;
}

void printCRCASMNoCopy(uint8_t const * A, size_t const n, sha2_func const func, char const * name, std::ostream & out)
{
	libmaus2::timing::RealTimeClock rtc; rtc.start();

	uint64_t const sha2_blocksize = 64;
	// full blocks
	uint64_t const fullblocks = n / sha2_blocksize;
	// rest bytes
	uint64_t const rest = n - fullblocks * sha2_blocksize;

	// initial state for sha256
	uint32_t digest[8] =
	{
		0x6a09e667ul, 0xbb67ae85ul, 0x3c6ef372ul, 0xa54ff53aul,
		0x510e527ful, 0x9b05688cul, 0x1f83d9abul, 0x5be0cd19ul
	};

	// process full blocks
	func(A, &digest[0], fullblocks);

	// temporary space (two input blocks)
	uint8_t temp[2*sha2_blocksize];

	// end of data
	uint8_t const * pe = A+n;
	// start of rest data
	uint8_t const * pa = pe - rest;

	// rest of message fits into next block (padding and length may not though)
	assert ( pe-pa < static_cast< ::std::ptrdiff_t >(sha2_blocksize) );
	// copy rest of data
	std::copy(pa,pe,&temp[0]);
	// put padding marker
	temp[rest] = 0x80;

	// space in first block
	uint64_t const firstblockspace = sha2_blocksize - rest;

	if ( firstblockspace >= 9 )
	{
		std::fill(&temp[rest]+1,&temp[sha2_blocksize-sizeof(uint64_t)],0);
		putBE64(&temp[sha2_blocksize-sizeof(uint64_t)],n<<3);
		func(&temp[0], &digest[0], 1);
	}
	else
	{
		std::fill(&temp[rest]+1,&temp[(2*sha2_blocksize)-sizeof(uint64_t)],0);
		putBE64(&temp[2*sha2_blocksize-sizeof(uint64_t)],n<<3);
		func(&temp[0], &digest[0], 2);
	}

	out << name << "\t";
	for ( uint64_t i = 0; i < 8; ++i )
		out << std::hex << std::setw(8) << std::setfill('0') << (digest[i]) << std::dec;

	out << "\t" << rtc.getElapsedSeconds();

	out << std::endl;
}

typedef void (*sha2_512_func)(uint8_t const * text, uint64_t digest[8], uint64_t const numblocks);

void printCRCASM512NoCopy(uint8_t const * A, size_t const n, sha2_512_func const func, char const * name, std::ostream & out)
{
	libmaus2::timing::RealTimeClock rtc; rtc.start();

	uint64_t const sha2_blocksize = 128;
	// full blocks
	uint64_t const fullblocks = n / sha2_blocksize;
	// rest bytes
	uint64_t const rest = n - fullblocks * sha2_blocksize;

	// initial state for sha256
	uint64_t digest[8] =
	{
		0x6A09E667F3BCC908ULL,0xBB67AE8584CAA73BULL,
		0x3C6EF372FE94F82BULL,0xA54FF53A5F1D36F1ULL,
		0x510E527FADE682D1ULL,0x9B05688C2B3E6C1FULL,
		0x1F83D9ABFB41BD6BULL,0x5BE0CD19137E2179ULL,
	};

	// process full blocks
	func(A, &digest[0], fullblocks);

	// temporary space (two input blocks)
	uint8_t temp[2*sha2_blocksize];

	// end of data
	uint8_t const * pe = A+n;
	// start of rest data
	uint8_t const * pa = pe - rest;

	// rest of message fits into next block (padding and length may not though)
	assert ( pe-pa < static_cast< ::std::ptrdiff_t>(sha2_blocksize) );
	// copy rest of data
	std::copy(pa,pe,&temp[0]);
	// put padding marker
	temp[rest] = 0x80;

	// space in first block
	uint64_t const firstblockspace = sha2_blocksize - rest;

	if ( firstblockspace >= 1+2*sizeof(uint64_t) )
	{
		std::fill(&temp[rest]+1,&temp[sha2_blocksize-2*sizeof(uint64_t)],0);
		putBE128(&temp[sha2_blocksize-2*sizeof(uint64_t)],n<<3);
		func(&temp[0], &digest[0], 1);
	}
	else
	{
		std::fill(&temp[rest]+1,&temp[(2*sha2_blocksize)-2*sizeof(uint64_t)],0);
		putBE128(&temp[2*sha2_blocksize-2*sizeof(uint64_t)],n<<3);
		func(&temp[0], &digest[0], 2);
	}

	out << name << "\t";
	for ( uint64_t i = 0; i < 8; ++i )
		out << std::hex << std::setw(16) << std::setfill('0') << (digest[i]) << std::dec;

	out << "\t" << rtc.getElapsedSeconds();

	out << std::endl;
}

std::string secondColumn(std::string const & s)
{
	uint64_t i = 0;

	while ( i != s.size() && s[i] != '\t' )
		++i;

	if ( i == s.size() )
		return std::string();

	++i;
	uint64_t const start = i;
	while ( i != s.size() && s[i] != '\t' )
		++i;

	return s.substr(start,i-start);
}

#if defined(LIBMAUS2_HAVE_SHA2_ASSEMBLY)
#include <libmaus2/digest/DigestFactory_SHA2_ASM.hpp>
#endif

#if defined(LIBMAUS2_HAVE_SMMINTRIN_H) && defined(HAVE_SSE4)
#include <libmaus2/digest/DigestFactory_CRC32C_SSE42.hpp>
#endif

#include <libmaus2/digest/DigestFactoryContainer.hpp>

int main(int argc, char * argv[])
{
	try
	{
		#if defined(LIBMAUS2_HAVE_SHA2_ASSEMBLY)
		libmaus2::digest::DigestFactoryContainer::addFactories(libmaus2::digest::DigestFactory_SHA2_ASM());
		#endif
		#if defined(LIBMAUS2_HAVE_SMMINTRIN_H) && defined(HAVE_SSE4)
		libmaus2::digest::DigestFactoryContainer::addFactories(libmaus2::digest::DigestFactory_CRC32C_SSE42());
		#endif

		libmaus2::util::ArgInfo const arginfo(argc,argv);

		{
			std::cerr << "digests " << libmaus2::digest::DigestFactoryContainer::getSupportedDigestsList() << std::endl;

			std::string const hash = arginfo.restargs.at(0);
			std::string const fn = arginfo.restargs.at(1);

			::libmaus2::autoarray::AutoArray<uint8_t> A = libmaus2::util::GetFileSize::readFile<uint8_t>(fn);

			libmaus2::digest::DigestInterface::unique_ptr_type Pdigest(libmaus2::digest::DigestFactoryContainer::construct(hash));

			Pdigest->vinit();
			Pdigest->vupdate(A.begin(),A.size());
			std::cout << Pdigest->vdigestAsString() << "\n";
		}

		return 0;


		#if defined(LIBMAUS2_HAVE_NETTLE) && defined(LIBMAUS2_USE_ASSEMBLY) && defined(LIBMAUS2_HAVE_i386)	&& defined(LIBMAUS2_HAVE_SHA2_ASSEMBLY)
		std::string ast(1024,'a');
		libmaus2::random::Random::setup(42);
		for ( uint64_t i = 0; i < ast.size(); ++i )
			ast[i] = libmaus2::random::Random::rand8();

		// test prefixes up to length of ast(1024)
		for ( uint64_t i = 0; i <= ast.size(); ++i )
		{
			std::string const s = ast.substr(0,i);
			::libmaus2::autoarray::AutoArray<uint8_t> A(s.size(),false);
			std::copy(s.begin(),s.end(),A.begin());

			std::ostringstream ostr0;
			printCRC<libmaus2::digest::SHA2_256>(A.begin(),A.size(),ostr0);
			std::string const s0 = secondColumn(ostr0.str());

			if ( libmaus2::util::I386CacheLineSize::hasSSE41() )
			{
				std::ostringstream ostr1;
				printCRCASM(A.begin(),A.size(), sha256_sse4, "sha256_sse4",ostr1);
				std::string const s1 = secondColumn(ostr1.str());
				std::ostringstream ostr2;
				printCRCASMNoCopy(A.begin(), A.size(), sha256_sse4, "sha256_sse4_nocopy",ostr2);
				std::string const s2 = secondColumn(ostr2.str());

				std::ostringstream ostr3;
				printCRC<libmaus2::digest::SHA2_256_sse4>(A.begin(),A.size(),ostr3);
				std::string const s3 = secondColumn(ostr3.str());

				std::ostringstream ostr4;
				printCRCSingleByteUpdate<libmaus2::digest::SHA2_256_sse4>(A.begin(),A.size(),ostr4);
				std::string const s4 = secondColumn(ostr4.str());

				bool ok = s0 == s1 && s0 == s2 && s0 == s3 && s0 == s4;

				if ( ! ok )
				{
					std::cerr << "failed for " << i << std::endl;
					std::cerr << s0 << std::endl;
					std::cerr << s1 << std::endl;
					std::cerr << s2 << std::endl;
					std::cerr << s3 << std::endl;

					assert ( s0 == s1 );
					assert ( s0 == s2 );
					assert ( s0 == s3 );
				}
			}
		}

		// test prefixes up to length of ast(1024)
		for ( uint64_t i = 0; i <= ast.size(); ++i )
		{
			std::string const s = ast.substr(0,i);
			::libmaus2::autoarray::AutoArray<uint8_t> A(s.size(),false);
			std::copy(s.begin(),s.end(),A.begin());

			std::ostringstream ostr0;
			printCRC<libmaus2::digest::SHA2_512>(A.begin(),A.size(),ostr0);
			std::string const s0 = secondColumn(ostr0.str());

			if ( libmaus2::util::I386CacheLineSize::hasSSE41() )
			{
				std::ostringstream ostr2;
				printCRCASM512NoCopy(A.begin(), A.size(), sha512_sse4, "sha512_sse4_nocopy",ostr2);
				std::string const s2 = secondColumn(ostr2.str());

				std::ostringstream ostr3;
				printCRC<libmaus2::digest::SHA2_512_sse4>(A.begin(),A.size(),ostr3);
				std::string const s3 = secondColumn(ostr3.str());

				std::ostringstream ostr4;
				printCRCSingleByteUpdate<libmaus2::digest::SHA2_512_sse4>(A.begin(),A.size(),ostr4);
				std::string const s4 = secondColumn(ostr4.str());

				bool ok = s0 == s2 && s0 == s3 && s0 == s4;

				if ( ! ok )
				{
					std::cerr << "failed for " << i << std::endl;
					std::cerr << s0 << std::endl;
					std::cerr << s2 << std::endl;
					std::cerr << s3 << std::endl;
					std::cerr << s4 << std::endl;

					assert ( s0 == s2 );
					assert ( s0 == s3 );
					assert ( s0 == s4 );
				}
			}
		}
		#endif

		for ( uint64_t i = 0; i < arginfo.restargs.size(); ++i )
		{
			::libmaus2::autoarray::AutoArray<uint8_t> const A = libmaus2::util::GetFileSize::readFile<uint8_t>(arginfo.restargs.at(i));
			std::string const text(A.begin(),A.end());

			std::ostream & out = std::cout;
			printCRC<libmaus2::digest::Null>(A.begin(),A.size(),out);
			printCRC<libmaus2::digest::CRC32>(A.begin(),A.size(),out);
			printCRC<libmaus2::digest::CRC32C>(A.begin(),A.size(),out);
			printCRC<libmaus2::util::MD5>(A.begin(),A.size(),out);

			#if defined(LIBMAUS2_HAVE_NETTLE)
			printCRC<libmaus2::digest::SHA1>(A.begin(),A.size(),out);
			printCRC<libmaus2::digest::SHA2_224>(A.begin(),A.size(),out);
			printCRC<libmaus2::digest::SHA2_256>(A.begin(),A.size(),out);
			printCRC<libmaus2::digest::SHA2_384>(A.begin(),A.size(),out);
			printCRC<libmaus2::digest::SHA2_512>(A.begin(),A.size(),out);
			#endif


			#if defined(LIBMAUS2_HAVE_SMMINTRIN_H) && defined(LIBMAUS2_USE_ASSEMBLY) && defined(LIBMAUS2_HAVE_x86_64) && defined(LIBMAUS2_HAVE_i386)
			if ( libmaus2::util::I386CacheLineSize::hasSSE42() )
			{
				printCRC<libmaus2::digest::CRC32C_sse42>(A.begin(),A.size(),out);
			}
			#endif

			#if defined(LIBMAUS2_USE_ASSEMBLY) && defined(LIBMAUS2_HAVE_i386)	&& defined(LIBMAUS2_HAVE_SHA2_ASSEMBLY)
			if ( libmaus2::util::I386CacheLineSize::hasSSE41() )
			{
				printCRC<libmaus2::digest::SHA2_256_sse4>(A.begin(),A.size(),out);

				printCRCASM(A.begin(),A.size(), sha256_sse4, "sha256_sse4",out);
				printCRCASMNoCopy(A.begin(), A.size(), sha256_sse4, "sha256_sse4_nocopy",out);

				printCRCASM512NoCopy(A.begin(), A.size(), sha512_sse4, "sha512_sse4_nocopy", out);
				printCRC<libmaus2::digest::SHA2_512_sse4>(A.begin(),A.size(),out);
			}
			if ( libmaus2::util::I386CacheLineSize::hasAVX() )
			{
				printCRCASM(A.begin(),A.size(), sha256_avx, "sha256_avx",out);
				printCRCASMNoCopy(A.begin(), A.size(), sha256_avx, "sha256_avx_nocopy",out);
			}
			#if 0
			printCRCASM(A, sha256_rorx, "sha256_rorx",out);
			printCRCASM(A, sha256_rorx_x8ms, "sha256_rorx_x8ms",out);
			#endif
			#endif
		}

		#if ! defined(LIBMAUS2_HAVE_NETTLE)
		libmaus2::exception::LibMausException lme;
		lme.getStream() << "support for nettle library is not present" << std::endl;
		lme.finish();
		throw lme;
		#endif
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
