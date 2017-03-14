/*
    libmaus2
    Copyright (C) 2017 German Tischler

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
#if ! defined(LIBMAUS2_HUFFMAN_OUTPUTADAPTER_HPP)
#define LIBMAUS2_HUFFMAN_OUTPUTADAPTER_HPP

#include <libmaus2/huffman/HuffmanEncoderFile.hpp>

namespace libmaus2
{
	namespace huffman
	{
		struct OutputAdapter
		{
			typedef uint64_t data_type;

			libmaus2::huffman::HuffmanEncoderFileStd & HEFS;

			OutputAdapter(libmaus2::huffman::HuffmanEncoderFileStd & rHEFS) : HEFS(rHEFS) {}

			void put(uint64_t const w)
			{
				HEFS.write(w >> 32         ,32);
				HEFS.write(w & 0xFFFFFFFFUL,32);
			}

			void flush()
			{
				HEFS.flush();
			}
		};
	}
}
#endif
