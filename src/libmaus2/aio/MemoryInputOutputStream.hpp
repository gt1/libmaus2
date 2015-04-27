/*
    libmaus2
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#if ! defined(LIBMAUS2_AIO_MEMORYINPUTOUTPUTSTREAM_HPP)
#define LIBMAUS2_AIO_MEMORYINPUTOUTPUTSTREAM_HPP

#include <libmaus2/aio/MemoryInputOutputStreamBuffer.hpp>

namespace libmaus2
{
	namespace aio
	{
		struct MemoryInputOutputStream : public MemoryInputOutputStreamBuffer, public std::iostream
		{
			typedef MemoryInputOutputStream this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
		
			MemoryInputOutputStream(std::string const & filename, std::ios_base::openmode const cxxmode, int64_t const bufsize = -1) 
			: MemoryInputOutputStreamBuffer(filename,cxxmode,bufsize),
			  std::iostream(this)
			{
				exceptions(std::ios::badbit);
			}
		};
	}
}
#endif
