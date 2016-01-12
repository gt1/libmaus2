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
#include <libmaus2/math/UnsignedInteger.hpp>
#include <iostream>

#include <sstream>
#include <cassert>
#include <libmaus2/random/Random.hpp>

void testOutputLow32()
{
	std::cerr << "Checking number output for number [0,2^24)...";
	#if defined(_OPENMP)
	#pragma omp parallel for
	#endif
	for ( uint64_t i = 0; i < (1ull<<24); ++i )
	{
		libmaus2::math::UnsignedInteger<1> N(i);
		std::ostringstream ostru;
		ostru << N;
		std::ostringstream ostri;
		ostri << i;

		assert ( ostru.str() == ostri.str() );
	}
	std::cerr << "done." << std::endl;
}

void testOutputLow64()
{
	std::cerr << "Checking number output for number [2^32-2^24,2^32+2^24)...";
	#if defined(_OPENMP)
	#pragma omp parallel for
	#endif
	for ( uint64_t i = (1ull<<32)-(1ull<<24); i < (1ull<<32)+(1ull<<24); ++i )
	{
		libmaus2::math::UnsignedInteger<2> N(i);
		std::ostringstream ostru;
		ostru << N;
		std::ostringstream ostri;
		ostri << i;

		// std::cerr << ostru.str() << '\n';

		assert ( ostru.str() == ostri.str() );
	}
	std::cerr << "done." << std::endl;
}

#if defined(LIBMAUS2_HAVE_UNSIGNED_INT128)
template<size_t k>
void compare(libmaus2::math::UnsignedInteger<k> A, libmaus2::uint128_t vA)
{
	while ( vA )
	{
		unsigned int const dvA = vA % 10;
		unsigned int const dA = (A % libmaus2::math::UnsignedInteger<4>(10))[0];

		assert ( dvA == dA );

		vA /= 10;
		A /= libmaus2::math::UnsignedInteger<4>(10);
	}
}

void testrandconst128()
{
	for ( uint64_t i = 0; i < 64*1024; ++i )
	{
		uint64_t const lowa  = libmaus2::random::Random::rand64();
		uint64_t const higha = libmaus2::random::Random::rand64();

		libmaus2::math::UnsignedInteger<4> A =
			libmaus2::math::UnsignedInteger<4>(lowa) | (libmaus2::math::UnsignedInteger<4>(higha) << 64);
		libmaus2::uint128_t vA =
			static_cast<libmaus2::uint128_t>(lowa) | (static_cast<libmaus2::uint128_t>(higha)<<64);

		// std::cerr << A << std::endl;
		compare(A,vA);
	}
}

void testrandadd128()
{
	std::cerr << "Running addition test...";
	for ( uint64_t i = 0; i < 64*1024; ++i )
	{
		uint64_t const lowa  = libmaus2::random::Random::rand64();
		uint64_t const higha = libmaus2::random::Random::rand64();
		uint64_t const lowb  = libmaus2::random::Random::rand64();
		uint64_t const highb = libmaus2::random::Random::rand64();

		libmaus2::math::UnsignedInteger<4> A = libmaus2::math::UnsignedInteger<4>(lowa) | (libmaus2::math::UnsignedInteger<4>(higha) << 64);
		libmaus2::uint128_t vA = static_cast<libmaus2::uint128_t>(lowa) | (static_cast<libmaus2::uint128_t>(higha)<<64);

		libmaus2::math::UnsignedInteger<4> B = libmaus2::math::UnsignedInteger<4>(lowb) | (libmaus2::math::UnsignedInteger<4>(highb) << 64);
		libmaus2::uint128_t vB = static_cast<libmaus2::uint128_t>(lowb) | (static_cast<libmaus2::uint128_t>(highb)<<64);

		compare(A+B,vA+vB);

		libmaus2::math::UnsignedInteger<4> C = A+B;

		libmaus2::uint128_t tV =
			(static_cast<libmaus2::uint128_t>(C[0]) << 0) |
			(static_cast<libmaus2::uint128_t>(C[1]) << 32) |
			(static_cast<libmaus2::uint128_t>(C[2]) << 64) |
			(static_cast<libmaus2::uint128_t>(C[3]) << 96);

		assert ( tV == vA + vB );
	}
	std::cerr << "done." << std::endl;
}

void testrandmult128()
{
	std::cerr << "Running multiplication test...";
	for ( uint64_t i = 0; i < 64*1024; ++i )
	{
		uint64_t const lowa  = libmaus2::random::Random::rand64();
		uint64_t const higha = libmaus2::random::Random::rand64();
		uint64_t const lowb  = libmaus2::random::Random::rand64();
		uint64_t const highb = libmaus2::random::Random::rand64();

		libmaus2::math::UnsignedInteger<4> A = libmaus2::math::UnsignedInteger<4>(lowa) | (libmaus2::math::UnsignedInteger<4>(higha) << 64);
		libmaus2::uint128_t vA = static_cast<libmaus2::uint128_t>(lowa) | (static_cast<libmaus2::uint128_t>(higha)<<64);

		libmaus2::math::UnsignedInteger<4> B = libmaus2::math::UnsignedInteger<4>(lowb) | (libmaus2::math::UnsignedInteger<4>(highb) << 64);
		libmaus2::uint128_t vB = static_cast<libmaus2::uint128_t>(lowb) | (static_cast<libmaus2::uint128_t>(highb)<<64);

		compare(A,vA);
		compare(B,vB);

		compare(A*B,vA*vB);

		libmaus2::math::UnsignedInteger<4> C = A*B;

		libmaus2::uint128_t tV =
			(static_cast<libmaus2::uint128_t>(C[0]) << 0) |
			(static_cast<libmaus2::uint128_t>(C[1]) << 32) |
			(static_cast<libmaus2::uint128_t>(C[2]) << 64) |
			(static_cast<libmaus2::uint128_t>(C[3]) << 96);

		assert ( tV == vA * vB );
	}
	std::cerr << "done." << std::endl;
}

