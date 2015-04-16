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

#if ! defined(LIBMAUS_FASTX_GZIPFILEFASTQREADER_HPP)
#define LIBMAUS_FASTX_GZIPFILEFASTQREADER_HPP

#include <libmaus/fastx/GzipStreamFastQReader.hpp>
#include <libmaus/aio/CheckedInputStreamAtOffset.hpp>

namespace libmaus
{
	namespace fastx
	{
		struct GzipFileFastQReader : public libmaus::aio::CheckedInputStreamAtOffsetWrapper, GzipStreamFastQReader
		{
                	typedef GzipFileFastQReader this_type;
                	typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			GzipFileFastQReader(std::string const & filename)
			: libmaus::aio::CheckedInputStreamAtOffsetWrapper(filename), GzipStreamFastQReader(libmaus::aio::CheckedInputStreamAtOffsetWrapper::object)
			{
			
			}
			GzipFileFastQReader(std::string const & filename, int const qualityOffset)
			: libmaus::aio::CheckedInputStreamAtOffsetWrapper(filename), GzipStreamFastQReader(libmaus::aio::CheckedInputStreamAtOffsetWrapper::object,qualityOffset)
			{
			
			}
			GzipFileFastQReader(std::string const & filename, libmaus::fastx::FastInterval const & FI)
			: libmaus::aio::CheckedInputStreamAtOffsetWrapper(filename,FI.fileoffset), GzipStreamFastQReader(libmaus::aio::CheckedInputStreamAtOffsetWrapper::object,FI)
			{
			
			}
			GzipFileFastQReader(std::string const & filename, int const qualityOffset, libmaus::fastx::FastInterval const & FI)
			: libmaus::aio::CheckedInputStreamAtOffsetWrapper(filename,FI.fileoffset), GzipStreamFastQReader(libmaus::aio::CheckedInputStreamAtOffsetWrapper::object,qualityOffset,FI)
			{
			
			}
		};
	}
}
#endif
