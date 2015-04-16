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
#include <libmaus2/util/Utf8BlockIndexDecoder.hpp>

libmaus2::util::Utf8BlockIndexDecoder::Utf8BlockIndexDecoder(std::string const & filename)
: CIS(filename)
{
	blocksize = ::libmaus2::util::NumberSerialisation::deserialiseNumber(CIS);
	lastblocksize = ::libmaus2::util::NumberSerialisation::deserialiseNumber(CIS);
	maxblockbytes = ::libmaus2::util::NumberSerialisation::deserialiseNumber(CIS);
	numblocks = ::libmaus2::util::NumberSerialisation::deserialiseNumber(CIS);
}

uint64_t libmaus2::util::Utf8BlockIndexDecoder::operator[](uint64_t const i)
{
	CIS.clear();
	CIS.seekg((4+i)*sizeof(uint64_t));
	
	return ::libmaus2::util::NumberSerialisation::deserialiseNumber(CIS);
}
