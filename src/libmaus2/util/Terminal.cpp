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

#include <libmaus2/util/Terminal.hpp>

uint64_t libmaus2::util::Terminal::getColumns()
{
	int fd = -1;
	uint64_t cols = 80;

	try
	{
		fd = open("/dev/tty",O_RDWR);

		if ( fd >= 0 )
		{
			winsize size;
			memset(&size,0,sizeof(size));
			int const stat = ioctl(fd,TIOCGWINSZ,&size);
			close(fd);
			fd = -1;

			if ( stat < 0 )
			{
				::libmaus2::exception::LibMausException se;
				se.getStream() << "ioctl failed: " << strerror(errno) << std::endl;
				se.finish();
				throw se;
			}
			else
			{
				cols = size.ws_col;
			}
		}
		else
		{
			::libmaus2::exception::LibMausException se;
			se.getStream() << "open failed: " << strerror(errno) << std::endl;
			se.finish();
			throw se;
		}
	}
	catch(...)
	{
		if ( fd >= 0 )
			close(fd);
	}

	return cols;
}
