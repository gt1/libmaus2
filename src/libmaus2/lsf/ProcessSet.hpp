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

#if ! defined(PROCESSSET_HPP)
#define PROCESSSET_HPP

#include <libmaus2/lsf/LSFProcess.hpp>
#include <libmaus2/util/MoveStack.hpp>
#include <libmaus2/network/LogSocket.hpp>
#include <libmaus2/util/ArgInfo.hpp>
#include <libmaus2/parallel/PosixThread.hpp>
#include <set>

namespace libmaus2
{
        namespace lsf
        {
                struct ProcessSet
                {
                        typedef ::libmaus2::lsf::LSFProcess process_type;
                        typedef process_type::unique_ptr_type process_ptr_type;
                        typedef ::libmaus2::util::MoveStack<process_type> process_container_type;
                        typedef ::libmaus2::network::SocketBase socket_type;
                        typedef socket_type::unique_ptr_type socket_ptr_type;
                        typedef ::libmaus2::util::MoveStack<socket_type> socket_container_type;
                        
                        process_container_type P;
                        socket_container_type S;
                        std::vector < std::string > hosts;
                        std::set < std::string > shosts;
                        ::libmaus2::network::LogSocket logsocket;
                        ::libmaus2::util::ArgInfo const & arginfo;

                        ProcessSet(std::string logfilenameprefix, ::libmaus2::util::ArgInfo const & rarginfo)
                        : logsocket(logfilenameprefix), arginfo(rarginfo)
                        {
                        }
                        
                        ~ProcessSet()
                        {
				for ( uint64_t i = 0; i < size(); ++i )
					if ( P[i] )
					{
						P[i]->kill(SIGTERM);
						P.reset(i);	
					}
                        }
                        
                        void barrierRw()
                        {
                                for ( uint64_t i = 0; i < size(); ++i ) socket(i)->barrierR();
                                for ( uint64_t i = 0; i < size(); ++i ) socket(i)->barrierW();
                        }
                        void barrierWr()
                        {
                                for ( uint64_t i = 0; i < size(); ++i ) socket(i)->barrierW();
                                for ( uint64_t i = 0; i < size(); ++i ) socket(i)->barrierR();
                        }
                        
                        // compact process list by killing processes on machines running multiple
                        void compact(uint64_t const thres = std::numeric_limits<uint64_t>::max())
                        {
                                #if defined(COMPACTION_DEBUG)
                                std::cerr << "Running compaction." << std::endl;
                                #endif
                                
                                std::set<std::string> lhosts;
                                
                                ::libmaus2::util::MoveStack<process_type> killlist;
                                
                                uint64_t j = 0;
                                for ( uint64_t i = 0; i < size(); ++i )
                                {
                                        #if defined(COMPACTION_DEBUG)
                                        std::cerr << "Checking process " << i << " id " 
                                                << process(i)->id << " on host ::" << process(i)->getSingleHost() 
                                                << "::" 
                                                << " stored " << hosts[i]
                                                << "...";
                                        #endif
                                        
                                        if ( lhosts.find(hosts[i]) == lhosts.end() )
                                        {
                                                #if defined(COMPACTION_DEBUG)
                                                std::cerr << "keeping." << std::endl;
                                                #endif
                                                lhosts.insert(hosts[i]);
                                                
                                                if ( i != j )
                                                {
                                                        (*(P.array))[j] = UNIQUE_PTR_MOVE((*(P.array))[i]);
                                                        (*(S.array))[j] = UNIQUE_PTR_MOVE((*(S.array))[i]);
                                                        hosts[j] = hosts[i];
                                                }
                                                ++j;                        
                                        }
                                        else
                                        {
                                                #if defined(COMPACTION_DEBUG)
                                                std::cerr << "moving to kill list." << std::endl;
                                                #endif
                                                killlist.push_back((*(P.array))[i]);
                                                S.reset(i);
                                        }
                                }
                                
                                #if defined(COMPACTION_DEBUG)
                                std::cerr << P.fill << "->" << j << std::endl;
                                #endif
                                
                                P.fill = j;
                                S.fill = j;
                                
                                while ( P.fill > thres )
                                {
                                        P.fill--;
                                        S.fill--;
                                        killlist.push_back((*(P.array))[P.fill]);
                                        S.reset(S.fill);
                                }
                                
                                for ( uint64_t i = 0; i < killlist.fill; ++i )
                                        killlist[i]->kill();
                                for ( uint64_t i = 0; i < killlist.fill; ++i )
                                {
                                        killlist[i]->wait(1);
                                        killlist.reset(i);
                                }
                        }
                        
