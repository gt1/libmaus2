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
#include <libmaus2/LibMausConfig.hpp>
#include <libmaus2/lz/IGzipDeflate.hpp>

// int const libmaus2::lz::IGzipDeflate::COMPRESSION_LEVEL = 11;

#if defined(LIBMAUS2_HAVE_IGZIP)
extern "C" {
	#include <igzip_lib.h>
}

int64_t libmaus2::lz::IGzipDeflate::deflate(uint8_t * const in, uint64_t const insize, uint8_t * const out, uint64_t const outsize)
{
	LZ_Stream2 stream;
	init_stream(&stream);

	stream.next_in = in;
	stream.avail_in = insize;
	stream.next_out = out;
	stream.avail_out = outsize;
	stream.end_of_stream = 1;

	fast_lz(&stream);

	if ( stream.avail_out == 0 )
		return -1;
	else
		return outsize - stream.avail_out;
}
#else
// stub which always returns the failure code
int64_t libmaus2::lz::IGzipDeflate::deflate(uint8_t * const, uint64_t const, uint8_t * const, uint64_t const)
{
	return -1;
}
#endif
