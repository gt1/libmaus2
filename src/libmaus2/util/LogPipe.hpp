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

#if ! defined(LOGPIPE_HPP)
#define LOGPIPE_HPP

#include <sys/types.h>
#include <sys/wait.h>
#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/network/Socket.hpp>

namespace libmaus2
{
	namespace util
	{
		struct LogPipe
		{
			typedef LogPipe this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			
			int stdoutpipe[2];
			int stderrpipe[2];

			::libmaus2::network::ClientSocket::unique_ptr_type outsock;
			::libmaus2::network::ClientSocket::unique_ptr_type errsock;
			
			pid_t pid;
			
			LogPipe(
				std::string const & serverhostname,
				unsigned short outport,
				unsigned short errport,
				uint64_t const rank,
				std::string const & sid
				);
			void join();
			~LogPipe();
		};
	}
}
#endif