                        uint64_t size() const
                        {
                                return P.fill;
                        }
                        
                        process_type * process(uint64_t i)
                        {
                                return P[i];
                        }
                        socket_type * socket(uint64_t i)
                        {
                                return S[i];
                        }
                        
                        void resetSocket(uint64_t const i)
                        {
                                S.reset(i);
                        }
                        
                        bool select(uint64_t & id)
                        {
                                int maxfd = -1;
                                fd_set fds;
                                FD_ZERO(&fds);
                                for ( uint64_t i = 0; i < size(); ++i )
                                        if ( socket(i) )
                                        {
                                                maxfd = std::max(maxfd,socket(i)->getFD());
                                                FD_SET(socket(i)->getFD(),&fds);
                                        }
                                        
                                if ( maxfd == -1 )
                                        return false;
                                
                                int const r = ::select(maxfd+1,&fds,0,0,0);
                                
                                if ( r <= 0 )
                                        return false;
                                else
                                {
                                        for ( uint64_t i = 0; i < size(); ++i )
                                                if ( socket(i) )
                                                        if ( FD_ISSET(socket(i)->getFD(),&fds) )
                                                                id = i;
                                        return true;
                                }
                        }

                        bool select(uint64_t & id, std::vector<bool> const & active, unsigned int const timeout = 0)
                        {
                                int maxfd = -1;
                                fd_set fds;
                                FD_ZERO(&fds);
                                for ( uint64_t i = 0; i < size(); ++i )
                                        if ( active[i] && socket(i) )
                                        {
                                                maxfd = std::max(maxfd,socket(i)->getFD());
                                                FD_SET(socket(i)->getFD(),&fds);
                                        }
                                        
                                if ( maxfd == -1 )
                                        return false;
                                
                                int r;
                                
                                if ( timeout )
                                {
                                	struct timeval tv = { static_cast<long>(timeout), 0 };
                                	r = ::select(maxfd+1,&fds,0,0,&tv);
				}
				else
                                	r = ::select(maxfd+1,&fds,0,0,0);
                                
                                if ( r <= 0 )
                                        return false;
                                else
                                {
                                        for ( uint64_t i = 0; i < size(); ++i )
                                                if ( active[i] && socket(i) )
                                                        if ( FD_ISSET(socket(i)->getFD(),&fds) )
                                                                id = i;
                                        return true;
                                }
                        }

                        bool selectRandom(
                        	uint64_t & id, 
                        	std::vector<bool> const & active,
                        	unsigned int * seed,
                        	unsigned int const timeout = 0)
                        {
                                int maxfd = -1;
                                fd_set fds;
                                FD_ZERO(&fds);
                                for ( uint64_t i = 0; i < size(); ++i )
                                        if ( active[i] && socket(i) )
                                        {
                                                maxfd = std::max(maxfd,socket(i)->getFD());
                                                FD_SET(socket(i)->getFD(),&fds);
                                        }
                                        
                                if ( maxfd == -1 )
                                        return false;
                                
                                int r;
                                
                                if ( timeout )
                                {
                                	struct timeval tv = { static_cast<long>(timeout), 0 };
                                	r = ::select(maxfd+1,&fds,0,0,&tv);
				}
				else
                                	r = ::select(maxfd+1,&fds,0,0,0);
                                
                                if ( r <= 0 )
                                        return false;
                                else
                                {
                                	uint64_t numready = 0;
                                	
                                        for ( uint64_t i = 0; i < size(); ++i )
                                                if ( active[i] && socket(i) )
                                                        if ( FD_ISSET(socket(i)->getFD(),&fds) )
                                                        	numready++;

					assert ( numready );
					uint64_t selrank = rand_r(seed) % numready;
					id = size();
					
                                        for ( uint64_t i = 0; i < size(); ++i )
                                                if ( active[i] && socket(i) )
                                                        if ( FD_ISSET(socket(i)->getFD(),&fds) )
                                                        	if ( ! selrank-- )
                                                        		id = i;
                                                        		
					assert ( id != size() );

                                        return true;
                                }
                        }
                        
