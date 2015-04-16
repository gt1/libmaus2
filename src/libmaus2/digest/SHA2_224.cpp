/*
    libmaus2
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
#include <libmaus2/digest/SHA2_224.hpp>

#if defined(LIBMAUS_HAVE_NETTLE)
#include <nettle/sha.h>

libmaus2::digest::SHA2_224::SHA2_224() : ctx(0) {ctx = new sha224_ctx;}
libmaus2::digest::SHA2_224::~SHA2_224() { delete reinterpret_cast<sha224_ctx *>(ctx); }
void libmaus2::digest::SHA2_224::init() { sha224_init(reinterpret_cast<sha224_ctx *>(ctx)); }
void libmaus2::digest::SHA2_224::update(uint8_t const * t, size_t l) { sha224_update(reinterpret_cast<sha224_ctx *>(ctx),l,t); }
void libmaus2::digest::SHA2_224::digest(uint8_t * digest) { sha224_digest(reinterpret_cast<sha224_ctx *>(ctx),digestlength,&digest[0]); }
void libmaus2::digest::SHA2_224::copyFrom(libmaus2::digest::SHA2_224 const & O)
{
	(*reinterpret_cast<sha224_ctx *>(ctx)) = (*reinterpret_cast<sha224_ctx *>(O.ctx));
}
#else
#include <libmaus2/exception/LibMausException.hpp>

libmaus2::digest::SHA2_224::SHA2_224() : ctx(0) 
{ 
	libmaus2::exception::LibMausException lme;
	lme.getStream() << "libmaus2::digest::SHA2_224::SHA2_224(): nettle library not present" << std::endl;
	lme.finish();
	throw lme;
}
libmaus2::digest::SHA2_224::~SHA2_224() 
{ 
}
void libmaus2::digest::SHA2_224::init() 
{ 
}
void libmaus2::digest::SHA2_224::update(uint8_t const *, size_t) 
{
}
void libmaus2::digest::SHA2_224::digest(uint8_t *) 
{ 
}
#endif

void libmaus2::digest::SHA2_224::vinit() { init(); }
void libmaus2::digest::SHA2_224::vupdate(uint8_t const * u, size_t l) { update(u,l); }
