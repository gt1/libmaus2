/*
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
*/
#if ! defined(LIBMAUS_NETWORK_SINGLEFILESERVER_HPP)
#define LIBMAUS_NETWORK_SINGLEFILESERVER_HPP

#include <libmaus/network/Socket.hpp>
#include <iostream>
#include <set>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <csignal>

namespace libmaus
{
        namespace network
        {
                struct SingleFileServer
                {
                        typedef SingleFileServer this_type;
                        typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
                        typedef ::libmaus::network::ServerSocket server_socket_type;
                        typedef server_socket_type::unique_ptr_type server_socket_ptr_type;

                        std::string filename;
                        libmaus::autoarray::AutoArray<char> data;
                        pid_t pid;
                        unsigned short port;
                        server_socket_ptr_type seso;

                        static void sigchildhandler(int)
                        {
                        	int status;
                        	pid_t pid = waitpid(-1,&status,WNOHANG);
                        }
                        
                        SingleFileServer(
                                std::string const & rfilename,
                                std::string const & shostname,
                                unsigned short rport = 4444,
                                unsigned int const backlog = 128)
                        : filename(rfilename), data(libmaus::autoarray::AutoArray<char>::readFile(filename)), port(rport), 
                          seso(UNIQUE_PTR_MOVE(server_socket_type::allocateServerSocket(port,backlog,shostname.c_str(),8*1024)))
                        {
                        }
                        
                        ~SingleFileServer()
                        {
                        }
                        
                        void exec()
                        {
                                pid = fork();
                                
                                if ( pid < 0 )
                                {
                                        ::libmaus::exception::LibMausException ex;
                                        ex.getStream() << "failed to fork: " << strerror(errno);
                                        ex.finish();
                                        throw ex;
                                }
                                
                                if ( ! pid )
                                {
                                	signal(SIGCHLD,sigchildhandler);
                                
                                        while ( true )
                                        {        
                                                try
                                                {
	                                                ::libmaus::network::SocketBase::unique_ptr_type recsock = seso->accept();
	                                                
                                                        pid_t childpid = fork();
                                                        
                                                        if ( childpid == 0 )
                                                        {
                                                        	try
                                                        	{
                                                        		char const * ptr = data.begin();
                                                        		char const * ptre = data.end();
                                                        		uint64_t const bs = 4096;
                                                        		
                                                        		while ( ptr != ptre )
                                                        		{
                                                        			uint64_t const rest = ptre-ptr;
                                                        			uint64_t const towrite = std::min(bs,rest); 
                                                        			recsock->write(ptr,towrite);
                                                        			ptr += towrite;
                                                        		}
								}
								catch(std::exception const & ex)
								{
									std::cerr << ex.what() << std::endl;
								}

                                                                _exit(0);
                                                        }
                                                }
                                                catch(std::exception const & ex)
                                                {
                                                        std::cerr << "Error in SingleFileServer: " << ex.what() << std::endl;
                                                }
                                        }
                                
                                        _exit(0);
                                }
                        }
                        
                        void shutdown()
                        {
                                kill(pid,SIGTERM);
                        }
                        
                        pid_t getPid() const
                        {
                        	return pid;
                        }
                };
        }
}
#endif
