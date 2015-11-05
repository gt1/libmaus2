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

#if ! defined(LIBMAUS2_PARALLEL_POSIXPROCESS_HPP)
#define LIBMAUS2_PARALLEL_POSIXPROCESS_HPP

#include <unistd.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <csignal>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <libmaus2/exception/LibMausException.hpp>

namespace libmaus2
{
	namespace parallel
	{
		struct PosixProcess
		{
			pid_t pid;

			PosixProcess() : pid(-1)
			{
			}
			virtual ~PosixProcess()
			{
				join();
			}

			void kill(int signal)
			{
				if ( pid != static_cast<pid_t>(-1) )
				{
					::kill(pid,signal);
				}
			}

			void start()
			{
				if ( pid != static_cast<pid_t>(-1) )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "PosixProcess::start() called for process which is already running." << std::endl;
					se.finish();
					throw se;
				}
				if ( (pid=fork()) == static_cast<pid_t>(-1) )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "fork() failed: " << strerror(errno) << std::endl;
					se.finish();
					throw se;
				}
				if ( pid == 0 )
				{
					int r = EXIT_FAILURE;
					try
					{
						r = run();
					}
					catch(std::exception const & ex)
					{
						std::cerr << "Uncaught exception in PosixProcess: " << ex.what() << std::endl;
					}
					catch(...)
					{
						std::cerr << "Uncaught unknown exception in PosixProcess: " << std::endl;
					}
					_exit(r);
				}
			}

			bool tryJoin()
			{
				if ( pid == static_cast<pid_t>(-1) )
					return true;

				int status;
				if ( waitpid(pid,&status,WNOHANG) == pid )
				{
					pid = static_cast<pid_t>(-1);
					return true;
				}
				else
				{
					return false;
				}
			}

			void join()
			{
				if ( pid != static_cast<pid_t>(-1) )
				{
					// wait for child to exit
					int status;
					waitpid(pid,&status,0);
					pid = -1;
				}
			}

			virtual int run() = 0;
		};
	}
}
#endif
