/**
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
**/

#if ! defined(POSIXEXECUTE_HPP)
#define POSIXEXECUTE_HPP

#include <string>
#include <cerrno>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/util/ArgInfo.hpp>
#include <libmaus/util/GetFileSize.hpp>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
              
/* According to POSIX.1-2001 */
#include <sys/select.h>
/* According to earlier standards */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
                                          
namespace libmaus
{
	namespace util
	{
		struct PosixExecute
		{
			static int getTempFile(std::string const stemplate, std::string & filename)
			{
				::libmaus::autoarray::AutoArray<char> Atemplate(stemplate.size()+1);
				std::copy ( stemplate.begin(), stemplate.end(), Atemplate.get() );
				int const fd = mkstemp ( Atemplate.get() );
				
				if ( fd < 0 )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Failed in mkstemp: " << strerror(errno);
					se.finish();
					throw se;
				}
				
				filename = Atemplate.get();
				
				return fd;
			}

			static std::string getBaseName(std::string const & name)
			{
				::libmaus::autoarray::AutoArray<char> Aname(name.size()+1);
				std::copy ( name.begin(), name.end(), Aname.get() );
				char * bn = basename(Aname.get());
				return std::string ( bn );
			}

			static std::string getProgBaseName(::libmaus::util::ArgInfo const & arginfo)
			{
				return getBaseName ( arginfo.progname );
			}

			static int getTempFile(::libmaus::util::ArgInfo const & arginfo, std::string & filename)
			{
				::std::ostringstream prefixstr;
				prefixstr << "/tmp/" << getProgBaseName(arginfo) << "_XXXXXX";
				return getTempFile(prefixstr.str(),filename);
			}

			static std::string loadFile(std::string const filename)
			{
				uint64_t const len = ::libmaus::util::GetFileSize::getFileSize(filename);
				std::ifstream istr(filename.c_str(),std::ios::binary);
				::libmaus::autoarray::AutoArray<char> data(len,false);
				istr.read ( data.get(), len );
				assert ( istr );
				assert ( istr.gcount() == static_cast<int64_t>(len) );
				return std::string(data.get(),data.get()+data.size());
			}

			static void executeOld(::libmaus::util::ArgInfo const & arginfo, std::string const & command, std::string & out, std::string & err)
			{
				std::string stdoutfilename;
				std::string stderrfilename;

				int const stdoutfd = getTempFile(arginfo,stdoutfilename);
				int const stderrfd = getTempFile(arginfo,stderrfilename);

				pid_t const pid = fork();
				
				if ( pid == -1 )
				{
					close(stdoutfd);
					close(stderrfd);
					remove ( stdoutfilename.c_str() );
					remove ( stderrfilename.c_str() );
					
					::libmaus::exception::LibMausException se;
					se.getStream() << "Failed to fork()";
					se.finish();
					throw se;
				}
				
				/* child */
				if ( pid == 0 )
				{
					close(STDOUT_FILENO);
					close(STDERR_FILENO);
					int const nullfd = open("/dev/null",O_RDONLY);
					dup2(nullfd,STDIN_FILENO);
					dup2(stdoutfd,STDOUT_FILENO);
					dup2(stderrfd,STDERR_FILENO);
					int const ret = system ( command.c_str() );
					_exit(ret);
				}
				else
				{
					close(stdoutfd);
					close(stderrfd);
					
					int status = 0;
					pid_t const wpid = waitpid(pid, &status, 0);
					assert ( wpid == pid );
					
					out = loadFile(stdoutfilename);
					err = loadFile(stderrfilename);
				}
			}
			
			static int setNonBlockFlag (int desc, bool on)
			{
				int oldflags = fcntl (desc, F_GETFL, 0);
				/* If reading the flags failed, return error indication now. */
				if (oldflags < 0)
					return oldflags;
				/* Set just the flag we want to set. */
				if (on)
					oldflags |= O_NONBLOCK;
				else
			        	oldflags &= ~O_NONBLOCK;
				/* Store modified flag word in the descriptor. */
				return fcntl (desc, F_SETFL, oldflags);
			}

			static int execute(std::string const & command, std::string & out, std::string & err, bool const donotthrow = false)
			{
				int stdoutpipe[2];
				int stderrpipe[2];
				
				if ( pipe(&stdoutpipe[0]) != 0 )
				{
					if ( donotthrow )
					{
						std::cerr << "pipe() failed: " << strerror(errno) << std::endl;
						return EXIT_FAILURE;
					}
					else
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "pipe() failed: " << strerror(errno);
						se.finish();
						throw se;
					}
				}
				if ( pipe(&stderrpipe[0]) != 0 )
				{
					close(stdoutpipe[0]);
					close(stderrpipe[0]);

					if ( donotthrow )
					{
						std::cerr << "pipe() failed: " << strerror(errno) << std::endl;
						return EXIT_FAILURE;
					}
					else
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "pipe() failed: " << strerror(errno);
						se.finish();
						throw se;
					}
				}

