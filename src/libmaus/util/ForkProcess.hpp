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

#if ! defined(FORKPROCESS_HPP)
#define FORKPROCESS_HPP

#include <iostream>
#include <cerrno>
#include <csignal>
#include <libmaus/util/WriteableString.hpp>
#include <libmaus/exception/LibMausException.hpp>
#include <libmaus/util/stringFunctions.hpp>
#include <libmaus/util/MemLimit.hpp>
#include <libmaus/lsf/LSFStateBase.hpp>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace libmaus
{
	namespace util
	{
	        struct ForkProcess
		{
			typedef ForkProcess this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			pid_t id;
			// 0 read
			// 1 write
			int failpipe[2];
			bool joined;
			bool result;
			std::string failmessage;
			
			void kill(int sig = SIGTERM)
			{
			        ::kill(id,sig);
			}
			
			void init(
				std::string const & exe,
				std::string const & pwd,
				std::vector < std::string > const & rargs,
				uint64_t const maxmem = std::numeric_limits<uint64_t>::max(),
				std::string const infilename = std::string(),
				std::string const outfilename = std::string(),
				std::string const errfilename = std::string()
				)
			{
				std::vector < std::string > args;
				args.push_back(exe);
				for ( uint64_t i = 0; i < rargs.size(); ++i )
					args.push_back(rargs[i]);

				typedef ::libmaus::util::WriteableString string_type;
				typedef string_type::unique_ptr_type string_ptr_type;
				::libmaus::autoarray::AutoArray< string_ptr_type > wargv(args.size());
				::libmaus::autoarray::AutoArray< char * > aargv(args.size()+1);
				
				for ( uint64_t i = 0; i < args.size(); ++i )
				{
					wargv[i] = UNIQUE_PTR_MOVE(string_ptr_type(new string_type(args[i])));
					aargv[i] = wargv[i]->A.get();
				}
				
				aargv[args.size()] = 0;
				
				int const fpr = pipe(&failpipe[0]);
				
				if ( fpr == -1 )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "ForkProcess(): pipe() failed: " << strerror(errno);
					se.finish();
					throw se;
				}
				
				id = fork();

				if ( id == static_cast<pid_t>(-1) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "ForkProcess(): fork() failed: " << strerror(errno);
					se.finish();
					throw se;
				}
				else if ( id == 0 )
				{
					close(failpipe[0]);
					
					try
					{
						int const dirret = chdir ( pwd.c_str() );
						if ( dirret != 0 )
						{
							::libmaus::exception::LibMausException se;
							se.getStream() << "chdir(\"" << pwd << "\") failed: " << strerror(errno) << std::endl;
							se.finish();
							throw se;
						}

						if ( maxmem != std::numeric_limits<uint64_t>::max() )
						        ::libmaus::util::MemLimit::setLimits(maxmem);
						
						if ( infilename != std::string() )
						{
							close(STDIN_FILENO);
							int const infd = open(infilename.c_str(),O_RDONLY);
							if ( infd < 0 )
							{
								::libmaus::exception::LibMausException se;
								se.getStream() << "open(\"" << infilename.c_str() << "\",O_RDONLY) failed: " << strerror(errno) << std::endl;
								se.finish();
								throw se;															
							}
							if ( dup2(infd,STDIN_FILENO) == -1 )
							{
								::libmaus::exception::LibMausException se;
								se.getStream() << "dup2() failed for STDIN_FILENO: " << strerror(errno) << std::endl;
								se.finish();
								throw se;																				
							}
						}

						if ( outfilename != std::string() )
						{
							close(STDOUT_FILENO);
							int const outfd = open(outfilename.c_str(),O_WRONLY|O_CREAT,0644);
							if ( outfd < 0 )
							{
								::libmaus::exception::LibMausException se;
								se.getStream() << "open(\"" << outfilename.c_str() << "\",O_WRONLY|O_CREAT," << 0644 <<") failed: " << strerror(errno) << std::endl;
								se.finish();
								throw se;															
							}
							if ( dup2(outfd,STDOUT_FILENO) == -1 )
							{
								::libmaus::exception::LibMausException se;
								se.getStream() << "dup2() failed for STDOUT_FILENO: " << strerror(errno) << std::endl;
								se.finish();
								throw se;																				
							}
						}

						if ( errfilename != std::string() )
						{
							close(STDERR_FILENO);
							int const errfd = open(errfilename.c_str(),O_WRONLY|O_CREAT,0644);
							if ( errfd < 0 )
							{
								::libmaus::exception::LibMausException se;
								se.getStream() << "open(\"" << errfilename.c_str() << "\",O_WRONLY|O_CREAT," << 0644 <<") failed: " << strerror(errno) << std::endl;
								se.finish();
								throw se;															
							}
							if ( dup2(errfd,STDERR_FILENO) == -1 )
							{
								::libmaus::exception::LibMausException se;
								se.getStream() << "dup2() failed for STDERR_FILENO: " << strerror(errno) << std::endl;
								se.finish();
								throw se;																				
							}
						}
					
						execv(exe.c_str(),aargv.get());
										
						::libmaus::exception::LibMausException se;
						se.getStream() << "execv(\""<< exe <<"\") failed: " << strerror(errno) << std::endl;
						se.finish();
						throw se;
					}
					catch(std::exception const & ex)
					{
						std::ostringstream ostr;
						ostr << ex.what();
						std::string const err = ostr.str();
						ssize_t w = write ( failpipe[1], err.c_str(), err.size() );
						if ( w != static_cast<ssize_t>(err.size()) )
							std::cerr << "Write failed: " << strerror(errno) << std::endl;
					}
					
					close ( failpipe[1] );			
					_exit(1);
				}
				else
				{
					close(failpipe[1]);
				}
			}

			ForkProcess(
				std::string const exe,
				std::string const pwd,
				std::vector < std::string > const & rargs,
				uint64_t const maxmem = std::numeric_limits<uint64_t>::max(),
				std::string const infilename = std::string(),
				std::string const outfilename = std::string(),
				std::string const errfilename = std::string()
				)
			: id(-1), joined(false), result(false)
			{
				init(exe,pwd,rargs,maxmem,infilename,outfilename,errfilename);
			}
			
			ForkProcess(
				std::string const cmdline,
				std::string const pwd,
				uint64_t const maxmem = std::numeric_limits<uint64_t>::max(),
				std::string const infilename = std::string(),
				std::string const outfilename = std::string(),
				std::string const errfilename = std::string()
			)
			: id(-1), joined(false), result(false)
			{
				std::deque<std::string> tokens =
					::libmaus::util::stringFunctions::tokenize(cmdline,std::string(" "));
				
				bool foundcmd = false;
				std::string cmd;
				std::vector < std::string > args;
				for ( uint64_t i = 0; i < tokens.size(); ++i )
					if ( tokens[i].size() )
					{
						if ( ! foundcmd )
						{
							foundcmd = true;
							cmd = tokens[i];
						}
						else
							args.push_back(tokens[i]);
					}
				
				if ( foundcmd )
					init(cmd,pwd,args,maxmem,infilename,outfilename,errfilename);
				else
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "ForkProcess called without program name";
					se.finish();
					throw se;
				}
			}

			bool running()
			{
				if ( joined )
					return false;

				fd_set fds;
				FD_ZERO(&fds);
				FD_SET(failpipe[0],&fds);
				struct timeval timeout = {0,0};
				int r = ::select(failpipe[0]+1,&fds,0,0,&timeout);
				return r <= 0;
			}

			bool join()
			{
				if ( ! joined )
				{
					int status;

					std::ostringstream ostr;
					::libmaus::autoarray::AutoArray<char> B(1024);
					ssize_t red = -1;
					while ( (red=read(failpipe[0],B.get(),B.size())) > 0 )
						ostr.write(B.get(),red);
					close(failpipe[0]);

					waitpid(id,&status,0);
					failmessage = ostr.str();
					joined = true;
					result = static_cast<bool>(WIFEXITED(status) && (WEXITSTATUS(status) == 0));
				}
				
				return result;
			}
		};
		
		struct LSFSim
		{
			static void init(std::string const &) {}
			static std::string getClusterName()
			{
				return "local";
			}
		};

		struct ForkProcessLSFSim : public ForkProcess, public ::libmaus::lsf::LSFStateBase
		{
			typedef ForkProcessLSFSim this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			
			static void initLSF(std::string const & s) {
				LSFSim::init(s);	
			}

		        static bool distributeUnique() { return false; }
		
			ForkProcessLSFSim(
				std::string const & scommand,
				std::string const & /* sjobname */,
				std::string const & /* sproject */,
				std::string const & /* queuename */,
				unsigned int const /* numcpu */,
				unsigned int const maxmem,
				std::string const & sinfilename = "/dev/null",
				std::string const & soutfilename = "/dev/null",
				std::string const & serrfilename = "/dev/null",
				std::vector < std::string > const * /* hosts */ = 0,
				char const * cwd = 0,
				uint64_t const = 0 /* tmpspace */
				)
			: ForkProcess(scommand,(cwd?std::string(cwd):std::string(".")),
			  (maxmem*1024ull*1024ull),sinfilename,soutfilename,serrfilename)
			{
			
			}

			state getState()
			{
				if ( isFinished() )
					return state_done;
				else
					return state_run;
			}

			bool isFinished()
			{
				return (!running());
			}
			bool finishedOk()
			{
				if ( isFinished() )
				{
					bool const ok = join();
					return ok;
				}
				else
				{
					return false;
				}
			}
			void wait(int sleepinterval = 1)
			{
				while ( ! isFinished() )
					sleep(sleepinterval);
				join();
			}
			
			std::string getSingleHost() const
			{
			        return "localhost";
			}
					
			bool getHost(::std::vector<std::string> & hostnames)
			{
				hostnames = std::vector < std::string >(1,"localhost");
				return true;
			}
			
			bool isKnown()
			{
				return true;
			}
			
			bool isUnfinished()
			{
				return ! isFinished();
			}

			static std::map<int64_t,int64_t> getIntStates()
			{
				return std::map<int64_t,int64_t>();
			}

			state getState(std::map<int64_t,int64_t> const &)
			{
				if ( isFinished() )
					return state_done;
				else
					return state_run;
			}
			
			#if 0
			int64_t getIntState(std::map<int64_t,int64_t> const & M) const
			{
				return getIntState();
			}
			#endif
		};
	}
}
#endif
