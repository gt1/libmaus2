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
#include <libmaus2/fastx/SingleWordDNABitBuffer.hpp>

std::ostream & libmaus2::fastx::operator<<(std::ostream & out, ::libmaus2::fastx::SingleWordDNABitBuffer const & S)
{
	unsigned int shift = S.width22;
	::libmaus2::fastx::SingleWordDNABitBuffer::data_type pmask = ::libmaus2::math::lowbits(2) << shift;

	for ( unsigned int i = 0; i < S.width; ++i, pmask >>= 2, shift -= 2 )
		out << ((S.buffer & pmask) >> shift);

	return out;
}
