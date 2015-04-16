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

#if ! defined(LIBMAUS_PARALLEL_PIPE_HPP)
#define LIBMAUS_PARALLEL_PIPE_HPP

#include <libmaus2/exception/LibMausException.hpp>
#include <unistd.h>

namespace libmaus2
{
	namespace parallel
	{
		struct Pipe
		{
			protected:
			int readfd;
			int writefd;
			
			Pipe()
			: readfd(-1), writefd(-1)
			{
				int fd[2];
				fd[0] = -1;
				fd[1] = -1;
				if ( pipe(&fd[0]) < 0 )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "pipe() failed: " << strerror(errno) << std::endl;
					se.finish();
					throw se;
				}
				readfd = fd[0];
				writefd = fd[1];
			}
			
			~Pipe()
			{
				if ( readfd != -1 )
				{
					close(readfd);
					readfd = -1;
				}
				if ( writefd != -1 )
				{
					close(writefd);
					writefd = -1;
				}
			}
		};
	}
}

#endif
