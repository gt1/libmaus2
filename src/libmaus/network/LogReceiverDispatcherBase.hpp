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
#if ! defined(LIBMAUS_NETWORK_LOGRECEIVERDISPATCHERBASE_HPP)
#define LIBMAUS_NETWORK_LOGRECEIVERDISPATCHERBASE_HPP

#include <libmaus/network/Socket.hpp>
#include <libmaus/util/LogPipeMultiplexGeneric.hpp>
#include <libmaus/util/ArgInfo.hpp>
#include <libmaus/util/MemLimit.hpp>
#include <libmaus/util/WriteableString.hpp>

namespace libmaus
{
	namespace network
	{
		struct DispatchCallback
		{
			virtual int operator()(::libmaus::util::ArgInfo const &, int const) = 0;
		};
			
		struct StringRecDispatchCallback : public DispatchCallback
		{
			int operator()(::libmaus::util::ArgInfo const &, int const fd)
			{
				::libmaus::network::SocketBase controlsock(fd);
				std::string remmes = controlsock.readString();					
				std::cout << remmes << std::endl;
				return 0;
			}			
		};
		
		struct LsfDispatchCallback : public DispatchCallback
		{
			int operator()(::libmaus::util::ArgInfo const & arginfo, int const fd)
			{
				uint64_t const mem = arginfo.getValue<uint64_t>("mem",2000);
				bool const valgrind = arginfo.getValue<uint64_t>("valgrind",0);
				std::string cmd = arginfo.getValue<std::string>("cmd","/bin/nonexistent");
				
				::libmaus::util::MemLimit::setLimits(mem*1000*1000);
				int const ulimitr = system("ulimit -a 1>&2");
				
				if ( ulimitr != 0 )
				{
					int const error = errno;
					std::cerr << "LsfDispatchCallback::operator(): system(ulimit -a) failed: " << strerror(error) << std::endl;
				}
				
				std::vector < std::string > args;
				if ( valgrind )
				{
					args.push_back("/usr/bin/valgrind");
					args.push_back("--leak-check=full");
					args.push_back(cmd);
					cmd = "/usr/bin/valgrind";
				}
				else
				{
					args.push_back(cmd);
				}
				
				std::ostringstream controlostr;
                                controlostr << "controlfd=" << fd;
                                args.push_back(controlostr.str());
                                
                                std::ostringstream memostr;
                                memostr << "mem=" << mem;
                                args.push_back(memostr.str());
				
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

				std::cerr << "about to execv() for command " << cmd << std::endl;

				int const r = execv(cmd.c_str(),aargv.get());
				int const error = errno;
				std::cerr << "execv() returned with value " << r << " error " << strerror(error) << std::endl;
				
				return error;
			}
		};

		struct LogReceiverDispatcherBase
		{
			static int dispatch(int argc, char ** argv, DispatchCallback * dc)
			{
				try
				{
					::libmaus::util::ArgInfo const arginfo(argc,argv);
					return dispatch(arginfo,dc);
				}
				catch(std::exception const & ex)
				{
					std::cerr << ex.what() << std::endl;
					return EXIT_FAILURE;
				}
			}
		
			static int dispatch(
				::libmaus::util::ArgInfo const & arginfo,
				DispatchCallback * dc
			)
			{
				try
				{
					std::string const sid = arginfo.getValue<std::string>("sid","???");
					std::string const loghostname = arginfo.getValue<std::string>("serverhostname","serverhostname");
					unsigned int const port = arginfo.getValue<uint64_t>("logport",0);
					uint64_t const id = arginfo.getValue<uint64_t>("id",0);

					return dispatch(arginfo,sid,loghostname,port,id,dc);
				}
				catch(std::exception const & ex)
				{
					std::cerr << ex.what() << std::endl;
					return EXIT_FAILURE;
				}
			}
		
			static int dispatch(
				::libmaus::util::ArgInfo const & arginfo,
				std::string const & sid,
				std::string const & loghostname,
				unsigned short const port,
				uint64_t const id,
				DispatchCallback * dc = 0
			)
			{
				try
				{
					::libmaus::util::LogPipeMultiplexGeneric LPMG(loghostname,port,sid,id);
					
					// connect
					::libmaus::network::ClientSocket::unique_ptr_type controlsock = UNIQUE_PTR_MOVE(
							::libmaus::network::ClientSocket::unique_ptr_type(
								new ::libmaus::network::ClientSocket(
									port,loghostname.c_str()
								)
							)
						);
							
					// write session id
					controlsock->writeString(0,sid);		
					// id
					controlsock->writeSingle<uint64_t>(id);						
					// connection type
					controlsock->writeString("control");
					// send host name
					controlsock->writeString(::libmaus::network::GetHostName::getHostName());

					if ( dc )
					{
						int const fd = controlsock->releaseFD();
						return (*dc)(arginfo,fd);
					}				
					else
					{
						return EXIT_SUCCESS;
					}
				}
				catch(std::exception const & ex)
				{
					std::cerr << ex.what() << std::endl;
					return EXIT_FAILURE;
				}
			}
		};
	}
}
#endif
