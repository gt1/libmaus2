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
#include <libmaus/digest/SHA1.hpp>

#if defined(LIBMAUS_HAVE_NETTLE)
#include <nettle/sha1.h>

libmaus::digest::SHA1::SHA1() : ctx(0) { ctx = new sha1_ctx; }
libmaus::digest::SHA1::~SHA1() { delete reinterpret_cast<sha1_ctx *>(ctx); }
void libmaus::digest::SHA1::init() { sha1_init(reinterpret_cast<sha1_ctx *>(ctx)); }
void libmaus::digest::SHA1::update(uint8_t const * t, size_t l) { sha1_update(reinterpret_cast<sha1_ctx *>(ctx),l,t); }
void libmaus::digest::SHA1::digest(uint8_t * digest) { sha1_digest(reinterpret_cast<sha1_ctx *>(ctx),digestlength,&digest[0]); }
void libmaus::digest::SHA1::copyFrom(libmaus::digest::SHA1 const & O)
{
	(*reinterpret_cast<sha1_ctx *>(ctx)) = (*reinterpret_cast<sha1_ctx *>(O.ctx));
}
#else
#include <libmaus/exception/LibMausException.hpp>

libmaus::digest::SHA1::SHA1() : ctx(0) 
{ 
	libmaus::exception::LibMausException lme;
	lme.getStream() << "libmaus::digest::SHA1::SHA1(): nettle library not present" << std::endl;
	lme.finish();
	throw lme;
}
libmaus::digest::SHA1::~SHA1() 
{ 
}
void libmaus::digest::SHA1::init() 
{ 
}
void libmaus::digest::SHA1::update(uint8_t const * t, size_t l) 
{
}
void libmaus::digest::SHA1::digest(uint8_t * digest) 
{ 
}
#endif
