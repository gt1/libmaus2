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
#if ! defined(LIBMAUS2_HUFFMAN_HUFFMANINPUTFILE_HPP)
#define LIBMAUS2_HUFFMAN_HUFFMANINPUTFILE_HPP

#include <libmaus2/huffman/CanonicalEncoder.hpp>

namespace libmaus2
{
	namespace huffman
	{
		struct HuffmanInputFile
		{
			typedef ::libmaus2::huffman::BitInputBuffer4 sbis_type;			
			
			::libmaus2::aio::CheckedInputStream::unique_ptr_type istr;
			sbis_type::raw_input_ptr_type ript;
			sbis_type::unique_ptr_type SBIS;
			::libmaus2::huffman::CanonicalEncoder const & canon;
			
			HuffmanInputFile(std::string const & filename, ::libmaus2::huffman::CanonicalEncoder const & rcanon)
			: istr(
				UNIQUE_PTR_MOVE(
					::libmaus2::aio::CheckedInputStream::unique_ptr_type(
						new ::libmaus2::aio::CheckedInputStream(filename)
					)
				)
			  ),
			  ript(
				UNIQUE_PTR_MOVE(
					sbis_type::raw_input_ptr_type(
						new sbis_type::raw_input_type(*istr)
					)
				)
			  ),
			  SBIS(UNIQUE_PTR_MOVE(sbis_type::unique_ptr_type(new sbis_type(ript,static_cast<uint64_t>(64*1024))))),
			  canon(rcanon)
			{
			
			}
			
			int64_t decode()
			{
				return canon.fastDecode(*SBIS);
			}
		};
	}
}
#endif
