/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#include <libmaus2/math/lowbits.hpp>
#include <iostream>
#include <cassert>

void testLowBits()
{
	for ( unsigned int i = 0; i <= 64; ++i )
	{
		std::cerr << i << "\t" << std::hex << libmaus2::math::lowbits(i) << "\t" << libmaus2::math::LowBits<uint64_t>::lowbits(i) << std::dec << std::endl;
		assert ( libmaus2::math::lowbits(i) == libmaus2::math::LowBits<uint64_t>::lowbits(i) );
	}

	#if defined(LIBMAUS2_HAVE_UNSIGNED_INT128)
	for ( unsigned int i = 0; i <= 128; ++i )
	{
		libmaus2::uint128_t v = libmaus2::math::LowBits<libmaus2::uint128_t>::lowbits(i);
		uint64_t const high = v>>64;
		uint64_t const low = v;

		if ( high )
			std::cerr << i << "\t" << std::hex << high << low << std::dec << std::endl;
		else
			std::cerr << i << "\t" << std::hex << low << std::dec << std::endl;
	}
	#endif
}

int main()
{
	testLowBits();
}
