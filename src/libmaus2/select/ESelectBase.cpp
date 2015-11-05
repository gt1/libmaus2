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
#include <libmaus2/select/ESelectBase.hpp>

/**
 * return offset of i+1th 1 bit in v
 **/
template<bool sym>
uint64_t libmaus2::select::ESelectBase<sym>::select1Slow(uint16_t v, unsigned int i)
{
	uint64_t mask = 1ull << 15;
	uint64_t j = 0;
	i++;

	for ( ; mask; mask >>= 1, j++ )
		if ( (v & mask) && ( ! (--i) ) )
			return j;

	return j;
}

template<bool sym>
libmaus2::autoarray::AutoArray<uint8_t> libmaus2::select::ESelectBase<sym>::computeRussians()
{
	::libmaus2::autoarray::AutoArray<uint8_t> R( (1ull<<16) * (16) );

	for ( uint32_t v = 0; v < (1u<<16); ++v )
		for ( unsigned int i = 0; i < 16; ++i )
			R [ (v << 4) | i ] = select1Slow(v,i);

	return R;
}


template struct ::libmaus2::select::ESelectBase<false>;
template struct ::libmaus2::select::ESelectBase<true>;
