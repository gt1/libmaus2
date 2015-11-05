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

#include <libmaus2/util/SimpleCountingHash.hpp>

void testSimpleCountingHash()
{
	::libmaus2::util::SimpleCountingHash<uint32_t,uint8_t> H(16);

	uint64_t const cnt =
		static_cast<uint64_t>(std::numeric_limits<uint8_t>::max()) * 2;

	for ( uint64_t z = 0; z < cnt; ++z )
	{
		#if defined(_OPENMP)
		#pragma omp parallel for
		#endif
		for ( int64_t i = 0; i < (1ll<<16); ++i )
			H.insert ( i );
	}

	for ( uint64_t i = 0; i < (1ull<<16); ++i )
		assert ( H.getCount(i) == std::numeric_limits<uint8_t>::max() );
}

int main(/* int argc, char * argv[] */)
{
	testSimpleCountingHash();
}
