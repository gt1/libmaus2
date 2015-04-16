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

#include <libmaus/network/LogReceiver.hpp>
#include <libmaus/aio/CheckedOutputStream.hpp>
#include <libmaus/random/Random.hpp>
#include <libmaus/util/MoveStack.hpp>

#include <iomanip>

std::string libmaus::network::LogReceiver::computeSessionId()
{
	std::ostringstream ostr;
	ostr << "logreceiver_" << ::libmaus::network::GetHostName::getHostName() << "_" << getpid() << "_" << time(0);
	return ostr.str();
}

::libmaus::network::LogReceiverTestProcess::unique_ptr_type libmaus::network::LogReceiver::constructLogReceiverTestProcess(
	uint64_t const id,
	::libmaus::network::DispatchCallback * dc)
{
	::libmaus::network::LogReceiverTestProcess::unique_ptr_type ptr(
		::libmaus::network::LogReceiverTestProcess::construct(
                sid,hostname,port,id,getOpenFds(),dc)
	);
	return UNIQUE_PTR_MOVE(ptr);
}

std::vector<int> libmaus::network::LogReceiver::getOpenFds() const
{
	std::vector<int> fds;
	
	if ( logsocket && logsocket->getFD() >= 0 )
		fds.push_back(logsocket->getFD());
	if ( SP && SP->childGet() != -1 )
		fds.push_back(SP->childGet());
	if ( SP && SP->parentGet() != -1 )
		fds.push_back(SP->parentGet());
	if ( SPpass && SPpass->childGet() != -1 )
		fds.push_back(SPpass->childGet());
	if ( SPpass && SPpass->parentGet() != -1 )
		fds.push_back(SPpass->parentGet());
	if ( pcsocket && pcsocket->getFD() >= 0 )
		fds.push_back(pcsocket->getFD());
		
	return fds;
}

libmaus::network::LogReceiver::LogReceiver(
	std::string const & rlogfileprefix,
	unsigned int rport, 
	unsigned int const rbacklog, 
	unsigned int const tries
)
: sid(computeSessionId()), hostname(::libmaus::network::GetHostName::getHostName()), logfileprefix(rlogfileprefix),
  port(rport), logsocket(::libmaus::network::ServerSocket::allocateServerSocket(port,rbacklog,hostname,tries)), 
  SP(new ::libmaus::network::SocketPair),
  pcsocket(new ::libmaus::network::SocketBase(SP->parentRelease())), 
  SPpass(new ::libmaus::network::SocketPair),
  passsem(::libmaus::parallel::NamedPosixSemaphore::getDefaultName(),true /* primary */)
{
	start();
	SP->closeParent();
	SPpass->closeChild();
	logsocket.reset();
	
	passsem.post(); // set semaphore post count to 1
}
libmaus::network::LogReceiver::~LogReceiver()
{
	try
	{
		// std::cerr << "Sending QUIT." << std::endl;
		
		// send termination to log process
		pcsocket->writeString("QUIT");
		
		// std::cerr << "Sent QUIT." << std::endl;
		
		// read ack
		pcsocket->readSingle<uint64_t>();
		
		//std::cerr << "Term sent and acknowledged." << std::endl;
		#if 0
		while ( controlDescriptorPending() )
			getControlDescriptor();
		#endif
	
		// close down communication sockets
		logsocket.reset();	
		pcsocket.reset();
		SP.reset();
		SPpass.reset();
					
		join();
	}
	catch(std::exception const & ex)
	{
		std::cerr << "Failure sending term signal to log process:\n" << ex.what() << std::endl;
	}		
}

std::string libmaus::network::LogReceiver::getLogFileNamePrefix(uint64_t const clientid) const
{
	std::ostringstream ostr;
	ostr << logfileprefix << "_" << std::setfill('0') << std::setw(6) << clientid;
	std::string fnprefix = ostr.str();
	return fnprefix;
}

bool libmaus::network::LogReceiver::controlDescriptorPending()
{
	return SPpass->pending();
}

