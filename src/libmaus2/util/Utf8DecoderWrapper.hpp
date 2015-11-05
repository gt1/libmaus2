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

#if ! defined(LIBMAUS2_UTIL_UTF8DECODERWRAPPER_HPP)
#define LIBMAUS2_UTIL_UTF8DECODERWRAPPER_HPP

#include <libmaus2/util/Utf8DecoderBuffer.hpp>

namespace libmaus2
{
	namespace util
	{
		struct Utf8DecoderWrapper : public Utf8DecoderBuffer, public ::std::wistream
		{
			typedef Utf8DecoderWrapper this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			typedef Utf8DecoderBuffer::char_type char_type;

			Utf8DecoderWrapper(std::string const & filename, uint64_t const buffersize = 64*1024);

			static wchar_t getSymbolAtPosition(std::string const & filename, uint64_t const offset);
			static uint64_t getFileSize(std::string const & filename);
		};
	}
}
#endif
