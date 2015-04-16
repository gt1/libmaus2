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
#include <libmaus2/lz/Lz4DecoderBuffer.hpp>
#include <libmaus2/lz/lz4.h>

int libmaus2::lz::Lz4DecoderBuffer::decompressBlock(
	char const * input, char * output, 
	uint64_t const inputsize,
	uint64_t const maxoutputsize
)
{
	return LZ4_decompress_safe (input, output, inputsize, maxoutputsize);
}
