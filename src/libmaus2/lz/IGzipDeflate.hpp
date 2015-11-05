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
#if ! defined(LIBMAUS2_LZ_IGZIPDEFLATE_HPP)
#define LIBMAUS2_LZ_IGZIPDEFLATE_HPP

#include <libmaus2/types/types.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct IGzipDeflate
		{
			static int const COMPRESSION_LEVEL = 11;

			static int getCompressionLevel()
			{
				return COMPRESSION_LEVEL;
			}

			/**
			 * run deflate algorithm on data
			 *
			 * @param in input data
			 * @param insize length of input data in bytes
			 * @param out output buffer
			 * @param outsize length of output buffer in bytes
			 * @return length of compressed data or -1 for failure (output buffer is too small or igzip is not available)
			 **/
			static int64_t deflate(
				uint8_t * const in,
				uint64_t const insize,
				uint8_t * const out,
				uint64_t const outsize
			);
		};
	}
}
#endif
