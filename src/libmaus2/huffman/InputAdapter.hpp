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
#if ! defined(LIBMAUS2_HUFFMAN_INPUTADAPTER_HPP)
#define LIBMAUS2_HUFFMAN_INPUTADAPTER_HPP

#include <libmaus2/aio/InputStreamInstance.hpp>
#include <libmaus2/huffman/BitInputBuffer.hpp>

namespace libmaus2
{
	namespace huffman
	{
		struct InputAdapter
		{
			typedef uint64_t data_type;

			libmaus2::aio::InputStreamInstance::unique_ptr_type PISI;
			libmaus2::huffman::BitInputBuffer4::unique_ptr_type PBIB;

			InputAdapter(std::string const & fn)
			: PISI(new libmaus2::aio::InputStreamInstance(fn))
			{
				libmaus2::huffman::FileStreamBaseType::unique_ptr_type FSBT(new libmaus2::huffman::FileStreamBaseType(*PISI));
				libmaus2::huffman::BitInputBuffer4::unique_ptr_type TBIB(new libmaus2::huffman::BitInputBuffer4(FSBT,4096));
				PBIB = UNIQUE_PTR_MOVE(TBIB);
			}

			InputAdapter(std::istream & in)
			{
				libmaus2::huffman::FileStreamBaseType::unique_ptr_type FSBT(new libmaus2::huffman::FileStreamBaseType(in));
				libmaus2::huffman::BitInputBuffer4::unique_ptr_type TBIB(new libmaus2::huffman::BitInputBuffer4(FSBT,4096));
				PBIB = UNIQUE_PTR_MOVE(TBIB);
			}

			uint64_t read(unsigned int b)
			{
				uint64_t v = 0;

				while ( b >= 32 )
				{
					v <<= 32;
					v |= PBIB->read(32);
					b -= 32;
				}

				if ( b )
				{
					v <<= b;
					v |= PBIB->read(b);
				}

				return v;
			}

			bool getNext(uint64_t & v)
			{
				v = PBIB->read(32);
				v <<= 32;
				v |= PBIB->read(32);
				return true;
			}
		};
	}
}
#endif
