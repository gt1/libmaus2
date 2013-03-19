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

#if ! defined(LOGSOCKET_HPP)
#define LOGSOCKET_HPP

#include <libmaus/network/Socket.hpp>
#include <libmaus/network/GetHostName.hpp>
#include <iomanip>
#include <fstream>
#include <sys/wait.h>
#include <poll.h>

namespace libmaus
{
	namespace network
	{
		struct LogSocket
		{
			typedef ::libmaus::network::ServerSocket server_socket_type;
			typedef server_socket_type::unique_ptr_type server_socket_ptr_type;

			unsigned short nextlogport;
			std::string const sid;
			uint64_t id;
			std::string const filenameprefix;
			std::vector<pid_t> logpids;

			static std::string constructSid()
			{
				std::ostringstream ostr;
				ostr
					<< ::libmaus::network::GetHostName::getHostName()
					<< "_"
					<< getpid()
					<< "_"
					<< time(0);
				return ostr.str();
			}

			LogSocket(std::string const rfilenameprefix)
			: 
			nextlogport(4445),
			sid(constructSid()),
			id(0),
			filenameprefix(rfilenameprefix)
			{
			}
                        
			server_socket_ptr_type getLogSocket()
			{
				server_socket_ptr_type lp(
					server_socket_type::allocateServerSocket(
						nextlogport,
						16,
						::libmaus::network::GetHostName::getHostName().c_str(),4096
					)
				);
                                
				nextlogport += 1;

				return UNIQUE_PTR_MOVE(lp);
			}
                        
			void logprocess(::libmaus::network::SocketBase::unique_ptr_type socket)
			{
				uint64_t const clientid = id++;
                               
				pid_t pid = fork();
                               
				if ( pid == static_cast<pid_t>(-1) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Failed to fork: " << strerror(errno) << std::endl;
					se.finish();
					throw se;
				}
				else if ( !pid )
				{
					try
					{
						std::ostringstream ostr;
						ostr << filenameprefix << "_" << std::setfill('0') << std::setw(6) << clientid;
						std::string fnprefix = ostr.str();
					       
						std::ofstream out((fnprefix+".out").c_str(),std::ios::binary);
						std::ofstream err((fnprefix+".err").c_str(),std::ios::binary);
				       
						bool running = true;
					       
						while ( running )
						{
							try
							{
								pollfd pfd = { socket->getFD(), POLLIN, 0 };
								int const ready = poll(&pfd,1,-1);
								      
								if ( (ready>0) && (pfd.revents & POLLIN) )
								{
									uint64_t stag;
									::libmaus::autoarray::AutoArray<char> B = socket->readMessage<char>(stag);
													      
									if ( stag == STDOUT_FILENO )
									{
										out.write(B.get(),B.size());
										socket->writeMessage<uint64_t>(0,0,0);
									}
									else if ( stag == STDERR_FILENO )
									{
										err.write(B.get(),B.size());
										err.flush();
										socket->writeMessage<uint64_t>(0,0,0);
									}
									else
									{
										std::cerr << "Unknown tag: " << stag << std::endl;
									}
								}
							}
							catch(std::exception const & ex)
							{
								// std::cerr << ex.what() << std::endl;
								err << "exiting log process for id " << clientid << std::endl;
								running = false;
							}
						}
					       
						out.flush();
						err.flush();
					       
						std::cerr << "log process for id " << clientid << " exiting." << std::endl;
					}
					catch(std::exception const & ex)
					{
						std::cerr << "Caught exception in LogSocket " << ex.what() << std::endl;
					}
					catch(...)
					{
						std::cerr << "Caught unknown exception in LogSocket" << std::endl;
					}
                               
					_exit(0);
				}
				else
				{
					logpids.push_back(pid);
				}
			}

			::libmaus::network::SocketBase::unique_ptr_type accept(
				std::string const & cmdline,
				::libmaus::network::ServerSocket::unique_ptr_type logsock,
				unsigned int timeout = 0
			)
			{
				return acceptReference(cmdline,logsock,timeout);
			}

