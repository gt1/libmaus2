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
#if ! defined(LIBMAUS2_LZ_XZINPUTSTREAM_HPP)
#define LIBMAUS2_LZ_XZINPUTSTREAM_HPP

#include <libmaus2/lz/XzInputStreamBuffer.hpp>
#include <istream>

namespace libmaus2
{
	namespace lz
	{
		struct XzInputStream : public XzInputStreamBuffer, std::istream
		{
			typedef XzInputStream this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			XzInputStream(::std::istream & in, uint64_t const bufsize = 64*1024, uint64_t const pushbacksize = 64*1024)
			: XzInputStreamBuffer(in,bufsize,pushbacksize), std::istream(this)
			{

			}
		};
	}
}
#endif
