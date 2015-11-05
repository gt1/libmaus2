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

#include <iostream>
#include <libmaus2/bitio/CompactArray.hpp>
#include <libmaus2/wavelet/toWaveletTreeBits.hpp>
#include <libmaus2/random/Random.hpp>
#include <libmaus2/timing/RealTimeClock.hpp>

int main()
{
	try
	{
		for ( uint64_t q = 0; q < 100; ++q )
		{
			uint64_t const n = ::libmaus2::random::Random::rand64() % (10*1024*1024) + 1;
			uint64_t const b = ::libmaus2::random::Random::rand64() % 8 + 1;

			typedef ::libmaus2::bitio::CompactArray array_type;
			typedef array_type::unique_ptr_type array_ptr_type;

			array_ptr_type C( new array_type (n,b) );

			for ( uint64_t i = 0; i < n; ++i )
			{
				if ( ( i & (1024*1024-1)) == 0 )
					std::cerr << i << std::endl;
				C->set ( i, ::libmaus2::random::Random::rand64() & ((1ull<<b)-1) );
			}

			array_ptr_type D = C->clone();

			::libmaus2::timing::RealTimeClock rtc;
			rtc.start();
			::libmaus2::autoarray::AutoArray<uint64_t> WC = ::libmaus2::wavelet::toWaveletTreeBits(C.get());
			std::cerr << ", time=" << rtc.getElapsedSeconds() << std::endl;

			rtc.start();
			::libmaus2::autoarray::AutoArray<uint64_t> WD = ::libmaus2::wavelet::toWaveletTreeBitsParallel(D.get());
			std::cerr << ", time=" << rtc.getElapsedSeconds() << std::endl;

			assert ( WC.getN() == WD.getN() );

			for ( uint64_t i = 0; i < WC.getN(); ++i )
				assert ( WC[i] == WD[i] );
		}
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what();
		return EXIT_FAILURE;
	}
}