libmaus::network::LogReceiver::ControlDescriptor libmaus::network::LogReceiver::getControlDescriptor()
{
	::libmaus::network::SocketBase::shared_ptr_type SBS;
	uint64_t id = 0;
	int fd = -1;
	std::string hostname;
	
	try
	{
		// receiver id
		::libmaus::network::SocketBase SB(SPpass->childGet());
		id = SB.readSingle<uint64_t>();
		hostname = SB.readString();
		SB.releaseFD();
			
		fd = SPpass->receiveFd();
			
		if ( fd < 0 )
		{
			::libmaus::exception::LibMausException se;
			se.getStream() << "Received invalid negative file descriptor " << fd << " for id " << id << std::endl;
			se.finish();
			throw se;
		}
			
		SBS = ::libmaus::network::SocketBase::shared_ptr_type(new ::libmaus::network::SocketBase(fd));
		fd = -1;
	}
	catch(...)
	{
		if ( fd != -1 )
		{
			::close(fd);
			fd = -1;
		}
		passsem.assureNonZero();
		throw;
	}

	// std::cerr << "*Got hostname " << hostname << std::endl;

	passsem.assureNonZero();	
	return ControlDescriptor(SBS,id,hostname);
}

static int multiPoll(
	std::map<uint64_t,::libmaus::network::SocketBase::shared_ptr_type> const & logsockets,
	::libmaus::network::ServerSocket::unique_ptr_type const & logsocket,
	::libmaus::network::SocketBase::unique_ptr_type const & cpsocket,
	int & pfd
)
{
	std::vector < int > fds;
	
	for ( std::map<uint64_t,::libmaus::network::SocketBase::shared_ptr_type>::const_iterator ita = 
		logsockets.begin(); ita != logsockets.end(); ++ita )
		if ( ita->second && ita->second->getFD() >= 0 )
			fds.push_back(ita->second->getFD());

	if ( logsocket && logsocket->getFD() >= 0 )
		fds.push_back(logsocket->getFD());

	if ( cpsocket && cpsocket->getFD() >= 0 )
		fds.push_back(cpsocket->getFD());

	return ::libmaus::network::SocketBase::multiPoll(fds,pfd);
}
	
