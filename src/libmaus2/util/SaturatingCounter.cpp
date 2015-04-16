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

#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/rank/ERank222B.hpp>
#include <libmaus2/bitio/putBit.hpp>
#include <libmaus2/bitio/getBit.hpp>
#include <libmaus2/util/SaturatingCounter.hpp>

libmaus2::util::SaturatingCounter::SaturatingCounter(uint64_t const rn)
: n(rn), B ( (((n*2)+63)/64) ), A(reinterpret_cast<uint8_t *>(B.get()))
{
}

uint64_t libmaus2::util::SaturatingCounter::size() const
{
	return n;
}

void libmaus2::util::SaturatingCounter::shrink()
{
	/* handle first word endian safe */
	uint64_t C = 0;
	for ( uint64_t i = 0; i < std::min(n,static_cast<uint64_t>(32)); ++i )
		::libmaus2::bitio::putBit(&C, i, (get(i) == 1) );
	B [ 0 ] = C;

	/* handle rest, byte order does no longer matter */
	for ( uint64_t i = 32; i < n; ++i )
		::libmaus2::bitio::putBit(B.get(), i, (get(i) == 1) );
	
	/* shorten array */
	B.resize( (n + 63)/64 );
	/* set up rank dictionary */
	rank_ptr_type trank(new rank_type(B.get(),B.size()*64));
	rank = UNIQUE_PTR_MOVE(trank);
}
		
std::ostream & libmaus2::util::operator<<(std::ostream & out, libmaus2::util::SaturatingCounter const & S)
{
	out << "SaturatingCounter(";
	
	for ( uint64_t i = 0; i < S.size(); ++i )
		out << static_cast<unsigned int>(S.get(i)) << ((i+1<S.size())?";":"");
	out << ")";
	return out;
}