                        struct AsynchronousWaiter : public ::libmaus2::parallel::PosixThread
                        {
                        	typedef AsynchronousWaiter this_type;
                        	typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
                        
                        	ProcessSet * owner;
                                ::libmaus2::autoarray::AutoArray< ::libmaus2::network::LogSocket::server_socket_ptr_type >::unique_ptr_type logsocks;
                                ::libmaus2::autoarray::AutoArray< process_ptr_type >::unique_ptr_type procs;
                                uint64_t const numinst;
                                uint64_t const shostlimit;
                                std::string const progdirname;
                                std::string const scommand;
                                std::vector<bool> active;

                                ::libmaus2::parallel::PosixMutex idsmutex;
                                std::vector <uint64_t> ids;
                                bool running;
                        	
                        	AsynchronousWaiter(
                        		ProcessSet * rowner,
                        		::libmaus2::autoarray::AutoArray< ::libmaus2::network::LogSocket::server_socket_ptr_type >::unique_ptr_type & rlogsocks,
	                                ::libmaus2::autoarray::AutoArray< process_ptr_type >::unique_ptr_type & rprocs,
	                                uint64_t const rnuminst,
	                                uint64_t const rshostlimit,
	                                std::string const rprogdirname,
	                                std::string const rscommand
				)
                        	: owner(rowner), logsocks(UNIQUE_PTR_MOVE(rlogsocks)), procs(UNIQUE_PTR_MOVE(rprocs)), numinst(rnuminst),
                        	  shostlimit(rshostlimit), progdirname(rprogdirname), scommand(rscommand), active(numinst,false),
                        	  idsmutex(), ids(), running(true)
                        	{
                        		start();	
                        	}

	                        bool select(uint64_t & id, unsigned int timeout = 0)
        	                {
        	                	std::vector<bool> activeCopy;
					{
	        	                	::libmaus2::parallel::ScopePosixMutex mutex(idsmutex);
        		                	activeCopy = active;
					}
        	                	return owner->select(id,activeCopy,timeout);
        	                }

	                        bool selectRandom(uint64_t & id, unsigned int * seed, unsigned int const timeout = 0)
	                        {
        	                	std::vector<bool> activeCopy;
					{
	        	                	::libmaus2::parallel::ScopePosixMutex mutex(idsmutex);
        		                	activeCopy = active;
					}
        	                	return owner->selectRandom(id,activeCopy,seed,timeout);	                        	
	                        }
        	                
        	                void add(uint64_t p, uint64_t id)
        	                {
        	                	::libmaus2::parallel::ScopePosixMutex mutex(idsmutex);
        	                	active[p] = true;
					ids.push_back(id);
        	                }

        	                void deactivate(uint64_t p)
        	                {
        	                	::libmaus2::parallel::ScopePosixMutex mutex(idsmutex);
        	                	active[p] = false;
        	                }
        	                
        	                void stop()
        	                {
        	                	::libmaus2::parallel::ScopePosixMutex mutex(idsmutex);
        	                	running = false;
        	                }
        	                
        	                bool getRunning()
        	                {
        	                	::libmaus2::parallel::ScopePosixMutex mutex(idsmutex);
					return running;        	                
        	                }
                        	
