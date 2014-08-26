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
#if ! defined(BUFFEREDGZIPSTREAMBUFFER_HPP)
#define BUFFEREDGZIPSTREAMBUFFER_HPP

#include <libmaus/lz/GzipStreamWrapper.hpp>

namespace libmaus
{
	namespace lz
	{
		struct BufferedGzipStreamBuffer : public GzipStreamWrapper, ::libmaus::lz::StreamWrapperBuffer< ::libmaus::lz::GzipStream >
		{
			typedef BufferedGzipStreamBuffer this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			BufferedGzipStreamBuffer(::std::istream & in, uint64_t const bufsize = 64*1024, uint64_t const pushbacksize = 64*1024)
			: GzipStreamWrapper(in), ::libmaus::lz::StreamWrapperBuffer< ::libmaus::lz::GzipStream >(GZ,bufsize,pushbacksize)
			{
			
			}	
		};		
	}
}
#endif