				pid_t const pid = fork();
				
				if ( pid == -1 )
				{
					close(stdoutpipe[0]);
					close(stdoutpipe[0]);
					close(stdoutpipe[1]);
					close(stdoutpipe[1]);
					
					if ( donotthrow )
					{
						std::cerr << "Failed to fork()" << strerror(errno) << std::endl;
						return EXIT_FAILURE;
					}
					else
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "Failed to fork(): " << strerror(errno);
						se.finish();
						throw se;
					}
				}
				
				/* child */
				if ( pid == 0 )
				{
					close(stdoutpipe[0]);
					close(stderrpipe[0]);
					
					close(STDOUT_FILENO);
					close(STDERR_FILENO);
					int const nullfd = open("/dev/null",O_RDONLY);
					dup2(nullfd,STDIN_FILENO);
					dup2(stdoutpipe[1],STDOUT_FILENO);
					dup2(stderrpipe[1],STDERR_FILENO);
					int const ret = system ( command.c_str() );
				
					#if 0	
					std::ostringstream ostr;
					ostr << "After return code " << ret << std::endl;
					std::string ostrstr = ostr.str();
					write ( STDERR_FILENO, ostrstr.c_str(), ostrstr.size() );
					#endif
					
					close(stdoutpipe[1]);
					close(stderrpipe[1]);
					
					_exit(WEXITSTATUS(ret));
				}
				else
				{
					close(stdoutpipe[1]);
					close(stderrpipe[1]);
					
					setNonBlockFlag ( stdoutpipe[0], true );
					setNonBlockFlag ( stderrpipe[0], true );
					
					bool done = false;
					int status;
					
					::libmaus::autoarray::AutoArray<char> B(8*8192,false);
					
					std::vector<char> outv;
					std::vector<char> errv;
					
					while ( ! done )
					{
						fd_set fileset;
						FD_ZERO(&fileset);
						FD_SET(stdoutpipe[0],&fileset);
						FD_SET(stderrpipe[0],&fileset);
						int nfds = std::max(stdoutpipe[0],stderrpipe[0])+1;
						
						struct timeval timeout = { 0,0 };
						int selret = select(nfds, &fileset, 0, 0, &timeout);

						if ( selret > 0 )
						{
							// std::cerr << "Select returned " << selret << std::endl;
							
							if ( FD_ISSET(stdoutpipe[0],&fileset) )
							{
								ssize_t red = read ( stdoutpipe[0], B.get(), B.size() );
								// std::cerr << "Got " << red << " from stdout." << std::endl;
								if ( red > 0 )
									for ( ssize_t i = 0; i < red; ++i )
										outv.push_back(B[i]);
							}
							if ( FD_ISSET(stderrpipe[0],&fileset) )
							{
								ssize_t red = read ( stderrpipe[0], B.get(), B.size() );
								// std::cerr << "Got " << red << " from stderr." << std::endl;
								if ( red > 0 )
									for ( ssize_t i = 0; i < red; ++i )
										errv.push_back(B[i]);
							}
						}
						
						status = 0;
						pid_t const wpid = waitpid(pid, &status, WNOHANG);
						
						if ( wpid == pid )
						{
							done = true;
						}
						else if ( wpid < 0 )
						{
							done = true;
						}
						else
						{
							if ( ! selret )
							{
								struct timespec waittime = { 0, 100000000 };
								nanosleep(&waittime,0);
							}
						}
					}

					setNonBlockFlag ( stdoutpipe[0], false );
					setNonBlockFlag ( stderrpipe[0], false );
					
					ssize_t red = -1;

					while ( (red = read ( stdoutpipe[0], B.get(), B.size() ) > 0 ) )
					{
						// std::cerr << "Got " << red << " from stdout in final." << std::endl;
						for ( ssize_t i = 0; i < red; ++i )
							outv.push_back(B[i]);
					}
					while ( (red = read ( stderrpipe[0], B.get(), B.size() ) > 0 ) )
					{
						// std::cerr << "Got " << red << " from stderr in final." << std::endl;
						for ( ssize_t i = 0; i < red; ++i )
							errv.push_back(B[i]);
					}
	
					close(stdoutpipe[0]);
					close(stderrpipe[0]);
					
					out = std::string(outv.begin(),outv.end());
					err = std::string(errv.begin(),errv.end());
					
					if ( ! WIFEXITED(status) )
					{
						if ( donotthrow )
						{
							std::cerr << "Calling process " << command << " failed." << std::endl;
							return EXIT_FAILURE;
						}
						else
						{
							::libmaus::exception::LibMausException se;
							se.getStream() << "Calling process " << command << " failed.";
							se.finish();
							throw se;
						}
					}
					else
					{
						return WEXITSTATUS(status);
					}
				}
			}
		};
	}
}
#endif
