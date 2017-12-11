/*
    libmaus2
    Copyright (C) 2017 German Tischler

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

#include <libmaus2/math/Convolution.hpp>
#include <libmaus2/util/ArgParser.hpp>

static std::vector < double > getInitialPI(double const p_i)
{
	double const q_i = 1.0 - p_i;
	double f_i = q_i;

	std::vector < double > P_I;
	while ( f_i >= 1e-7 )
	{
		P_I.push_back(f_i);
		f_i *= p_i;
	}

	return P_I;
}

#if 0
static std::vector < double > getInitialPD(double const p_d)
{
	std::vector < double > P_D;
	P_D.push_back(1.0 - p_d);
	P_D.push_back(p_d);
	return P_D;
}
#endif

bool compareVectors(
	std::vector<double> const & A,
	std::vector<double> const & B,
	double const eps
)
{
	for ( uint64_t i = 0; i < std::min(A.size(),B.size()); ++i )
		if ( std::abs(A[i]-B[i]) > eps )
			return false;

	for ( uint64_t i = A.size(); i < B.size(); ++i )
		if ( std::abs(B[i]) > eps )
			return false;

	for ( uint64_t i = B.size(); i < A.size(); ++i )
		if ( std::abs(A[i]) > eps )
			return false;

	return true;
}

bool testPowerCache(std::vector<double> const & V, unsigned int const l, double const eps)
{
	std::vector<double> C(1,1.0);

	libmaus2::math::Convolution::PowerCache PC(V,1e-8);

	bool ok = true;
	uint64_t const n = 1ull < l;
	for ( uint64_t i = 0; ok && i < n; ++i )
	{
		ok = ok && compareVectors(C,PC[i],eps);
	}

	return ok;
}

bool testPowerCacheRussian(std::vector<double> const & V, unsigned int const l, unsigned int const maxshift, double const eps)
{
	std::vector<double> C(1,1.0);

	bool ok = true;
	for ( unsigned int shift = 0; shift < maxshift; ++shift )
	{
		libmaus2::math::Convolution::PowerCacheRussian PCR(V,l,shift*l,1 /* numthreads */,1e-8);
		libmaus2::math::Convolution::PowerCache PC(V,1e-8);

		uint64_t const n = 1ull << l;
		uint64_t const subshift = shift * l;

		//std::cerr << "[V] testPowerCacheRussian shift=" << shift << " l=" << l << " n=" << n << std::endl;

		for ( uint64_t i = 0; ok && i < n; ++i )
		{
			ok = ok && compareVectors(PC[i << subshift],PCR[i],eps);
			//std::cerr << "[V] testPowerCacheRussian shift=" << shift << " " << i << "/" << n << " ok=" << ok << std::endl;
		}
	}

	return ok;
}

bool testPowerBlockCache(std::vector<double> const & V, unsigned int const l, double const eps)
{
	std::vector<double> C(1,1.0);

	libmaus2::math::Convolution::PowerCache PC(V,1e-8);
	libmaus2::math::Convolution::PowerBlockCache PBC(V,4/*blocksize*/,1/*numthreads*/,1e-8);

	bool ok = true;
	uint64_t const n = 1ull << (3*l);
	for ( uint64_t i = 0; ok && i < n; ++i )
	{
		ok = ok && compareVectors(PC[i],PBC[i],eps);
		// std::cerr << i << std::endl;
	}

	return ok;
}

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgParser const arg(argc,argv);

		std::vector<double> const V = getInitialPI(0.1);

		bool const pcok = testPowerCache(V,8,1e-6);

		std::cerr << "[V] testPowerCache " << (pcok?"ok":"failed") << std::endl;

		bool const pcokrussian = testPowerCacheRussian(V,4,4,1e-6);

		std::cerr << "[V] testPowerCacheRussian " << (pcokrussian?"ok":"failed") << std::endl;

		bool const pcokblock = testPowerBlockCache(V,4,1e-6);

		std::cerr << "[V] testPowerBlockCache " << (pcokblock?"ok":"failed") << std::endl;

		bool const allok = pcok && pcokrussian && pcokblock;

		if ( allok )
		{
			std::cerr << "[V] all ok" << std::endl;
			return EXIT_SUCCESS;
		}
		else
		{
			std::cerr << "[V] test failed" << std::endl;
			return EXIT_FAILURE;
		}
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
