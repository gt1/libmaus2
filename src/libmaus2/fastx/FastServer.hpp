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

#if ! defined(FASTSERVER_HPP)
#define FASTSERVER_HPP

#include <libmaus/network/Socket.hpp>
#include <libmaus/fastx/FastInterval.hpp>
#include <libmaus/aio/FileFragment.hpp>
#include <libmaus/aio/ReorderConcatGenericInput.hpp>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <csignal>

namespace libmaus
{
        namespace fastx
        {
                struct FastServer
                {
                        typedef FastServer this_type;
                        typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
                        typedef ::libmaus::network::ServerSocket server_socket_type;
                        typedef server_socket_type::unique_ptr_type server_socket_ptr_type;

                        std::vector<std::string> filenames;
                        std::vector< ::libmaus::aio::FileFragment > fragments;
                        std::vector< ::libmaus::fastx::FastInterval > intervals;
                        pid_t pid;
                        unsigned short port;
                        server_socket_ptr_type seso;
                        bool intervalserver;

                        static void sigchildhandler(int)
                        {
                        	int status;
                        	pid_t pid;
                        	
                        	do
                        	{
                        	        pid = waitpid(-1,&status,WNOHANG);
                                } while ( pid > 0 );
                                
                        	// std::cerr << "sigchld for id " << pid << std::endl;
                        }
                        
                        static std::vector < ::libmaus::aio::FileFragment > getFragments(std::vector < std::string > const & filenames)
                        {
                                std::vector < ::libmaus::aio::FileFragment > fragments;
                                for ( uint64_t i = 0; i < filenames.size(); ++i )
                                        fragments.push_back(::libmaus::aio::FileFragment(filenames[i],0,::libmaus::util::GetFileSize::getFileSize(filenames[i])));
                                return fragments;
                        }
                        
                        FastServer(
                                std::vector<std::string> const & rfilenames,
                                std::vector< ::libmaus::fastx::FastInterval > const & rintervals,
                                std::string const & shostname,
                                unsigned short rport = 4444,
                                unsigned int const backlog = 128)
                        : filenames(rfilenames), fragments(getFragments(filenames)), intervals(rintervals), port(rport), 
                          seso(UNIQUE_PTR_MOVE(server_socket_type::allocateServerSocket(port,backlog,shostname.c_str(),32*1024))),
                          intervalserver(false)
                        {
                        }

                        FastServer(
                                std::vector< ::libmaus::aio::FileFragment > const & rfragments,
                                std::vector< ::libmaus::fastx::FastInterval > const & rintervals,
                                std::string const & shostname,
                                unsigned short rport = 4444,
                                unsigned int const backlog = 128)
                        : filenames(rfragments.size()), fragments(rfragments), intervals(rintervals), port(rport), 
                          seso(UNIQUE_PTR_MOVE(server_socket_type::allocateServerSocket(port,backlog,shostname.c_str(),32*1024))),
                          intervalserver(false)
                        {
                                for ( uint64_t i = 0; i < fragments.size(); ++i )
                                        filenames[i] = fragments[i].filename;
                        }
                        
                        void setIntervalMode()
                        {
                        	intervalserver = true;
                        }
                        
                        ~FastServer()
                        {
                                shutdown();
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
                                	bool childrunning = true;
                                
                                        while ( childrunning )
                                        {
                                                try
                                                {
	                                                ::libmaus::network::SocketBase::unique_ptr_type recsock = seso->accept();
	                                                
                                                        pid_t childpid = fork();
                                                        
                                                        if ( childpid == 0 )
                                                        {
                                                                try
                                                                {
                                                                        libmaus::fastx::FastInterval FI;
                                                                        
                                                                        if ( (!intervalserver) )
                                                                        {
	                                                                        uint64_t tag,i,ni;
        	                                                                recsock->readMessage<uint64_t>(tag,&i,ni);
        	                                                                
                                                                         	if ( i < intervals.size() )
	                                                                                FI = intervals[i];
										else
										{
	                                                                        	std::cerr << "interval out of range: " << i << " (number of intervals is " << intervals.size() << ")" << std::endl;
	                                                                        	_exit(1);
										}	
									}
									else
									{
										FI = libmaus::fastx::FastInterval::deserialise(recsock->readString());
									}
                                                                                
									uint64_t const flow = FI.fileoffset;
									uint64_t const fhigh = FI.fileoffsethigh;
									uint64_t const flen = fhigh-flow;
									uint64_t todo = flen;

									::libmaus::aio::ReorderConcatGenericInput<char> SFRB(
										fragments,16*1024,fhigh-flow,flow
									);

									uint64_t const blocksize = 16*1024;
									::libmaus::autoarray::AutoArray<char> B(blocksize,false);

									while ( todo )
									{
										uint64_t const toproc = std::min(blocksize,todo);
										
										for ( uint64_t k = 0; k < toproc; ++k )
											B[k] = SFRB.getNextCharacter();
										recsock->write(B.get(),toproc);
										
										todo -= toproc;
									}

                                                                        _exit(0);
                                                                }
                                                                catch(std::exception const & ex)
                                                                {
                                                                        std::cerr << ex.what() << std::endl;
                                                                        _exit(1);
                                                                }
                                                        }
                                                }
                                                catch(std::exception const & ex)
                                                {
                                                        std::cerr << "Error in FastServer: " << ex.what() << std::endl;
                                                }
                                        }
                                
                                        _exit(0);
                                }
                        }
                        
                        void shutdown()
                        {
                                kill(pid,SIGTERM);
                        }
                };
        }
}
#endif
