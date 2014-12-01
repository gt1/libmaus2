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
#include <libmaus/digest/CRC32C.hpp>
#include <libmaus/digest/CRC32C_Core.hpp>

libmaus::digest::CRC32C::CRC32C() : ctx(0) {}
libmaus::digest::CRC32C::~CRC32C() {}
	
void libmaus::digest::CRC32C::init() { ctx = 0; }
void libmaus::digest::CRC32C::update(uint8_t const * t, size_t l) { ctx = libmaus::digest::CRC32C_Core::crc32c_core(ctx,t,l); }
void libmaus::digest::CRC32C::digest(uint8_t * digest) 
{
	digest[0] = (ctx >> 24) & 0xFF;
	digest[1] = (ctx >> 16) & 0xFF;
	digest[2] = (ctx >>  8) & 0xFF;
	digest[3] = (ctx >>  0) & 0xFF;
}

void libmaus::digest::CRC32C::copyFrom(CRC32C const & O)
{
	ctx = O.ctx;
}
