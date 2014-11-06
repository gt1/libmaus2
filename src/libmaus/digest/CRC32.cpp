/*
    libmaus
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#include <libmaus/digest/CRC32.hpp>

#include <zlib.h>

libmaus::digest::CRC32::CRC32() : initial(crc32(0L, Z_NULL, 0)), ctx(0) {}
libmaus::digest::CRC32::~CRC32() {}
	
void libmaus::digest::CRC32::init() { ctx = initial; }
void libmaus::digest::CRC32::update(uint8_t const * t, size_t l) { ctx = crc32(ctx,t,l); }
void libmaus::digest::CRC32::digest(uint8_t * digest) 
{
	digest[0] = (ctx >> 24) & 0xFF;
	digest[1] = (ctx >> 16) & 0xFF;
	digest[2] = (ctx >>  8) & 0xFF;
	digest[3] = (ctx >>  0) & 0xFF;
}

void libmaus::digest::CRC32::copyFrom(CRC32 const & O)
{
	ctx = O.ctx;
}
