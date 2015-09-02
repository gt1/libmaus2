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

#if ! defined(INDEXLOADERBASE_HPP)
#define INDEXLOADERBASE_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/exception/LibMausException.hpp>
#include <libmaus2/aio/InputStreamFactoryContainer.hpp>
#include <fstream>

#if defined(__linux__)
#include <byteswap.h>
#endif

#if defined(__FreeBSD__) || defined(__NetBSD__)
#include <sys/endian.h>
#endif

namespace libmaus2
{
	namespace huffman
	{
		struct IndexLoaderBase
		{
			/* 
			 * read position of index, which is stored in the last 8 bytes of the file 
			 * byte order of the number is big endian
			 */
			static uint64_t getIndexPos(std::string const & filename);
		};
	}
}
#endif