                        ::libmaus::network::SocketBase::unique_ptr_type acceptReference(
                                std::string const & cmdline,
                                ::libmaus::network::ServerSocket::unique_ptr_type & logsock,
                                unsigned int timeout = 0
                        )
                        {
                                bool accepted = false;
                                bool exceptionpass = false;
                                
                                // accept log socket
                                while ( !accepted )
                                {
                                        try
                                        {
                                        	int const fd = logsock->getFD();
                                        	fd_set readfds;
                                        	FD_ZERO(&readfds);
                                        	FD_SET(fd,&readfds);
                                        	int ready = -1;
                                        	
                                        	// std::cerr << "(select...";
                                        	if (  timeout )
                                        	{
                                        		struct timeval to = { static_cast<long>(timeout), 0 };
                                        		ready = ::select(fd+1, &readfds, 0, 0, &to);
                                        		exceptionpass = true;
						}
						else
						{
                                        		ready = ::select(fd+1, &readfds, 0, 0, 0);							
						}
						// std::cerr << "ready=" << ready << ")";
						
						if ( ! ready )
						{
							::libmaus::exception::LibMausException se;
							se.getStream() << "fd not ready after select in LogSocket::acceptReference(): " << strerror(errno);
							se.finish();
							throw se;
						}
                                        
						// std::cerr << "(accept...";
                                                ::libmaus::network::SocketBase::unique_ptr_type socket;
                                                
                                                try
                                                {
	                                                socket = UNIQUE_PTR_MOVE(logsock->accept());
						}
						catch(std::exception const & ex)
						{
							std::cerr << "Failure in LogSocket::accept():\n" << ex.what() << std::endl;
							throw;
						}
                                                
                                                // std::cerr << ")";
                                                
                                                // std::cerr << "(nodelay...";
                                                socket->setNoDelay();
                                                // std::cerr << ")";
                                                
                                                // std::cerr << "(read sid...";
                                                uint64_t stag;
                                                std::string const remsid = socket->readString(stag);
                                                // std::cerr << ")";
                                                
                                                // std::cerr << "(writing commandline...";
                                                socket->writeString(0,cmdline);
                                                // std::cerr << ")";

                                                if ( remsid == sid )
                                                {
                                                        accepted = true;
                                                        // std::cerr << "(logprocess...";
                                                        logprocess(UNIQUE_PTR_MOVE(socket));
                                                        // std::cerr << ")";
                                                }
                                                else
                                                {
                                                       std::cerr << "Received broken session id " << remsid << std::endl;
                                                }
                                        }
                                        catch(std::exception const & ex)
                                        {
                                        	if ( exceptionpass )
                                        	{
                                        		throw;
                                        	}
                                        	else
                                        	{
	                                                std::cerr << ex.what() << std::endl;
						}
                                        }
                                }
                                
                                accepted = false;
                                
                                ::libmaus::network::SocketBase::unique_ptr_type socket;
                                
                                // accept control socket
                                while ( !accepted )
                                {
                                        try
                                        {
                                                socket = UNIQUE_PTR_MOVE(logsock->accept());
                                                uint64_t stag;
                                                std::string const remsid = socket->readString(stag);

                                                if ( remsid == sid )
                                                {
                                                        accepted = true;                                       
                                                }
                                                else
                                                {
                                                       std::cerr << "Received broken session id " << remsid << std::endl;
                                                }
                                        }
                                        catch(std::exception const & ex)
                                        {
                                                std::cerr << "Failure in accepting control socket:\n" << ex.what() << std::endl;
                                        }
                                }
                                
                                // std::cerr << "(about to leave acceptReference)";
                                
                                return UNIQUE_PTR_MOVE(socket);
                        }
                        
                        void reap()
                        {
                                for ( uint64_t i = 0; i < logpids.size(); ++i )
                                        if ( logpids[i] != static_cast<pid_t>(-1) )
                                        {
                                                int status;
                                                if ( waitpid(logpids[i],&status,WNOHANG) == logpids[i] )
                                                        logpids[i] = static_cast<pid_t>(-1);
                                        }
                        }
                        
                        bool allLogPidsInactive()
                        {
                        	bool inactive = true;
                        	for ( uint64_t i = 0; i < logpids.size(); ++i )
                        		if ( logpids[i] != static_cast<pid_t>(-1) )
                        			inactive = false;
				return inactive;
                        }
                        
                        void join()
                        {
                        	if ( allLogPidsInactive() )
                        		return;
                        		
				reap();

                        	if ( allLogPidsInactive() )
                        		return;
				
				uint64_t const sleeptime = 5;
				std::cerr << "Some log processes still active, waiting " << sleeptime << " seconds and then sending SIGTERM." << std::endl;

				sleep(sleeptime);
                        	for ( uint64_t i = 0; i < logpids.size(); ++i )
                        		if ( logpids[i] != static_cast<pid_t>(-1) )
                        			kill(logpids[i],SIGTERM);

				std::cerr << "Signal SIGTERM sent, waiting for " << sleeptime << " seconds." << std::endl;
				sleep(sleeptime);
				
				reap();
				if ( allLogPidsInactive() )
					return;

				std::cerr << "Some log processes still active, waiting " << sleeptime << " seconds and then sending SIGKILL." << std::endl;

				sleep(sleeptime);
                        	for ( uint64_t i = 0; i < logpids.size(); ++i )
                        		if ( logpids[i] != static_cast<pid_t>(-1) )
                        			kill(logpids[i],SIGKILL);

				std::cerr << "Signal SIGKILL sent, waiting for " << sleeptime << " seconds." << std::endl;
				sleep(sleeptime);
                        
				reap();
				if ( allLogPidsInactive() )
					return;
					
				std::cerr << "Some log processes seem to be ignoring signals, giving up." << std::endl;
                        }
                        
                        ~LogSocket()
                        {
                                join();
                        }
                };
        }
}
#endif
