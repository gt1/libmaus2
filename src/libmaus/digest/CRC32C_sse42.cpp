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
#include <libmaus/digest/CRC32C_sse42.hpp>
#include <libmaus/exception/LibMausException.hpp>

#if defined(LIBMAUS_USE_ASSEMBLY) && defined(LIBMAUS_HAVE_x86_64) && defined(LIBMAUS_HAVE_i386)
#include <libmaus/util/I386CacheLineSize.hpp>
#endif

libmaus::digest::CRC32C_sse42::CRC32C_sse42() : ctx(0) 
{
	#if ! ( defined(LIBMAUS_USE_ASSEMBLY) &&  defined(LIBMAUS_HAVE_x86_64) && defined(LIBMAUS_HAVE_i386) && defined(LIBMAUS_HAVE_SMMINTRIN_H) )
	libmaus::exception::LibMausException lme;
	lme.getStream() << "CRC32C_sse42(): code has not been compiled into libmaus" << std::endl;
	lme.finish();
	throw lme;
	#endif

	#if defined(LIBMAUS_USE_ASSEMBLY) && defined(LIBMAUS_HAVE_x86_64) && defined(LIBMAUS_HAVE_i386)
	if ( !libmaus::util::I386CacheLineSize::hasSSE42() )
	{
		libmaus::exception::LibMausException lme;
		lme.getStream() << "CRC32C_sse42(): processor does not support SSE4.2" << std::endl;
		lme.finish();
		throw lme;
	}
	#endif
}
libmaus::digest::CRC32C_sse42::~CRC32C_sse42() {}
	
void libmaus::digest::CRC32C_sse42::init() { ctx = 0; }

void libmaus::digest::CRC32C_sse42::digest(uint8_t * digest) 
{
	digest[0] = (ctx >> 24) & 0xFF;
	digest[1] = (ctx >> 16) & 0xFF;
	digest[2] = (ctx >>  8) & 0xFF;
	digest[3] = (ctx >>  0) & 0xFF;
}

void libmaus::digest::CRC32C_sse42::copyFrom(CRC32C_sse42 const & O)
{
	ctx = O.ctx;
}