                        	virtual void * run()
                        	{	
                        		try
                        		{
						uint64_t lastprinted = numinst;
						uint64_t p = 0;
						
						while ( getRunning() && (p < numinst) && (owner->shosts.size() < shostlimit) )
						{
							uint64_t const id = (owner->P).push_back((*procs)[p]);
							
							std::ostringstream scommandostr;
							scommandostr << progdirname << "/" << scommand;
							
							if ( lastprinted != p )
							{
								std::cerr << "Waiting for process " << p+1 << "/" << numinst << " unique so far " << owner->shosts.size() << "...";
								lastprinted = p;
							}

							::libmaus2::network::LogSocket::server_socket_ptr_type & logsock = (*logsocks)[p];
							socket_ptr_type sock;
							
							while ( getRunning() && (! sock.get()) )
							{
								try
								{
									//std::cerr << "acceptReference(";
									sock = UNIQUE_PTR_MOVE(owner->logsocket.acceptReference(scommandostr.str(),logsock,5));
									//std::cerr << ")";
								}
								catch(std::exception const & ex)
								{
									//std::cerr << ex.what() << std::endl;
									continue;
								}
							}
							if ( ! getRunning() )
								break;
								
							//std::cerr << "(resetting logsocket and pushing accepted...";
								
							logsock.reset();
							(owner->S).push_back(sock);
							
							//std::cerr << ")";

							//std::cerr << "(Getting hostname...";
							std::vector<std::string> hostnames;
							while ( ! owner->process(id)->getHost(hostnames) )
								sleep(1);
							//std::cerr << ")";
								
							if ( hostnames.size() )
							{
								std::cerr << hostnames[0] << std::endl;
								owner->hosts.push_back(hostnames[0]);
								owner->shosts.insert ( hostnames[0] );
							}
							else
							{
								::libmaus2::exception::LibMausException se;
								se.getStream() << "Failed to get host name for LSF process " << (*procs)[p]->id << std::endl;
								se.finish();
								throw se;
							}
						
							add(p,id);

							p += 1;
						}

						#if 0					
						for ( ; p < numinst; ++p )
						{
							(*procs)[p]->kill();
							(*logsocks)[p].reset();
						}
						#endif
					}
					catch(std::exception const & ex)
					{
						std::cerr << ex.what() << std::endl;
					}

					std::cerr << "Leaving AsynchronousWaiter::run()" << std::endl;
					
					return 0;                        	
                        	}
                        };
                        