void testrandsub128()
{
	std::cerr << "Running subtraction test...";
	for ( uint64_t i = 0; i < 64*1024; ++i )
	{
		uint64_t const lowa  = libmaus2::random::Random::rand64();
		uint64_t const higha = libmaus2::random::Random::rand64();
		uint64_t const lowb  = libmaus2::random::Random::rand64();
		uint64_t const highb = libmaus2::random::Random::rand64();

		libmaus2::math::UnsignedInteger<4> A = libmaus2::math::UnsignedInteger<4>(lowa) | (libmaus2::math::UnsignedInteger<4>(higha) << 64);
		libmaus2::uint128_t vA = static_cast<libmaus2::uint128_t>(lowa) | (static_cast<libmaus2::uint128_t>(higha)<<64);

		libmaus2::math::UnsignedInteger<4> B = libmaus2::math::UnsignedInteger<4>(lowb) | (libmaus2::math::UnsignedInteger<4>(highb) << 64);
		libmaus2::uint128_t vB = static_cast<libmaus2::uint128_t>(lowb) | (static_cast<libmaus2::uint128_t>(highb)<<64);

		if ( vA < vB )
		{
			std::swap(vA,vB);
			std::swap(A,B);
		}

		compare(A,vA);
		compare(B,vB);

		compare(A-B,vA-vB);

		libmaus2::math::UnsignedInteger<4> C = A-B;

		libmaus2::uint128_t tV =
			(static_cast<libmaus2::uint128_t>(C[0]) << 0) |
			(static_cast<libmaus2::uint128_t>(C[1]) << 32) |
			(static_cast<libmaus2::uint128_t>(C[2]) << 64) |
			(static_cast<libmaus2::uint128_t>(C[3]) << 96);

		assert ( tV == vA - vB );
	}
	std::cerr << "done." << std::endl;
}

void testranddiv128()
{
	std::cerr << "Running division test...";
	for ( uint64_t i = 0; i < 64*1024; ++i )
	{
		uint64_t const lowa  = libmaus2::random::Random::rand64();
		uint64_t const higha = libmaus2::random::Random::rand64();
		uint64_t const lowb  = libmaus2::random::Random::rand64();
		uint64_t const highb = libmaus2::random::Random::rand64();

		libmaus2::math::UnsignedInteger<4> A = libmaus2::math::UnsignedInteger<4>(lowa) | (libmaus2::math::UnsignedInteger<4>(higha) << 64);
		libmaus2::uint128_t vA = static_cast<libmaus2::uint128_t>(lowa) | (static_cast<libmaus2::uint128_t>(higha)<<64);

		libmaus2::math::UnsignedInteger<4> B = libmaus2::math::UnsignedInteger<4>(lowb) | (libmaus2::math::UnsignedInteger<4>(highb) << 64);
		libmaus2::uint128_t vB = static_cast<libmaus2::uint128_t>(lowb) | (static_cast<libmaus2::uint128_t>(highb)<<64);

		if ( vA < vB )
		{
			std::swap(vA,vB);
			std::swap(A,B);
		}

		compare(A,vA);
		compare(B,vB);

		compare(A/B,vA/vB);

		libmaus2::math::UnsignedInteger<4> C = A/B;

		libmaus2::uint128_t tV =
			(static_cast<libmaus2::uint128_t>(C[0]) << 0) |
			(static_cast<libmaus2::uint128_t>(C[1]) << 32) |
			(static_cast<libmaus2::uint128_t>(C[2]) << 64) |
			(static_cast<libmaus2::uint128_t>(C[3]) << 96);

		assert ( tV == vA / vB );
	}
	std::cerr << "done." << std::endl;
}
#endif

int main(
	#if 0
	int argc,
	char * argv[]
	#endif
)
{
	libmaus2::random::Random::setup();

	{
		libmaus2::math::UnsignedInteger<4> U(1);
		for ( unsigned int i = 0; i < 8*sizeof(uint32_t)*4; ++i )
		{
			std::cerr << U << " " << U.clz() << std::endl;
			U <<= 1;
		}
	}

	#if defined(LIBMAUS2_HAVE_UNSIGNED_INT128)
	testranddiv128();
	testrandsub128();
	testrandmult128();
	testrandadd128();
	testrandconst128();
	#endif

	testOutputLow32();
	testOutputLow64();
}
