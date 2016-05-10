/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#include <libmaus2/huffman/SymCountEncoder.hpp>
#include <libmaus2/huffman/SymCountDecoder.hpp>
#include <libmaus2/random/Random.hpp>
#include <libmaus2/huffman/SymBitDecoder.hpp>
#include <libmaus2/huffman/SymBitEncoder.hpp>

void testsymcnt()
{
	std::string const fn = "mem://tmp";

	uint64_t const n = 16*1024+37;
	libmaus2::autoarray::AutoArray<libmaus2::huffman::SymCount> ASC(n);

	for ( uint64_t i = 0; i < n; ++i )
	{
		ASC[i].sbit = libmaus2::random::Random::rand64() % 2;
	}
	for ( uint64_t i = 0; i < n; )
	{
		uint64_t const z = libmaus2::random::Random::rand64() % 64;
		uint64_t const zz = std::min(n-i,z);

		ASC[i].cnt = zz;

		i += zz;
	}
	for ( uint64_t i = 0; i < n; )
	{
		uint64_t const z = libmaus2::random::Random::rand64() % 64;
		uint64_t const zz = std::min(n-i,z);
		uint64_t const sym = libmaus2::random::Random::rand64() % 31;

		for ( uint64_t j = 0; j < zz; ++j )
			ASC[i++].sym = sym;
	}

	libmaus2::huffman::SymCountEncoderStd::unique_ptr_type senc(new libmaus2::huffman::SymCountEncoderStd(fn,512));
	for ( uint64_t i = 0; i < n; ++i )
		senc->encode(ASC[i]);
	senc.reset();

	assert ( libmaus2::huffman::SymCountDecoder::getLength(fn) == n );

	for ( uint64_t j = 0; j < n; ++j )
	{
		libmaus2::huffman::SymCountDecoder sdec(std::vector<std::string>(1,fn),j,1 /* numthreads */);
		libmaus2::huffman::SymCount SC;
		uint64_t i = 0;
		for ( ; sdec.decode(SC); ++i )
		{
			// std::cerr << SC.sym << "," << SC.cnt << "," << SC.sbit << std::endl;
			assert ( SC == ASC[i+j] );
		}
		assert ( i+j == n );
	}
}

void testsymbit()
{
	std::string const fn = "mem://tmp";

	uint64_t const n = 16*1024+37;
	libmaus2::autoarray::AutoArray<libmaus2::huffman::SymBit> ASC(n);

	for ( uint64_t i = 0; i < n; ++i )
	{
		ASC[i].sbit = libmaus2::random::Random::rand64() % 2;
	}
	for ( uint64_t i = 0; i < n; )
	{
		uint64_t const z = libmaus2::random::Random::rand64() % 64;
		uint64_t const zz = std::min(n-i,z);

		// ASC[i].cnt = zz;

		i += zz;
	}
	for ( uint64_t i = 0; i < n; )
	{
		uint64_t const z = libmaus2::random::Random::rand64() % 64;
		uint64_t const zz = std::min(n-i,z);
		uint64_t const sym = libmaus2::random::Random::rand64() % 31;

		for ( uint64_t j = 0; j < zz; ++j )
			ASC[i++].sym = sym;
	}

	libmaus2::huffman::SymBitEncoderStd::unique_ptr_type senc(new libmaus2::huffman::SymBitEncoderStd(fn,512));
	for ( uint64_t i = 0; i < n; ++i )
		senc->encode(ASC[i]);
	senc.reset();

	assert ( libmaus2::huffman::SymBitDecoder::getLength(fn) == n );

	for ( uint64_t j = 0; j < n; ++j )
	{
		libmaus2::huffman::SymBitDecoder sdec(std::vector<std::string>(1,fn),j,1 /* numthreads */);
		libmaus2::huffman::SymBit SC;
		uint64_t i = 0;
		for ( ; sdec.decode(SC); ++i )
		{
			// std::cerr << SC.sym << "," << SC.cnt << "," << SC.sbit << std::endl;
			assert ( SC == ASC[i+j] );
		}
		assert ( i+j == n );
	}
}

int main()
{
	try
	{
		testsymbit();
		testsymcnt();
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
