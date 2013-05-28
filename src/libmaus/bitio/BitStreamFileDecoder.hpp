/*
    libmaus
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
#if ! defined(LIBMAUS_BITIO_BITSTREAMFILEDECODER_HPP)
#define LIBMAUS_BITIO_BITSTREAMFILEDECODER_HPP

#include <libmaus/huffman/BitInputBuffer.hpp>

namespace libmaus
{
	namespace bitio
	{
		struct BitStreamFileDecoder
		{
			typedef ::libmaus::huffman::BitInputBuffer4 sbis_type;
			
			::libmaus::aio::CheckedInputStream::unique_ptr_type istr;
			sbis_type::raw_input_ptr_type ript;
			sbis_type::unique_ptr_type SBIS;
			
			static ::libmaus::aio::CheckedInputStream::unique_ptr_type openFileAtPosition(std::string const & filename, uint64_t const pos)
			{
				::libmaus::aio::CheckedInputStream::unique_ptr_type istr(new ::libmaus::aio::CheckedInputStream(filename));
				istr->seekg(pos,std::ios::beg);
				return UNIQUE_PTR_MOVE(istr);
			}

			BitStreamFileDecoder(std::string const & infile, uint64_t const bitpos = 0)
			:
				istr(UNIQUE_PTR_MOVE(openFileAtPosition(infile,bitpos/8))),
				ript(new sbis_type::raw_input_type(*istr)),
				SBIS(new sbis_type(ript,static_cast<uint64_t>(64*1024)))
			{
				for ( uint64_t i = 0; i < (bitpos%8); ++i )
					SBIS->readBit();
			}
			
			bool readBit()
			{
				return SBIS->readBit();
			}
		};
	}
}
#endif
