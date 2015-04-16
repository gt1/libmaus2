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

#include <libmaus2/util/Array832.hpp>

void libmaus2::util::Array832::serialise(std::ostream & out) const
{
	::libmaus2::serialize::Serialize<uint64_t>::serialize(out,n);
	B.serialize(out);
	A8.serialize(out);
	A32.serialize(out);
}

uint64_t libmaus2::util::Array832::deserializeNumber(std::istream & in)
{
	uint64_t n;
	::libmaus2::serialize::Serialize<uint64_t>::deserialize(in,&n);
	assert ( in );
	return n;
}

libmaus2::util::Array832::Array832(std::istream & in)
: n(deserializeNumber(in)), B(in), R(new ::libmaus2::rank::ERank222B(B.get(),B.size()*64)), A8(in), A32(in)
{
}
