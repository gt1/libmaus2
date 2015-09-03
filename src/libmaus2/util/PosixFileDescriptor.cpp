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

#include <libmaus2/util/PosixFileDescriptor.hpp>

#include <libmaus2/LibMausConfig.hpp>
#if defined(LIBMAUS2_HAVE_UNISTD_H)
#include <unistd.h>
#endif
			
libmaus2::util::PosixFileDescriptor::PosixFileDescriptor() : fd(-1) {}
libmaus2::util::PosixFileDescriptor::PosixFileDescriptor(int rfd) : fd(rfd) {}

libmaus2::util::PosixFileDescriptor::~PosixFileDescriptor()
{
	if ( fd != -1 )
	{
		close(fd);
		fd = -1;
	}
}

ssize_t libmaus2::util::PosixFileDescriptor::read(void * buf, size_t cnt)
{
	return ::read(fd,buf,cnt);
}

ssize_t libmaus2::util::PosixFileDescriptor::write(void const * buf, size_t cnt)
{
	return ::write(fd,buf,cnt);
}
