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

#include <libmaus2/util/LogPipe.hpp>

#include <libmaus2/LibMausConfig.hpp>
#if defined(LIBMAUS2_HAVE_UNISTD_H)
#include <unistd.h>
#endif

libmaus2::util::LogPipe::LogPipe(
	std::string const & serverhostname,
	unsigned short outport,
	unsigned short errport,
	uint64_t const rank,
	std::string const & sid
	)
: pid(-1)
{
	::libmaus2::network::ClientSocket::unique_ptr_type toutsock(new ::libmaus2::network::ClientSocket(outport,serverhostname.c_str()));
	outsock = UNIQUE_PTR_MOVE(toutsock);
	outsock->setNoDelay();
	outsock->writeMessage<uint64_t>(0,&rank,1);
	outsock->writeString(0,sid);

	::libmaus2::network::ClientSocket::unique_ptr_type terrsock(new ::libmaus2::network::ClientSocket(errport,serverhostname.c_str()));
	errsock = UNIQUE_PTR_MOVE(terrsock);
	errsock->setNoDelay();
	errsock->writeMessage<uint64_t>(0,&rank,1);
	errsock->writeString(0,sid);

	#if 1
	if ( pipe(&stdoutpipe[0]) != 0 )
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "pipe() failed: " << strerror(errno) << std::endl;
		se.finish();
		throw se;
	}
	if ( pipe(&stderrpipe[0]) != 0 )
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "pipe() failed: " << strerror(errno) << std::endl;
		se.finish();
		throw se;
	}

	if ( close(STDOUT_FILENO) != 0 )
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "close() failed: " << strerror(errno) << std::endl;
		se.finish();
		throw se;
	}
	if ( close(STDERR_FILENO) != 0 )
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "close() failed: " << strerror(errno) << std::endl;
		se.finish();
		throw se;
	}

	if ( dup2(stdoutpipe[1],STDOUT_FILENO) == -1 )
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "dup2() failed: " << strerror(errno) << std::endl;
		se.finish();
		throw se;
	}
	if ( dup2(stderrpipe[1],STDERR_FILENO) == -1 )
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "dup2() failed: " << strerror(errno) << std::endl;
		se.finish();
		throw se;
	}

	pid = fork();

	if ( pid < 0 )
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "fork() failed: " << strerror(errno) << std::endl;
		se.finish();
		throw se;
	}
	else if ( pid == 0 )
	{
		// close write end
		close(stdoutpipe[1]);
		close(stderrpipe[1]);
		// close copies
		close(STDOUT_FILENO);
		close(STDERR_FILENO);

		bool running = true;

		try
		{
			while ( running )
			{
				running = false;
				fd_set fds;
				int maxfd = -1;
				FD_ZERO(&fds);

				if ( stdoutpipe[0] != -1 )
				{
					FD_SET(stdoutpipe[0],&fds);
					maxfd = std::max(maxfd,stdoutpipe[0]);
				}
				if ( stderrpipe[0] != -1 )
				{
					FD_SET(stderrpipe[0],&fds);
					maxfd = std::max(maxfd,stderrpipe[0]);
				}

				running = (maxfd != -1);

				if ( running )
				{
					int r = select(maxfd+1,&fds,0,0,0);

					try
					{
						if ( r > 0 )
						{
							if ( stdoutpipe[0] != -1 && FD_ISSET(stdoutpipe[0],&fds) )
							{
								::libmaus2::autoarray::AutoArray<char> B(1024,false);
								ssize_t red = read(stdoutpipe[0],B.get(),B.size());
								if ( red <= 0 )
								{
									stdoutpipe[0] = -1;
								}
								else
								{
									outsock->writeMessage<char>(0,B.get(),red);
									uint64_t stag, n;
									outsock->readMessage<uint64_t>(stag,0,n);
								}
							}
							if ( stderrpipe[0] != -1 && FD_ISSET(stderrpipe[0],&fds) )
							{
								::libmaus2::autoarray::AutoArray<char> B(1024,false);
								ssize_t red = read(stderrpipe[0],B.get(),B.size());
								if ( red <= 0 )
								{
									stderrpipe[0] = -1;
								}
								else
								{
									errsock->writeMessage<char>(0,B.get(),red);
									uint64_t stag, n;
									errsock->readMessage<uint64_t>(stag,0,n);
								}
							}
						}
					}
					catch(std::exception const & ex)
					{
					}
				}
			}
		}
		catch(...)
		{
			std::cerr << "Caught exception in LogPipe" << std::endl;
		}

		_exit(0);
	}
	else
	{
		// close read ends
		close(stdoutpipe[0]);
		close(stderrpipe[0]);
	}
	#endif
}

void libmaus2::util::LogPipe::join()
{
	close(stdoutpipe[1]);
	close(stderrpipe[1]);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	int status;
	waitpid(pid,&status,0);
}

libmaus2::util::LogPipe::~LogPipe()
{
	join();
}
