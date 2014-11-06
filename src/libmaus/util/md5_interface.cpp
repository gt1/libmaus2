/*
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
*/

#include <libmaus/util/md5.hpp>
#include <libmaus/util/GetFileSize.hpp>

bool libmaus::util::MD5::md5(std::string const & input, std::string & output)
{
	md5_state_s state;
	md5_init(&state);
	md5_append(&state, reinterpret_cast<md5_byte_t const *>(input.c_str()), input.size());
	md5_byte_t digest[16];
	md5_finish(&state,digest);
	
	std::ostringstream ostr;
	for ( uint64_t i = 0; i < sizeof(digest)/sizeof(digest[0]); ++i )
		ostr << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(digest[i]);
	output = ostr.str();

	return true;
}

bool libmaus::util::MD5::md5(std::vector<std::string> const & V, std::string & output)
{
	std::ostringstream ostr;
	for ( uint64_t i = 0; i < V.size(); ++i )
		ostr << V[i];
	return md5(ostr.str(),output);
}

bool libmaus::util::MD5::md5(std::vector<std::string> const & V, uint64_t const k, std::string & output)
{
	std::ostringstream ostr;
	for ( uint64_t i = 0; i < V.size(); ++i )
		ostr << V[i] << ":" << ::libmaus::util::GetFileSize::getFileSize(V[i]) << ":";
	ostr << k;
	return md5(ostr.str(),output);
}

void libmaus::util::MD5::md5(uint8_t const * in, size_t const len, uint8_t digest[16])
{
	md5_state_s state;
	md5_init(&state);
	md5_append(&state, reinterpret_cast<md5_byte_t const *>(in), len);
	md5_finish(&state,digest);
}

void libmaus::util::MD5::md5(uint8_t const * in, size_t const len, libmaus::math::UnsignedInteger<4> & udigest)
{
	md5_state_s state;
	md5_init(&state);
	md5_append(&state, reinterpret_cast<md5_byte_t const *>(in), len);
	md5_byte_t digest[16];
	md5_finish(&state,digest);

	uint64_t const low = 
		(static_cast<uint64_t>(digest[0]) << 0)  | (static_cast<uint64_t>(digest[1]) << 8)  | (static_cast<uint64_t>(digest[2]) << 16) | (static_cast<uint64_t>(digest[3]) << 24) |
		(static_cast<uint64_t>(digest[4]) << 32) | (static_cast<uint64_t>(digest[5]) << 40) | (static_cast<uint64_t>(digest[6]) << 48) | (static_cast<uint64_t>(digest[7]) << 56);
	uint64_t const high = 
		(static_cast<uint64_t>(digest[8]) << 0) | (static_cast<uint64_t>(digest[9]) << 8) | (static_cast<uint64_t>(digest[10]) << 16) | (static_cast<uint64_t>(digest[11]) << 24) |
		(static_cast<uint64_t>(digest[12]) << 32) | (static_cast<uint64_t>(digest[13]) << 40) | (static_cast<uint64_t>(digest[14]) << 48) | (static_cast<uint64_t>(digest[15]) << 56);
	
	udigest = libmaus::math::UnsignedInteger<4>(high);
	udigest <<= 64;
	udigest |= libmaus::math::UnsignedInteger<4>(low);
}
libmaus::math::UnsignedInteger<4> libmaus::util::MD5::md5(uint8_t const * in, size_t const len)
{
	libmaus::math::UnsignedInteger<4> D;
	md5(in,len,D);
	return D;
}
