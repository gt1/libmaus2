/*
    libmaus
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS_AIO_POSIXFDINPUTSTREAM_HPP)
#define LIBMAUS_AIO_POSIXFDINPUTSTREAM_HPP

#include <libmaus/aio/PosixFdInputWrapper.hpp>
#include <libmaus/lz/StreamWrapper.hpp>

namespace libmaus
{
	namespace aio
	{
		struct PosixFdInputStream : public PosixFdInputWrapper, libmaus::lz::StreamWrapper<PosixFdInput>
		{
			typedef PosixFdInputStream this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			PosixFdInputStream(int const rfd, uint64_t const bufsize, uint64_t const pushbacksize = 0) : PosixFdInputWrapper(rfd), libmaus::lz::StreamWrapper<PosixFdInput>(PosixFdInputWrapper::object,bufsize,pushbacksize) {}
			PosixFdInputStream(std::string const & filename, uint64_t const bufsize, uint64_t const pushbacksize = 0, int const rflags = PosixFdInput::getDefaultFlags()) : PosixFdInputWrapper(filename,rflags), libmaus::lz::StreamWrapper<PosixFdInput>(PosixFdInputWrapper::object,bufsize,pushbacksize) {}
		};
	}
}
#endif