                        /* */
			AsynchronousWaiter::unique_ptr_type startAsynchronous(
                                uint64_t const numinst,
                                uint64_t const jobnamebase,
                                std::string const & scommand,
                                std::string const & rsjobname,
                                std::string const & sproject,
                                std::string const & queuename,
                                unsigned int const numcpu, // number of requested processors
                                unsigned int const maxmem, // requested memory in megabytes
                                std::string const & sinfilename = "/dev/null",
                                std::string const & soutfilename = "/dev/null",
                                std::string const & serrfilename = "/dev/null", 
                                std::vector < std::string > const * hosts = 0, // host list (empty for any)
                                char const * cwd = 0, // working directory
                                uint64_t const tmpspace = 0, // tmp space
                                uint64_t const shostlimit = std::numeric_limits<uint64_t>::max(), // number of different hosts
                                uint64_t const valgrind = 0, // start processes under valgrind
                                char const * model = 0
                                )
			{
                                std::string const dispatchername = "computeLSFGenericDispatcher";
                                std::string const absprogname = arginfo.getAbsProgName();
                                std::string const progdirname = ::libmaus2::util::ArgInfo::getDirName(absprogname);
                                std::string const dispatcherpath = progdirname + "/" + dispatchername;
                                
                                P.reserve(numinst);
                                S.reserve(numinst);

                                ::libmaus2::autoarray::AutoArray< ::libmaus2::network::LogSocket::server_socket_ptr_type >::unique_ptr_type
                                	logsocks(new ::libmaus2::autoarray::AutoArray< ::libmaus2::network::LogSocket::server_socket_ptr_type >(numinst));
                                ::libmaus2::autoarray::AutoArray< process_ptr_type >::unique_ptr_type procs(
                                	new ::libmaus2::autoarray::AutoArray< process_ptr_type >(numinst)
				);

                                /* submit processes */
                                for ( uint64_t p = 0; p < numinst; ++p )
                                {
                                        std::ostringstream jobnamestr;
                                        jobnamestr << rsjobname << "_" << jobnamebase+p;
                                        std::string const sjobname = jobnamestr.str();
                                
                                        (*logsocks)[p] = UNIQUE_PTR_MOVE(logsocket.getLogSocket());
                                        
                                        std::ostringstream dispostr;
                                        dispostr << dispatcherpath
                                                << " sid=" << logsocket.sid
                                                << " logport=" << (*logsocks)[p]->getPort()
                                                << " valgrind=" << valgrind
                                                << " serverhostname=" << ::libmaus2::network::GetHostName::getHostName()
                                                << " mem=" << maxmem;

                                        std::cerr << "Creating process...";
                                        (*procs)[p] = UNIQUE_PTR_MOVE(process_ptr_type (
                                                new process_type(
                                                        dispostr.str(),
                                                        sjobname,
                                                        sproject,
                                                        queuename,
                                                        numcpu,
                                                        maxmem,
                                                        sinfilename,
                                                        soutfilename,
                                                        serrfilename,
                                                        hosts,
                                                        cwd,
                                                        tmpspace,
                                                        model
                                                        )
                                                ));
                                        std::cerr << "done." << std::endl;
                                }
                                
                                AsynchronousWaiter::unique_ptr_type waiter(new AsynchronousWaiter(this,logsocks,procs,numinst,shostlimit,progdirname,scommand));
                                
                                return UNIQUE_PTR_MOVE(waiter);
			}

