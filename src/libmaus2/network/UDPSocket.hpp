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

#if ! defined(UDPSOCKET_HPP)
#define UDPSOCKET_HPP

#include <string>
#include <cerrno>
#include <cstring>
#include <vector>

#include <libmaus2/exception/LibMausException.hpp>
#include <libmaus2/network/GetDottedAddress.hpp>
#include <libmaus2/bitio/putBit.hpp>
#include <libmaus2/bitio/getBit.hpp>
#include <libmaus2/aio/BufferedOutput.hpp>

#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <poll.h>


namespace libmaus2
{
	namespace network
	{
		struct UDPSocket
		{
			typedef UDPSocket this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
		
			int socket;

			void setBroadcastFlag()
			{
				int broadcast = true;
				
				if ( ::setsockopt(socket,SOL_SOCKET,SO_BROADCAST,&broadcast,sizeof(broadcast)) == -1 )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "setsockopt(SO_BROADCAST) failed: " << strerror(errno) << std::endl;
					se.finish();
					throw se;
				
				}
			}
			UDPSocket(bool const broadcast = false)
			: socket(-1)
			{
				socket = ::socket(AF_INET, SOCK_DGRAM, 0);
				
				if ( socket < 0 )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "socket() failed: " << strerror(errno) << std::endl;
					se.finish();
					throw se;
				}
				
				if ( broadcast )
				{
					try
					{
						setBroadcastFlag();
					}
					catch(...)
					{
						close(socket);
						throw;
					}
				}
			}
			
			static unique_ptr_type allocateServerSocket(Interface const & interface, unsigned short const port, bool const broadcast = false)
			{
				return unique_ptr_type ( new this_type (interface,port,broadcast) );
			}

			static unique_ptr_type allocateServerSocket(unsigned short const port, bool const broadcast = false)
			{
				return unique_ptr_type ( new this_type (getHostNameInterface(),port,broadcast) );
			}

			static unique_ptr_type allocateBroadcastReveicer(unsigned short const port)
			{
				return unique_ptr_type ( new this_type (getHostNameInterface().baddr,port,true) );
			}

			UDPSocket(::std::vector<uint8_t> const addr, unsigned short const port, bool const broadcast = false)
			{
				sockaddr_in inaddr;
				memset(&inaddr, 0, sizeof(sockaddr_in));
				inaddr.sin_family = AF_INET;
				inaddr.sin_port = htons(port);
				inaddr.sin_addr.s_addr =
					htonl((static_cast<uint32_t>(addr[0]) << 24) |
					(static_cast<uint32_t>(addr[1]) << 16) |
					(static_cast<uint32_t>(addr[2]) <<  8) |
					(static_cast<uint32_t>(addr[3]) <<  0));

				socket = ::socket(AF_INET, SOCK_DGRAM, 0);
				
				if ( socket == -1 )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "socket() failed: " << strerror(errno) << std::endl;
					se.finish();
					throw se;
				}
				
				if ( bind ( socket, reinterpret_cast<sockaddr const *>(&inaddr), sizeof(inaddr) ) == -1 )
				{
					if ( errno == EADDRINUSE )
					{
						close(socket);
						throw std::runtime_error("bind() failed: address is in use");
					}
					else
					{
						::libmaus2::exception::LibMausException se;
						se.getStream() << "bind() failed: " << strerror(errno) << std::endl;
						se.finish();
						close(socket);
						throw se;
					}
				}
				