int libmaus::network::LogReceiver::run()
{
	srand(time(0));

	#if 0
	std::cerr << "*** 1 ***" << std::endl;
	printFds();
	#endif

	#if 0
	::libmaus::network::ServerSocket::unique_ptr_type logsocket;
	::libmaus::network::SocketPair::unique_ptr_type SP;
	::libmaus::network::SocketBase::unique_ptr_type pcsocket;
	::libmaus::network::SocketPair::unique_ptr_type SPpass;
	#endif

	/**
	 * close unneeded file descriptors
	 **/
	pcsocket.reset();      // this stores the SP parent socket
	SPpass->closeParent(); // we are the parent for descriptor passing
	
	::libmaus::network::SocketBase::unique_ptr_type cpsocket(new ::libmaus::network::SocketBase(SP->childRelease()));
	assert ( cpsocket->getFD() >= 0 );

	#if 0
	std::cerr << "*** 2 ***" << std::endl;
	printFds();
	#endif

	
	// file descriptors passed to parent by id
	// std::deque< std::pair<uint64_t,::libmaus::network::SocketBase::shared_ptr_type> > passfds;
	// std::deque < ::libmaus::network::FileDescriptorPasser::shared_ptr_type > passprocs;
	::libmaus::util::MoveStack < ::libmaus::network::FileDescriptorPasser > passprocs;
	
	// log files and log file sockets
	std::map<uint64_t,::libmaus::aio::CheckedOutputStream::shared_ptr_type> outfiles;
	std::map<uint64_t,::libmaus::aio::CheckedOutputStream::shared_ptr_type> errfiles;
	std::map<uint64_t,::libmaus::network::SocketBase::shared_ptr_type> logsockets;
	std::map<int,uint64_t> fdToId;
	
	bool running = true;
	

	while ( running || logsockets.size() )
	{
		while ( passprocs.size() )
		{
			// std::cerr << "Checking pass procs." << std::endl;
			assert ( passprocs.back() );
			
			if ( passprocs.back()->tryJoin() )
			{
				// std::cerr << "Popping pass proc." << std::endl;
				passprocs.pop_back();
			}
			else
			{
				break;
			}
		}
	
		int pfd = -1;
		int const ready = multiPoll(logsockets,logsocket,cpsocket,pfd);

		if ( ready < 0 )
		{
			// interrupted by signal? try again
			if ( errno == EINTR || errno == EAGAIN )
			{
				// sleep(1);
			}
			else
			{
				std::cerr << "::poll() failed: " << strerror(errno) << ", log process leaving." << std::endl;
				break;
			}
		}
		else if ( ready == 0 )
		{
			// wait for 1 s
			struct timespec waittime = { 1, 0 };
			nanosleep(&waittime,0);
		}
		else
		{
			// new process connection pending?
			if ( logsocket && logsocket->getFD() == pfd )
			{
				#if defined(LOGDEBUG)
				std::cerr << "[D1] new connection pending." << std::endl;
				#endif
					
				try
				{
					// accept the connection
					::libmaus::network::SocketBase::shared_ptr_type newcon = 
						logsocket->acceptShared();

					// read and session id
					std::string const remsid = newcon->readString();
					
					#if defined(LOGDEBUG)
					std::cerr << "[D2] Remote sid: " << remsid << std::endl;
					#endif
				
					// check sid	
					if ( remsid != sid )
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "logprocess received defect session id, closing connection." << std::endl;
						se.finish();
						throw se;
					}
					
					// read remote id
					uint64_t const remid = newcon->readSingle<uint64_t>();
					// read type of connection
					std::string const contype = newcon->readString();
					
					#if defined(LOGDEBUG)
					std::cerr << "[D3] Remote id: " << remid << std::endl;
					std::cerr << "[D4] Connection type: " << contype << std::endl;
					#endif
					
					// log connection
					if ( contype == "log" )
					{
						logsockets[remid] = newcon;
						fdToId[newcon->getFD()] = remid;

						std::string const lfprefix = getLogFileNamePrefix(remid);
						outfiles[remid] = ::libmaus::aio::CheckedOutputStream::shared_ptr_type(new ::libmaus::aio::CheckedOutputStream(lfprefix+".out"));
						errfiles[remid] = ::libmaus::aio::CheckedOutputStream::shared_ptr_type(new ::libmaus::aio::CheckedOutputStream(lfprefix+".err"));
					}
					// control stream connection
					else if ( contype == "control" )
					{
						std::string const remhostname = newcon->readString();
						// std::cerr << "Received host name " << remhostname << std::endl;
						// std::cerr << "Got control connection from " << remid << std::endl;
						::libmaus::network::FileDescriptorPasser::unique_ptr_type fdp(new ::libmaus::network::FileDescriptorPasser(
							SPpass.get(),remid,newcon.get(),remhostname,passsem.semname));
						passprocs.push_back(fdp);
						// newcon->releaseFD();
					}		
				}
				catch(std::exception const & ex)
				{
					std::cerr << "[D5] logprocess caught exception while accepting new connection: " << ex.what() << std::endl;
				}	
			}
			// incoming message from parent
			else if ( cpsocket && cpsocket->getFD() == pfd )
			{
				try
				{
					std::string const message = cpsocket->readString();
											
					if ( message == "QUIT" )
					{
						// close log socket for new connections
						logsocket.reset();
						// close file descriptor passing 
						SPpass.reset();
						
						// acknowledge shutdown
						cpsocket->writeSingle<uint64_t>();
						
						// reset cp socket, so we do not try to read from it again
						cpsocket.reset();
					
						#if defined(LOGDEBUG)
						std::cerr << "[D6] Received QUIT message from parent." << std::endl;
						#endif
						running = false;							
					}
					else
					{
						std::cerr << "[D7] Received unknown message " << message << " from parent." << std::endl;
					}
				}
				catch(std::exception const & ex)
				{
					std::cerr << "[D8] log process caught exception while reading from parent: " << ex.what() << std::endl;
					// printFds();
					break;
				}
			}
			else
			{
				int64_t cid = -1;
			
				try
				{
					#if defined(LOGDEBUG)
					std::cerr << "[D9] Incoming log data." << std::endl;
					#endif
					
					std::vector<uint64_t> activeids;
					
					for (
						std::map<uint64_t,::libmaus::network::SocketBase::shared_ptr_type>::const_iterator ita = logsockets.begin();
						ita != logsockets.end();
						++ita )
						if ( ita->second->getFD() == pfd )
						{
							activeids.push_back(ita->first);
						}
						
					if ( ! activeids.size() )
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "logprocess select() returned value > 0, but no file descriptor seems ready." << std::endl;
						se.finish();
						throw se;						
					}
					

					#if defined(LOGDEBUG)
					std::cerr << "[D10] Number of active ids: " << activeids.size() << std::endl;
					#endif
					
					// randomly choose any active id
					cid = activeids[::libmaus::random::Random::rand64() % activeids.size()];
					

					#if defined(LOGDEBUG)
					std::cerr << "[D11] Chosen active id: " << cid << std::endl;
					#endif

					//
					assert ( logsockets.find(cid) != logsockets.end() );
					//
					::libmaus::network::SocketBase::shared_ptr_type & socket = logsockets.find(cid)->second;
			
					// receive log data		
					uint64_t stag;
					::libmaus::autoarray::AutoArray<char> B = socket->readMessage<char>(stag);
					

					#if defined(LOGDEBUG)
					std::cerr << "[D12] Received message, tag is " << stag << " length of message is " << B.size() << std::endl;
					#endif
		      
					// write it to file chosen by tag
					if ( stag == STDOUT_FILENO )
					{
						assert ( outfiles.find(cid) != outfiles.end() );
						outfiles.find(cid)->second->write(B.get(),B.size());
						socket->writeSingle<uint64_t>(0);
					}
					else if ( stag == STDERR_FILENO )
					{
						assert ( errfiles.find(cid) != errfiles.end() );
						errfiles.find(cid)->second->write(B.get(),B.size());
						errfiles.find(cid)->second->flush();
						socket->writeSingle<uint64_t>(0);
					}
					// termination
					else if ( static_cast<int64_t>(stag) == std::max(STDOUT_FILENO,STDERR_FILENO)+1 )
					{
						assert ( errfiles.find(cid) != errfiles.end() );
						errfiles.find(cid)->second->write(B.get(),B.size());
						errfiles.find(cid)->second->flush();

						// send ack
						socket->writeSingle<uint64_t>(0);

						if ( logsockets.find(cid) != logsockets.end() )
							logsockets.erase(logsockets.find(cid));
							
						if ( outfiles.find(cid) != outfiles.end() )
						{
							try
							{
								outfiles.find(cid)->second->flush();
								outfiles.erase(outfiles.find(cid));
							}
							catch(std::exception const & ex)
							{
								std::cerr << "[D13] " << ex.what();
							}
						}

						if ( errfiles.find(cid) != errfiles.end() )
						{
							try
							{
								errfiles.find(cid)->second->flush();
								errfiles.erase(errfiles.find(cid));
							}
							catch(std::exception const & ex)
							{
								std::cerr << "[D14] " << ex.what();
							}
						}

						#if defined(LOGDEBUG)
						std::cerr << "[D15] log process received termination from id " << cid << "." << std::endl;
						#endif
					}
					else
					{
						std::cerr << "[D16] Unknown tag: " << stag << std::endl;
						socket->writeSingle<uint64_t>(0);
					}
				}
				catch(std::exception const & ex)
				{
					std::cerr << "[D17] log process caught exception while reading log data: " << ex.what() << std::endl;	
					
					if ( cid >= 0 )
					{
						if ( logsockets.find(cid) != logsockets.end() )
							logsockets.erase(logsockets.find(cid));
							
						if ( outfiles.find(cid) != outfiles.end() )
						{
							try
							{
								outfiles.find(cid)->second->flush();
								outfiles.erase(outfiles.find(cid));
							}
							catch(std::exception const & ex)
							{
								std::cerr << "[D18] " << ex.what();
							}
						}

						if ( errfiles.find(cid) != errfiles.end() )
						{
							try
							{
								errfiles.find(cid)->second->flush();
								errfiles.erase(errfiles.find(cid));
							}
							catch(std::exception const & ex)
							{
								std::cerr << "[D19] " << ex.what();
							}
						}

						std::cerr << "[D20] log process dropped id " << cid << "." << std::endl;
					}
				}
			}
		}
	}
	
	#if defined(LOGDEBUG)
	std::cerr << "[D21] log process about to terminate." << std::endl;
	#endif
	
	return 0;
}