                        std::vector<uint64_t> start(
                                uint64_t const numinst,
                                uint64_t const jobnamebase,
                                std::string const & scommand,
                                std::string const & rsjobname,
                                std::string const & sproject,
                                std::string const & queuename,
                                unsigned int const numcpu, // number of requested processors
                                unsigned int const maxmem, // requested memory in megabytes
                                std::string const & sinfilename = "/dev/null",
                                std::string const & soutfilename = "/dev/null",
                                std::string const & serrfilename = "/dev/null", 
                                std::vector < std::string > const * hosts = 0, // host list (empty for any)
                                char const * cwd = 0, // working directory
                                uint64_t const tmpspace = 0, // tmp space
                                uint64_t const shostlimit = std::numeric_limits<uint64_t>::max(), // number of different hosts
                                uint64_t const valgrind = 0, // start processes under valgrind
                                char const * model = 0
                                )
                        {
                                std::string const dispatchername = "computeLSFGenericDispatcher";
                                std::string const absprogname = arginfo.getAbsProgName();
                                std::string const progdirname = ::libmaus2::util::ArgInfo::getDirName(absprogname);
                                std::string const dispatcherpath = progdirname + "/" + dispatchername;

                                ::libmaus2::autoarray::AutoArray< ::libmaus2::network::LogSocket::server_socket_ptr_type > logsocks(numinst);
                                ::libmaus2::autoarray::AutoArray< process_ptr_type > procs(numinst);

                                /* submit processes */
                                for ( uint64_t p = 0; p < numinst; ++p )
                                {
                                        std::ostringstream jobnamestr;
                                        jobnamestr << rsjobname << "_" << jobnamebase+p;
                                        std::string const sjobname = jobnamestr.str();
                                
                                        logsocks[p] = UNIQUE_PTR_MOVE(logsocket.getLogSocket());
                                        
                                        std::ostringstream dispostr;
                                        dispostr << dispatcherpath
                                                << " sid=" << logsocket.sid
                                                << " logport=" << logsocks[p]->getPort()
                                                << " valgrind=" << valgrind
                                                << " serverhostname=" << ::libmaus2::network::GetHostName::getHostName()
                                                << " mem=" << maxmem;

                                        std::cerr << "Creating process...";
                                        procs[p] = UNIQUE_PTR_MOVE(process_ptr_type (
                                                new process_type(
                                                        dispostr.str(),
                                                        sjobname,
                                                        sproject,
                                                        queuename,
                                                        numcpu,
                                                        maxmem,
                                                        sinfilename,
                                                        soutfilename,
                                                        serrfilename,
                                                        hosts,
                                                        cwd,
                                                        tmpspace,
                                                        model
                                                        )
                                                ));
                                        std::cerr << "done." << std::endl;
                                }
                                
                                std::vector <uint64_t> ids;
                                
                                uint64_t p = 0;
                                for ( ; (p < numinst) && (shosts.size() < shostlimit); ++p )
                                {
                                        uint64_t const id = P.push_back(procs[p]);
                                        
                                        std::ostringstream scommandostr;
                                        scommandostr << progdirname << "/" << scommand;
                                        
                                        std::cerr << "Waiting for process " << p+1 << "/" << numinst << " unique so far " << shosts.size() << "...";
                                        socket_ptr_type sock = logsocket.accept(scommandostr.str(),UNIQUE_PTR_MOVE(logsocks[p]));
                                        S.push_back(sock);

                                        std::vector<std::string> hostnames;
                                        while ( ! process(id)->getHost(hostnames) )
                                                sleep(1);
                                                
                                        if ( hostnames.size() )
                                        {
                                                std::cerr << hostnames[0] << std::endl;
                                                this->hosts.push_back(hostnames[0]);
                                                shosts.insert ( hostnames[0] );
                                        }
                                        else
                                        {
                                        	::libmaus2::exception::LibMausException se;
                                        	se.getStream() << "Failed to get host name for LSF process " << procs[p]->id << std::endl;
                                        	se.finish();
                                        	throw se;
                                        }
                                                                                
                                        ids.push_back(id);
                                }
                                
                                for ( ; p < numinst; ++p )
                                {
                                        procs[p]->kill();
                                        logsocks[p].reset();
                                }
                                
                                return ids;
                        }

                        std::vector<uint64_t> startUnique(
                                uint64_t const numinst,
                                uint64_t const quant,
                                uint64_t const jobnamebase,
                                std::string const & scommand,
                                std::string const & rsjobname,
                                std::string const & sproject,
                                std::string const & queuename,
                                unsigned int const numcpu,
                                unsigned int const maxmem,
                                std::string const & sinfilename = "/dev/null",
                                std::string const & soutfilename = "/dev/null",
                                std::string const & serrfilename = "/dev/null",
                                std::vector < std::string > const * hosts = 0,
                                char const * cwd = 0,
                                uint64_t const tmpspace = 0,
                                uint64_t const valgrind = 0,
                                char const * model = 0
                                )
                        {
                                 assert ( size() == 0 );
                                 
                                 uint64_t base = jobnamebase;

                                 if ( process_type::distributeUnique() )
                                 {
	                                while ( shosts.size() < numinst )
					{
                                        	start(quant,base,scommand,rsjobname,sproject,queuename,numcpu,maxmem,
                                                	sinfilename,soutfilename,serrfilename,hosts,cwd,tmpspace,numinst,valgrind,model);
                                        	base += quant;
					}
                                        compact(numinst);
                                        logsocket.reap();
                                 }
				 else
				 {
                                        start(numinst,base,scommand,rsjobname,sproject,queuename,numcpu,maxmem,
                                                sinfilename,soutfilename,serrfilename,hosts,cwd,tmpspace);
                                 }
                                 
                                 std::vector<uint64_t> ids;
                                 for ( uint64_t i = 0; i < numinst; ++i )
                                         ids.push_back(i);
                                 return ids;
                        }
                };
        }
}
#endif