				#if 0 // requires super user privileges on linux
				if ( ::setsockopt(socket,SOL_SOCKET,SO_BINDTODEVICE,interface.name.c_str(),
					interface.name.size()) == -1 )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "setsockopt(SO_BINDTODEVICE) failed: " << strerror(errno) << std::endl;
					se.finish();
					close(socket);
					throw se;
				}
				#endif
				
				if ( broadcast )
				{
					try
					{
						setBroadcastFlag();
					}
					catch(...)
					{
						close(socket);
						throw;
					}
				}
			}

			UDPSocket(Interface const & interface, unsigned short const port, bool const broadcast = false)
			{
				sockaddr_in inaddr;
				memset(&inaddr, 0, sizeof(sockaddr_in));
				inaddr.sin_family = AF_INET;
				inaddr.sin_port = htons(port);
				inaddr.sin_addr.s_addr =
					htonl((static_cast<uint32_t>(interface.addr[0]) << 24) |
					(static_cast<uint32_t>(interface.addr[1]) << 16) |
					(static_cast<uint32_t>(interface.addr[2]) <<  8) |
					(static_cast<uint32_t>(interface.addr[3]) <<  0));

				socket = ::socket(AF_INET, SOCK_DGRAM, 0);
				
				if ( socket == -1 )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "socket() failed: " << strerror(errno) << std::endl;
					se.finish();
					throw se;
				}
				
				if ( bind ( socket, reinterpret_cast<sockaddr const *>(&inaddr), sizeof(inaddr) ) == -1 )
				{
					if ( errno == EADDRINUSE )
					{
						close(socket);
						throw std::runtime_error("bind() failed: address is in use");
					}
					else
					{
						::libmaus2::exception::LibMausException se;
						se.getStream() << "bind() failed: " << strerror(errno) << std::endl;
						se.finish();
						close(socket);
						throw se;
					}
				}
				
				#if 0 // requires super user privileges on linux
				if ( ::setsockopt(socket,SOL_SOCKET,SO_BINDTODEVICE,interface.name.c_str(),
					interface.name.size()) == -1 )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "setsockopt(SO_BINDTODEVICE) failed: " << strerror(errno) << std::endl;
					se.finish();
					close(socket);
					throw se;
				}
				#endif
				
				if ( broadcast )
				{
					try
					{
						setBroadcastFlag();
					}
					catch(...)
					{
						close(socket);
						throw;
					}
				}
			}
			~UDPSocket()
			{
				close(socket);
			}

			bool hasData(unsigned int timeout, int & num)
			{
				pollfd fd;
				fd.fd = socket;
				fd.events = POLLIN;
				fd.revents = 0;
				int const r = poll ( &fd, 1, timeout );
				if ( r < 0 )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "poll() failed: " << strerror(errno) << std::endl;
					se.finish();
					throw se;
				}
				if ((r == 1) && (fd.revents & POLLIN))
				{
					num = -1;
					if ( ioctl(socket,FIONREAD,&num) < 0 )
					{
						::libmaus2::exception::LibMausException se;
						se.getStream() << "ioctl() failed: " << strerror(errno) << std::endl;
						se.finish();
						throw se;		
					}
					return true;
				}
				else
				{
					return false;
				}
			}
			
			std::string recvfromString(size_t bufsize)
			{
				::libmaus2::autoarray::AutoArray<uint8_t> B = recvfrom(bufsize);
				return std::string(
					reinterpret_cast<char const *>(B.begin()),
					reinterpret_cast<char const *>(B.end()));
			}

			::libmaus2::autoarray::AutoArray<uint8_t> recvfrom(size_t bufsize)
			{
				::libmaus2::autoarray::AutoArray<uint8_t> B(bufsize,false);
				recvfrom(reinterpret_cast<char *>(B.get()),bufsize);
				return B;
			}

			void recvfrom(char * buffer, size_t const bufsize)
			{
				sockaddr_in inaddr;
				socklen_t inaddrlen = sizeof(inaddr);
				
				ssize_t const r = ::recvfrom(socket, buffer, bufsize, 0 /* flags */,
					reinterpret_cast<sockaddr *>(&inaddr),
					&inaddrlen);
				
				if ( r < 0 )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Failed to recvfrom message of length " << bufsize << " from UDP socket: " << strerror(errno) << std::endl;
					se.finish();
					throw se;	
				}
				else if ( r != static_cast<ssize_t>(bufsize) )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Received message of length " << r << " when expecting size " << bufsize << std::endl;
					se.finish();
					throw se;
				}
			}
			
			void sendto(
				Interface const & interface, unsigned short const port,
				char const * msg, uint64_t const n
			)
			{
				sockaddr_in inaddr;
				memset(&inaddr, 0, sizeof(sockaddr_in));
				inaddr.sin_family = AF_INET;
				inaddr.sin_port = htons(port);
				inaddr.sin_addr.s_addr =
					htonl((static_cast<uint32_t>(interface.addr[0]) << 24) |
					(static_cast<uint32_t>(interface.addr[1]) << 16) |
					(static_cast<uint32_t>(interface.addr[2]) <<  8) |
					(static_cast<uint32_t>(interface.addr[3]) <<  0));

				while ( ::sendto(socket,msg,n,0,reinterpret_cast<sockaddr const *>(&inaddr),sizeof(inaddr)) < 0 )
				{
					if ( errno == ENOBUFS )
					{
						usleep(1000);
					}
					else
					{
						::libmaus2::exception::LibMausException se;
						se.getStream() << "sendto failed: " << strerror(errno) << std::endl;
						se.finish();
						throw se;
					}
				}
			}

			void sendtobroadcast(
				Interface const & interface, unsigned short const port,
				char const * msg, uint64_t const n
			)
			{
				sockaddr_in inaddr;
				memset(&inaddr, 0, sizeof(sockaddr_in));
				inaddr.sin_family = AF_INET;
				inaddr.sin_port = htons(port);
				inaddr.sin_addr.s_addr =
					htonl((static_cast<uint32_t>(interface.baddr[0]) << 24) |
					(static_cast<uint32_t>(interface.baddr[1]) << 16) |
					(static_cast<uint32_t>(interface.baddr[2]) <<  8) |
					(static_cast<uint32_t>(interface.baddr[3]) <<  0));

				while ( ::sendto(socket,msg,n,0,reinterpret_cast<sockaddr const *>(&inaddr),sizeof(inaddr)) < 0 )
				{
					if ( errno == ENOBUFS )
					{
						// usleep(1000);
					}
					else
					{
						::libmaus2::exception::LibMausException se;
						se.getStream() << "sendto failed: " << strerror(errno) << std::endl;
						se.finish();
						throw se;
					}
				}
			}
			
			template<typename N>
			static uint64_t decodePackage(
				::libmaus2::autoarray::AutoArray<uint8_t> const & B,
				N * A,
				uint64_t const n,
				uint64_t const paybytes
				)
			{
				// uint64_t const packsize = B.size();
				// uint64_t const paybytes = packsize-sizeof(uint64_t);
				uint64_t const totalpaybytes = n*sizeof(N);
				uint64_t const fullpackets = totalpaybytes / paybytes;
				uint64_t const restbytes = totalpaybytes - fullpackets * paybytes;
			
				uint8_t const * U = reinterpret_cast<uint8_t const *>(B.get());
				uint64_t const i =
					(static_cast<uint64_t>(U[0]) << (7*8)) |
					(static_cast<uint64_t>(U[1]) << (6*8)) |
					(static_cast<uint64_t>(U[2]) << (5*8)) |
					(static_cast<uint64_t>(U[3]) << (4*8)) |
					(static_cast<uint64_t>(U[4]) << (3*8)) |
					(static_cast<uint64_t>(U[5]) << (2*8)) |
					(static_cast<uint64_t>(U[6]) << (1*8)) |
					(static_cast<uint64_t>(U[7]) << (0*8));
				uint8_t * output = reinterpret_cast<uint8_t *>(A) + i * paybytes;
				
				if ( i < fullpackets )
					std::copy ( B.get() + sizeof(uint64_t), B.end(), output );
				else
					std::copy ( B.get() + sizeof(uint64_t), B.get()+sizeof(uint64_t)+restbytes, output );
				
				return i;
			}

			template<typename N>
			static uint64_t fillPackage(
				::libmaus2::autoarray::AutoArray<char> & B,
				N const * A,
				uint64_t const n,
				uint64_t const i
				)
			{
				uint64_t const packsize = B.size();
				uint64_t const paybytes = packsize-sizeof(uint64_t);
				uint64_t const totalpaybytes = n*sizeof(N);
				uint64_t const fullpackets = totalpaybytes / paybytes;
				uint64_t const restbytes = totalpaybytes - fullpackets * paybytes;

				char const * input = reinterpret_cast<char const *>(A) + i*paybytes;
				uint8_t * U = reinterpret_cast<uint8_t *>(B.get());
				U[0] = (i >> (7*8)) & 0xFF;
				U[1] = (i >> (6*8)) & 0xFF;
				U[2] = (i >> (5*8)) & 0xFF;
				U[3] = (i >> (4*8)) & 0xFF;
				U[4] = (i >> (3*8)) & 0xFF;
				U[5] = (i >> (2*8)) & 0xFF;
				U[6] = (i >> (1*8)) & 0xFF;
				U[7] = (i >> (0*8)) & 0xFF;
				
				if ( i < fullpackets )
				{
					::std::copy(input,input+paybytes,B.begin()+sizeof(uint64_t));
					return packsize;
				}
				else
				{
					::std::copy(input,input+restbytes,B.begin()+sizeof(uint64_t));
					return sizeof(uint64_t)+restbytes;
				}
			}
			
			template<typename N>
			static void sendArrayBroadcast(
				Interface const & interface,
				unsigned short const broadcastport,
				::libmaus2::autoarray::AutoArray < ::libmaus2::network::ClientSocket::unique_ptr_type > & secondarysockets,
				N const * A,
				uint64_t const n,
				uint64_t const packsize = 508
				)
			{
				#define BROADCASTSENDDEBUG
				
				#if defined(BROADCASTSENDDEBUG)
				std::cerr << "Sending meta...";
				#endif
				for ( uint64_t i = 0; i < secondarysockets.size(); ++i )
				{
					secondarysockets[i]->writeSingle<uint64_t>(n);
					secondarysockets[i]->writeSingle<uint64_t>(packsize);
				}
				// read ack
				for ( uint64_t i = 0; i < secondarysockets.size(); ++i )
				{
					secondarysockets[i]->readSingle<uint64_t>();
				}
				#if defined(BROADCASTSENDDEBUG)
				std::cerr << "done." << std::endl;
				#endif
					
				UDPSocket udpsock(true);
				::libmaus2::autoarray::AutoArray<char> B(packsize);

				uint64_t const paybytes = packsize-sizeof(uint64_t);
				uint64_t const totalpaybytes = n*sizeof(N);
				uint64_t const numpackages = (totalpaybytes + paybytes-1)/paybytes;

				::libmaus2::timing::RealTimeClock rtc;
				rtc.start();
				
				#if defined(BROADCASTSENDDEBUG)
				std::cerr << "Broadcasting...";
				#endif
				for ( uint64_t i = 0; i < numpackages; ++i )
				{
					#if 0
					std::cerr << "Sending " << i+1 << "/" << numpackages << "...";
					#endif
					uint64_t const m = fillPackage(B,A,n,i);
					udpsock.sendtobroadcast(interface,broadcastport,B.begin(),m);
					#if 0
					std::cerr << "done." << std::endl;
					#endif
				}
				#if defined(BROADCASTSENDDEBUG)
				std::cerr << "done." << std::endl;
				#endif

				#if defined(BROADCASTSENDDEBUG)
				std::cerr << "Post broadcast barrier...";
				#endif
				for ( uint64_t i = 0; i < secondarysockets.size(); ++i )
					secondarysockets[i]->writeSingle<uint64_t>(0);
				for ( uint64_t i = 0; i < secondarysockets.size(); ++i )
					secondarysockets[i]->readSingle<uint64_t>();
				#if defined(BROADCASTSENDDEBUG)
				std::cerr << "done." << std::endl;
				#endif

				#if defined(BROADCASTSENDDEBUG)
				std::cerr << "Sending rest...";
				#endif
				uint64_t resent = 0;
				for ( uint64_t i = 0; i < secondarysockets.size(); ++i )
				{
					bool running = true;
					
					while ( running )
					{
						uint64_t tag;
						::libmaus2::autoarray::AutoArray<uint64_t> Q = secondarysockets[i]->readMessage<uint64_t>(tag);
						
						if ( tag == 1 )
						{
							running = false;
						}
						else
						{
							for ( uint64_t j = 0; j < Q.size(); ++j )
							{
								uint64_t const m = fillPackage(B,A,n,Q[j]);
								secondarysockets[i]->writeMessage<char>(0,B.get(),m);
								resent++;
							
							}
						}
					}
				}
				#if defined(BROADCASTSENDDEBUG)
				std::cerr << "done." << std::endl;
				#endif
				
				#if defined(BROADCASTSENDDEBUG)
				std::cerr << "(resent " << resent << "/" << numpackages << ")";
				#endif

				#if defined(BROADCASTSENDDEBUG)
				std::cerr << "Post complete barrier...";
				#endif
				for ( uint64_t i = 0; i < secondarysockets.size(); ++i )
					secondarysockets[i]->writeSingle<uint64_t>(0);
				for ( uint64_t i = 0; i < secondarysockets.size(); ++i )
					secondarysockets[i]->readSingle<uint64_t>();
				#if defined(BROADCASTSENDDEBUG)
				std::cerr << "done." << std::endl;
				#endif
			}
			
			template<typename N>
			static ::libmaus2::autoarray::AutoArray<N> receiveArrayBroadcast(
				::libmaus2::network::UDPSocket::unique_ptr_type & broadcastsocket,
				::libmaus2::network::SocketBase::unique_ptr_type & parentsocket
				)
			{
				::libmaus2::timing::RealTimeClock rtc; rtc.start();

				#if 0
				std::cerr << "Receiving meta...";
				#endif
				uint64_t const n = parentsocket->readSingle<uint64_t>();
				uint64_t const packsize = parentsocket->readSingle<uint64_t>();
				uint64_t const paybytes = packsize - sizeof(uint64_t);
				uint64_t const totalpaybytes = n*sizeof(N);
				uint64_t const numpackages = (totalpaybytes + paybytes-1)/paybytes;
				parentsocket->writeSingle<uint64_t>(0);
				#if 0
				std::cerr << "done." << std::endl;
				#endif
			
				::libmaus2::autoarray::AutoArray<N> A(n,false);
				::libmaus2::autoarray::AutoArray<uint64_t> U((numpackages+63)/64);
			
				uint64_t numrec = 0;
				bool running = true;
				while ( running )
				{
					int num = -1;
					if ( broadcastsocket->hasData(1000,num) )
					{
						#if 0
						std::cerr << "Getting packet of length " << num;
						#endif
						::libmaus2::autoarray::AutoArray<uint8_t> const dgram = 
							broadcastsocket->recvfrom(num);
						uint64_t const i = decodePackage<N>(dgram,A.get(),n,paybytes);
						::libmaus2::bitio::putBit(U.get(),i,1);
						++numrec;
						#if 0
						std::cerr << i << "...done" << std::endl;
						#endif
					}
					else
					{
						// check if sender stoppped sending
						if ( parentsocket->hasData() )
							running = false;
					}
				}
				#if 0
				std::cerr << "Left loop" << std::endl;
				#endif
				
				#if 0
				std::cerr << "Post broadcast barrier...";
				#endif
				parentsocket->barrierRw();
				#if 0
				std::cerr << "done." << std::endl;
				#endif
				
				#if 0
				std::cerr << "Receiving rest packets...";
				#endif
				::libmaus2::aio::BufferedOutputNull<uint64_t> BON(16*1024);
				
				for ( uint64_t i = 0; i < numpackages; ++i )
					if ( ! ::libmaus2::bitio::getBit(U.get(),i) )
					{
						bool const full = BON.put(i);
						if ( full )
						{
							parentsocket->writeMessage<uint64_t>(0,BON.pa,BON.fill());
							for ( uint64_t j = 0; j < BON.fill(); ++j )
							{
								::libmaus2::autoarray::AutoArray<uint8_t> const dgram = parentsocket->readMessage<uint8_t>();
								uint64_t const k = decodePackage<N>(dgram,A.get(),n,paybytes);
								assert ( k == BON.pa[j] );
							}
							BON.reset();
						}
					}
				if ( BON.fill() )
				{
					parentsocket->writeMessage<uint64_t>(0,BON.pa,BON.fill());
					for ( uint64_t j = 0; j < BON.fill(); ++j )
					{
						::libmaus2::autoarray::AutoArray<uint8_t> const dgram = parentsocket->readMessage<uint8_t>();
						uint64_t const k = decodePackage<N>(dgram,A.get(),n,paybytes);
						assert ( k == BON.pa[j] );
					}
					BON.reset();
				}
				parentsocket->writeMessage<uint64_t>(1,0,0);
				
				#if 0
				std::cerr << "done." << std::endl;
				#endif
				
				#if 0
				std::cerr << "Complete barrier...";
				#endif
				parentsocket->barrierRw();
				#if 0
				std::cerr << "done." << std::endl;
				#endif

				std::cerr << "Received array of size " << A.size()*sizeof(N) << " bytes in time " <<
					rtc.getElapsedSeconds() << " rate " << 
					((A.size()*sizeof(N))/rtc.getElapsedSeconds()) / (1024.0*1024.0) << " MB/s." << std::endl;
				
				return A;
			}
			
			static std::vector < Interface > getNetworkInterfaces()
			{
				UDPSocket sock;

				std::vector < Interface > R;
				::libmaus2::autoarray::AutoArray<char> buf(16*1024);
				ifconf ifc;
				ifc.ifc_len = buf.size();
				ifc.ifc_buf = buf.get();
				
				if ( ::ioctl(sock.socket, SIOCGIFCONF, &ifc) < 0)
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "ioctl failed: " << strerror(errno) << std::endl;
					se.finish();
					throw se;
				}
				
				uint64_t const numinterfaces = ifc.ifc_len / sizeof(ifreq);
				ifreq * ifr = ifc.ifc_req;
				
				for ( uint64_t i = 0; i < numinterfaces; ++i )
				{
					ifreq & entry = ifr[i];
					std::string interfacename = entry.ifr_name;
					sockaddr_in const & sockaddr = reinterpret_cast<sockaddr_in const &>(entry.ifr_addr);
					uint32_t const saddr = ntohl(sockaddr.sin_addr.s_addr);

					std::vector < uint8_t > vaddr(4);
					vaddr[0] = (saddr>>24) & 0xFF;
					vaddr[1] = (saddr>>16) & 0xFF;
					vaddr[2] = (saddr>> 8) & 0xFF;
					vaddr[3] = (saddr>> 0) & 0xFF;
					
					std::vector < uint8_t > baddr(4);
					if( ::ioctl(sock.socket, SIOCGIFBRDADDR, &entry) >= 0)
					{
						sockaddr_in const & broadaddr = reinterpret_cast<sockaddr_in const &>(entry.ifr_broadaddr);
						uint32_t const broadsaddr = ntohl(broadaddr.sin_addr.s_addr);
						baddr[0] = (broadsaddr>>24) & 0xFF;
						baddr[1] = (broadsaddr>>16) & 0xFF;
						baddr[2] = (broadsaddr>> 8) & 0xFF;
						baddr[3] = (broadsaddr>> 0) & 0xFF;				
					}
					std::vector < uint8_t > naddr(4);
					if ( ::ioctl(sock.socket, SIOCGIFNETMASK, &entry) >= 0 )
					{
						sockaddr_in const & broadaddr = reinterpret_cast<sockaddr_in const &>(entry.ifr_broadaddr);
						uint32_t const broadsaddr = ntohl(broadaddr.sin_addr.s_addr);
						naddr[0] = (broadsaddr>>24) & 0xFF;
						naddr[1] = (broadsaddr>>16) & 0xFF;
						naddr[2] = (broadsaddr>> 8) & 0xFF;
						naddr[3] = (broadsaddr>> 0) & 0xFF;
					}
					
					R.push_back(Interface(interfacename,vaddr,baddr,naddr));
				}
				
				return R;
			}
			
			static Interface getHostNameInterface()
			{
				std::vector< std::vector <uint8_t> > hostaddr = ::libmaus2::network::GetDottedAddress::getIP4Address();
				std::vector < Interface > interfaces = getNetworkInterfaces();
				for ( uint64_t i = 0; i < interfaces.size(); ++i )
					for ( uint64_t j = 0; j < hostaddr.size(); ++j )
						if ( interfaces[i].addr == hostaddr[j] )
							return interfaces[i];
				
				::libmaus2::exception::LibMausException se;
				se.getStream() << "No interface for host name " << ::libmaus2::network::GetHostName::getHostName() << " found." << std::endl;
				se.finish();
				throw se;
			}
			static Interface getInterfaceByName(std::string const & name)
			{
				std::vector < Interface > interfaces = getNetworkInterfaces();
				for ( uint64_t i = 0; i < interfaces.size(); ++i )
					if ( interfaces[i].name == name )
							return interfaces[i];
				
				::libmaus2::exception::LibMausException se;
				se.getStream() << "No interface named " << name << " found." << std::endl;
				se.finish();
				throw se;
			}
		};
	}
}
#endif
