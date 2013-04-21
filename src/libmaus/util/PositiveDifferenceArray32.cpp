/**
    libmaus
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
**/

#include <libmaus/util/PositiveDifferenceArray32.hpp>

void libmaus::util::PositiveDifferenceArray32::serialise(std::ostream & out) const
{
	A->serialise(out);
}

libmaus::util::PositiveDifferenceArray32::PositiveDifferenceArray32(::std::istream & in)
: A ( new ::libmaus::util::Array832(in) )
{

}

libmaus::util::PositiveDifferenceArray32::PositiveDifferenceArray32(::libmaus::util::Array832::unique_ptr_type & rA)
: A(UNIQUE_PTR_MOVE(rA))
{

}
