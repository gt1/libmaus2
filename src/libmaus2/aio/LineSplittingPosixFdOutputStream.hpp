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
#if ! defined(LIBMAUS2_AIO_LINESPLITTINGPOSIXFDOUTPUTSTREAM_HPP)
#define LIBMAUS2_AIO_LINESPLITTINGPOSIXFDOUTPUTSTREAM_HPP

#include <libmaus2/aio/LineSplittingPosixFdOutputStreamBuffer.hpp>

namespace libmaus2
{
	namespace aio
	{
		struct LineSplittingPosixFdOutputStream : public LineSplittingPosixFdOutputStreamBuffer, public std::ostream
		{
			typedef LineSplittingPosixFdOutputStream this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			LineSplittingPosixFdOutputStream(std::string const & rfilename, uint64_t const rlinemod, uint64_t const rbuffersize = 64*1024)
			: LineSplittingPosixFdOutputStreamBuffer(rfilename,rlinemod,rbuffersize), std::ostream(this)
			{
				exceptions(std::ios::badbit);
			}
			~LineSplittingPosixFdOutputStream()
			{
				flush();
			}
			
			std::string getFileName(uint64_t const id) const
			{
				return LineSplittingPosixFdOutputStreamBuffer::getFileName(id);
			}
			
			uint64_t getNumFiles() const
			{
				return LineSplittingPosixFdOutputStreamBuffer::getNumFiles();
			}
			
			std::vector<std::string> getFileNames() const
			{
				std::vector<std::string> Vfn(getNumFiles());
				for ( uint64_t i = 0; i < Vfn.size(); ++i )
					Vfn[i] = getFileName(i);
				return Vfn;
			}
		};
	}
}
#endif
