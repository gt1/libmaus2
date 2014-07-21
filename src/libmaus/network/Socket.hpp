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

#if ! defined(LIBMAUS_SOCKET_HPP)
#define LIBMAUS_SOCKET_HPP

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <libgen.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <limits.h>
#include <sys/resource.h>

#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <iostream>
#include <set>

#include <libmaus/exception/LibMausException.hpp>
#include <libmaus/util/unique_ptr.hpp>
#include <libmaus/util/shared_ptr.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/timing/RealTimeClock.hpp>
#include <libmaus/network/GetHostName.hpp>
#include <libmaus/network/SocketInputOutputInterface.hpp>
#include <stdexcept>

#include <libmaus/parallel/PosixThread.hpp>
#include <libmaus/parallel/SynchronousQueue.hpp>

namespace libmaus
{
	namespace network
	{
		struct SocketBase : public SocketInputOutputInterface
		{
			public:
			typedef SocketBase this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

			static int multiPoll(std::vector<int> const & fds, int & pfd)
			{
				// std::cerr << "Multi poll " << fds.size() << std::endl;
			
				uint64_t const limit = RLIMIT_NOFILE;
				uint64_t const numfd = fds.size();
				::libmaus::autoarray::AutoArray < struct pollfd > PFD(std::min(limit,numfd));
				uint64_t const loops = (fds.size() + limit - 1 )/limit;
				
				for ( uint64_t l = 0; l < loops; ++l )
				{
					uint64_t const low = l*limit;
					uint64_t const high = std::min(low+limit,numfd);
					
					for ( uint64_t i = 0; i < (high-low); ++i )
					{
						PFD[i].fd = fds[low+i];
						PFD[i].events = POLLIN;
						PFD[i].revents = 0;
					}
					
					int const r = poll(PFD.begin(),high-low,0);
					
					// std::cerr << "l=" << (l+1) << "/" << loops << " r=" << r << std::endl;

					if ( r > 0 )
					{
						for ( uint64_t i = 0; i < (high-low); ++i )
							if ( PFD[i].revents )
							{
								pfd = PFD[i].fd;
								// std::cerr << "pfd=" << pfd << " events " << PFD[i].revents << std::endl;
								return 1;
							}
						
						std::cerr << "WARNING: poll returned " << r << " but no file descriptor has events" << std::endl;
					}
					else if ( r < 0 )
					{
						#if 0
						int const error = errno;
						std::cerr << "WARNING: poll returned error: " << strerror(error) << std::endl;
						#endif
						return r;
					}
				}

				return 0;
			}

			private:
			int fd;
			static unsigned int const checkinterval = 300*1000;

			protected:
			struct sockaddr_in remaddr;
			bool remaddrset;
			

			private:
			void cleanup()
			{
				if ( fd != -1 )
				{
					close(fd);
					fd = -1;
				}
			}
			
			std::string getStringAddr()
			{
				if ( remaddrset )
				{
					std::ostringstream ostr;
					uint32_t const rem = ntohl(remaddr.sin_addr.s_addr);
					ostr
						<< ((rem >> 24) & 0xFF) << "."
						<< ((rem >> 16) & 0xFF) << "."
						<< ((rem >>  8) & 0xFF) << "."
						<< ((rem >>  0) & 0xFF);
					return ostr.str();
				}
				else
				{
					return "<unknown addr>";
				}
			}

			public:
			void write(char const * data, size_t len)
			{
				while ( len )
				{
					pollfd pfd = { getFD(), POLLOUT, 0 };
					int const ready = poll(&pfd, 1, checkinterval);
					
					if ( ready == 1 && (pfd.revents & POLLOUT) )
					{
						ssize_t wr = ::write ( fd, data, len );
						
						if ( wr < 0 )
						{
							if ( errno == EINTR )
							{
								std::cerr << "write interrupted by signal." << std::endl;
							}
							else
							{
								::libmaus::exception::LibMausException se;
								se.getStream() << "SocketBase::write() to " << getStringAddr() << " failed: " << strerror(errno);
								se.finish();
								throw se;	
							}
						}
						
						data += wr;
						len -= wr;
					}
					else
					{
						std::cerr << "Waiting for fd=" << getFD() << " to become ready for writing"; 
						if ( remaddrset )
						{
							uint32_t const rem = ntohl(remaddr.sin_addr.s_addr);
							std::cerr << " remote " 
								<< ((rem >> 24) & 0xFF) << "."
								<< ((rem >> 16) & 0xFF) << "."
								<< ((rem >>  8) & 0xFF) << "."
								<< ((rem >>  0) & 0xFF);
						}
						std::cerr << " remaining packet portion " << len;
						std::cerr << std::endl;
						::libmaus::util::StackTrace ST;
						std::cerr << ST.toString(false);
					}
				}
			}
			
			public:
			ssize_t read(char * data, size_t len)
			{
				// std::cerr << "read(.," << len << ")" << std::endl;
			
				ssize_t totalred = 0;
				
				while ( len )
				{
					#if ! defined(__APPLE__)
					pollfd pfd = { getFD(), POLLIN, 0 };
					int const ready = poll(&pfd, 1, checkinterval);
					
					if ( ready == 1 && (pfd.revents & POLLIN) )
					{
						ssize_t red = ::read(fd,data,len);
					
						if ( red > 0 )
						{
							totalred += red;
							data += red;
							len -= red;
						}
						else if ( red < 0 && errno == EINTR )
						{
							std::cerr << "read interrupted by signal." << std::endl;
						}
						else
						{
							len = 0;
						}
					}
					else if ( ready == 1 && (pfd.revents & POLLHUP) )
					{
						len = 0;
					}
					else
					{
						std::cerr << "Waiting for fd=" << getFD() << " to become ready for reading, ready " << ready << " events " << pfd.revents; 
						if ( remaddrset )
						{
							uint32_t const rem = ntohl(remaddr.sin_addr.s_addr);
							std::cerr << " remote " 
								<< ((rem >> 24) & 0xFF) << "."
								<< ((rem >> 16) & 0xFF) << "."
								<< ((rem >>  8) & 0xFF) << "."
								<< ((rem >>  0) & 0xFF);
						}
						std::cerr << std::endl;
						::libmaus::util::StackTrace ST;
						std::cerr << ST.toString(false);
					}
					#else // __APPLE__
					ssize_t red = ::read(fd,data,len);
					
					if ( red > 0 )
					{
						totalred += red;
						data += red;
						len -= red;
					}
					else if ( red < 0 && errno == EINTR )
					{
						std::cerr << "read interrupted by signal." << std::endl;
					}
					else
					{
						len = 0;
					}
					#endif
				}
				
				return totalred;
			}

			ssize_t readPart(char * data, size_t len)
			{
				ssize_t totalred = 0;
				
				while ( (! totalred) && len )
				{
					#if ! defined(__APPLE__)
					pollfd pfd = { getFD(), POLLIN, 0 };
					int const ready = poll(&pfd, 1, checkinterval);
										
					if ( ready == 1 && (pfd.revents & POLLIN) )
					{
						ssize_t red = ::read(fd,data,len);
					
						if ( red > 0 )
						{
							totalred += red;
							data += red;
							len -= red;
						}
						else if ( red < 0 && errno == EINTR )
						{
							std::cerr << "read interrupted by signal." << std::endl;
						}
						else
						{
							len = 0;
						}
					}
					else if ( ready == 1 && (pfd.revents & POLLHUP) )
					{
						len = 0;
					}
					else
					{
						std::cerr << "Waiting for fd=" << getFD() << " to become ready for reading, ready " << ready << " events " << pfd.revents; 
						if ( remaddrset )
						{
							uint32_t const rem = ntohl(remaddr.sin_addr.s_addr);
							std::cerr << " remote " 
								<< ((rem >> 24) & 0xFF) << "."
								<< ((rem >> 16) & 0xFF) << "."
								<< ((rem >>  8) & 0xFF) << "."
								<< ((rem >>  0) & 0xFF);
						}
						std::cerr << std::endl;
						::libmaus::util::StackTrace ST;
						std::cerr << ST.toString(false);
					}
					#else // __APPLE__
					ssize_t red = ::read(fd,data,len);
					
					if ( red > 0 )
					{
						totalred += red;
						data += red;
						len -= red;
					}
					else if ( red < 0 && errno == EINTR )
					{
						std::cerr << "read interrupted by signal." << std::endl;
					}
					else
					{
						len = 0;
					}
					#endif
				}
				
				return totalred;
			}

			protected:
			static void setAddress(char const * hostname, sockaddr_in & recadr)
			{
				if ( hostname )
				{
					struct hostent * he = gethostbyname2(hostname,AF_INET);
					
					if ( ! he )
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "failed to get address for " << hostname << " via gethostbyname: " << hstrerror(h_errno);
						se.finish();
						throw se;		
					}
					
					recadr.sin_family = he->h_addrtype;
					
					if ( he->h_addr_list[0] == 0 )
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "failed to get address for " << hostname << " via gethostbyname (no address returned)";
						se.finish();
						throw se;		
					}
					else
					{
						if ( recadr.sin_family == AF_INET )
						{
							memcpy ( &recadr.sin_addr.s_addr, he->h_addr_list[0], he->h_length );
						}
						else
						{
							::libmaus::exception::LibMausException se;
							se.getStream() << "only IPv4 supported";
							se.finish();
							throw se;					
						}
						
						#if 0
						if ( recadr.sin_family == AF_INET )
						{
							std::cerr << inet_ntoa(recadr.sin_addr) << std::endl;
						}
						#endif
					}
				}
				else
				{
					recadr.sin_family = AF_INET;
					recadr.sin_addr.s_addr = INADDR_ANY;			
				}
			}

			public:
			SocketBase()
			: fd(-1), remaddrset(false)
			{
				fd = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
				
				if ( fd < 0 )
				{
					cleanup();
					::libmaus::exception::LibMausException se;
					se.getStream() << "socket() failed: " << strerror(errno);
					se.finish();
					throw se;
				}
			}
			SocketBase(int rfd, sockaddr_in const * aadr = 0)
			: fd(rfd), remaddrset(aadr != 0)
			{
				if ( aadr )
				{
					memcpy ( &remaddr, aadr, sizeof(sockaddr_in) );
				}
			}
			
			~SocketBase()
			{
				cleanup();
			}
			
			void setNoDelay()
			{
				int flag = 1;
				int result = setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,
					reinterpret_cast<char *>(&flag),sizeof(int));
				if ( result < 0 )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "setsockopt() failed: " << strerror(errno);
					se.finish();
					throw;
				}
			}
			
			void setNonBlocking()
			{
				#if 0
				int const flags = ::ioctl(fd,F_GETFL);
				
				if ( flags == -1 )
				{
					int const error = errno;
					
					::libmaus::exception::LibMausException se;
					se.getStream() << "ioctl("<<fd<<"," << F_GETFL << ",0" <<") failed: " << strerror(error) << std::endl;
					se.finish();
					throw se;
				}				
				#else
				int const flags = 0;
				#endif

				int const r = ioctl(fd,F_SETFL,flags | O_NONBLOCK);

				if ( r == -1 )
				{
					int const error = errno;
					
					::libmaus::exception::LibMausException se;
					se.getStream() << "ioctl() failed: " << strerror(error);
					se.finish();
					throw se;				
				}
			}
			
			bool hasData()
			{
				fd_set readfds;
				FD_ZERO(&readfds);
				FD_SET(fd,&readfds);
				struct timeval tv = { 0,0 };
				int selok = ::select(fd+1,&readfds,0,0,&tv);
				return selok > 0;                                              				
			}
			
			int getFD() const
			{
				return fd;
			}

			int releaseFD()
			{
				int const rfd = fd;
				fd = -1;
				return rfd;
			}
			
			template<typename data_type>
			void writeMessage(
				uint64_t const tag,
				data_type const * D,
				uint64_t const n
				)
			{
				writeNumber(tag);
				writeNumber(n);

				if ( n )
				{
					write(
						reinterpret_cast<char const *>(D),
						n * sizeof(data_type)
					);
				}
			}
			
			template<typename data_type>
			void writeSingle(
				uint64_t const tag,
				data_type const D)
			{
				writeMessage<data_type>(tag,&D,1);
			}

			template<typename data_type>
			void writeSingle(data_type const D)
			{
				writeSingle<data_type>(0,D);
			}

			template<typename data_type>
			void writeSingle()
			{
				data_type D = data_type();
				writeSingle<data_type>(D);
			}

			template<typename type_a, typename type_b>
			void writePair(
				uint64_t const tag,
				std::pair<type_a,type_b> const & D)
			{
				writeSingle<type_a>(tag,D.first);
				writeSingle<type_b>(tag,D.second);
			}

			template<typename type_a, typename type_b>
			void writePair(std::pair<type_a,type_b> const & D)
			{
				writePair<type_a,type_b>(0,D);
			}
			
			template<typename type_a, typename type_b>
			void writePairArray(std::pair<type_a,type_b> const * P, uint64_t const n)
			{
				writeSingle<uint64_t>(n);
				for ( uint64_t i = 0; i < n; ++i )
					writePair(P[i]);
			}
			
			void barrierR()
			{
				readSingle<uint64_t>();			
			}
			void barrierW()
			{
				writeSingle<uint64_t>();			
			}
			
			void barrierRw()
			{
				barrierR();
				barrierW();
			}

			void barrierWr()
			{
				barrierW();
				barrierR();
			}

			template<typename data_type, ::libmaus::autoarray::alloc_type atype >
			void writeMessageInBlocks(
				::libmaus::autoarray::AutoArray<data_type,atype> const & A,
				uint64_t const b = 8192
				)
			{
				writeMessageInBlocks(A.begin(),A.size(),b);
			}

			template<typename data_type>
			void writeMessageInBlocks(
				data_type const * D,
				uint64_t n,
				uint64_t const b
				)
			{
				writeSingle<uint64_t>(n);
				writeSingle<uint64_t>( (n + b -1 ) / b );
			
				while ( n )
				{
					uint64_t const towrite = std::min(n,b);
					writeMessage<data_type>(0,D,towrite);
					n -= towrite;
					D += towrite;
				}
			}

			template<typename stream_type, typename data_type>
			void writeMessageInBlocksFromStream(
				stream_type & stream,
				uint64_t n,
				uint64_t const b
				)
			{
				::libmaus::autoarray::AutoArray<data_type> B(b);
				// number of lements
				writeSingle<uint64_t>(n);
				// number of blocks
				writeSingle<uint64_t>( (n + b -1 ) / b );
			
				while ( n )
				{
					uint64_t const towrite = std::min(n,b);
					stream.read(reinterpret_cast<char *>(B.begin()), towrite*sizeof(data_type));
					writeMessage<data_type>(0,B.begin(),towrite);
					n -= towrite;
				}
			}

			template<
				typename data_type, 
				::libmaus::autoarray::alloc_type atype /* = ::libmaus::autoarray::alloc_type_cxx */
			>
			::libmaus::autoarray::AutoArray<data_type,atype> readMessageInBlocks()
			{
				::libmaus::timing::RealTimeClock rtc; rtc.start();
			
				uint64_t const n = readSingle<uint64_t>();
				uint64_t const blocks = readSingle<uint64_t>();
				::libmaus::autoarray::AutoArray<data_type,atype> A(n,false);
				data_type * p = A.begin();
				
				for ( uint64_t i = 0; i < blocks; ++i )
				{
					::libmaus::autoarray::AutoArray<data_type> B = readMessage<data_type>();
					std::copy(B.begin(),B.end(),p);
					p += B.size();
				}
				
				#if 0
				std::cerr << "Received array of size " << n*sizeof(data_type) << " bytes in time " <<
					rtc.getElapsedSeconds() << " rate " << 
					((n*sizeof(data_type))/rtc.getElapsedSeconds()) / (1024.0*1024.0) << " MB/s." << std::endl;
				#endif
				
				return A;
			}

			template<typename data_type>
			void writeMessageBlocked(
				uint64_t const tag,
				data_type const * D,
				uint64_t n,
				uint64_t const b
				)
			{
				while ( n )
				{
					uint64_t const towrite = std::min(n,b);
					writeMessage<data_type>(tag,D,towrite);
					n -= towrite;
					D += towrite;
				}
			}
			
			void writeString(uint64_t const tag, std::string const & s)
			{
				char const * c = s.c_str();
				uint64_t const n = s.size();
				writeSingle<uint64_t>(tag);
				writeMessageInBlocks<char> ( c, n, 8*1024 );
			}

			void writeString(std::string const & s)
			{
				writeString(0,s);
			}
			
			void writeStringVector(uint64_t const tag, std::vector<std::string> const & V)
			{
				uint64_t const n = V.size();

				// length of vector
				writeMessage<uint64_t>(tag,&n,1);
				for ( uint64_t i = 0; i < n; ++i )
					writeString(tag,V[i]);
			}

			void writeStringVector(std::vector<std::string> const & V)
			{
				writeStringVector(0,V);
			}
			
			std::string readString(uint64_t & tag)
			{
				tag = readSingle<uint64_t>();
				::libmaus::autoarray::AutoArray<char> A = readMessageInBlocks<char,::libmaus::autoarray::alloc_type_cxx>();
				return std::string(A.get(),A.get()+A.size());
			}
			std::string readString()
			{
				uint64_t tag;
				return readString(tag);
			}
			
			std::vector<std::string> readStringVector(uint64_t & tag)
			{
				uint64_t n,nn;
				readMessage<uint64_t>(tag,&n,nn);
				
				std::vector<std::string> V;
				for ( uint64_t i = 0; i < n; ++i )
					V.push_back( readString(tag) );
				
				return V;
			}
			
			std::vector<std::string> readStringVector()
			{
				uint64_t tag;
				return readStringVector(tag);
			}
			
			template<typename data_type>
			void readSingle(uint64_t & tag, data_type & D)
			{			
				tag = readNumber();
				uint64_t const n = readNumber();
				
				if ( n != 1 )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Number of elements expected != 1 in " <<
						::libmaus::util::Demangle::demangle<this_type>() << "::readSingle()";
					se.finish();
					throw se;
				}

				uint64_t const toread = n * sizeof(data_type);
				ssize_t const red = read ( reinterpret_cast<char *>(&D) , toread );
				
				if ( red != static_cast<ssize_t>(toread) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Failed to readSingle: " << strerror(errno);
					se.finish();
					throw se;
				}
			}

			template<typename data_type>
			data_type readSingle(uint64_t & tag)
			{	
				data_type D;
				readSingle(tag,D);		
				return D;
			}

			template<typename data_type>
			data_type readSingle()
			{		
				uint64_t tag;
				data_type D;
				readSingle<data_type>(tag,D);
				return D;
			}

			template<typename type_a, typename type_b>
			std::pair<type_a,type_b> readPair(uint64_t & tag)
			{	
				type_a a = readSingle<type_a>(tag);
				type_b b = readSingle<type_b>(tag);
				return std::pair<type_a,type_b>(a,b);
			}

			template<typename type_a, typename type_b>
			std::pair<type_a,type_b> readPair()
			{
				uint64_t tag;
				return readPair<type_a,type_b>(tag);
			}

			template<typename type_a, typename type_b>
			::libmaus::autoarray::AutoArray < std::pair<type_a,type_b> > readPairVector()
			{
				uint64_t const n = readSingle<uint64_t>();
				::libmaus::autoarray::AutoArray < std::pair<type_a,type_b> > A(n,false);
				for ( uint64_t i = 0; i < n; ++i )
					A[i] = readPair<type_a,type_b>();
				return A;
			}
			
			template<typename data_type>
			void readMessage(
				uint64_t & tag,
				data_type * D,
				uint64_t & n)
			{
				tag = readNumber();
				n = readNumber();

				#if 0
				std::cerr << "tag=" << tag << " n=" << n << std::endl;
				#endif
				
				uint64_t const toread = n * sizeof(data_type);
				ssize_t const red = read ( reinterpret_cast<char *>(D) , toread );
				
				if ( red != static_cast<ssize_t>(toread) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Failed to SocketBase::readMessage: " << strerror(errno);
					se.finish();
					throw se;
				}
			}

			template<typename data_type>
			::libmaus::autoarray::AutoArray<data_type> readMessage(uint64_t & tag)
			{
				tag = readNumber();
				uint64_t const n = readNumber();
				
				#if defined(READMESSAGEDEBUG)
				std::cerr << "tag=" << tag << " n=" << n << std::endl;
				::libmaus::timing::RealTimeClock rtc; rtc.start();
				#endif
				
				::libmaus::autoarray::AutoArray<data_type> D(n,false);
				
				uint64_t const toread = n * sizeof(data_type);
				ssize_t const red = read ( reinterpret_cast<char *>(D.get()) , toread );

				if ( red != static_cast<ssize_t>(toread) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Failed to SocketBase::readMessage: " << strerror(errno);
					se.finish();
					throw se;
				}

				#if defined(READMESSAGEDEBUG)
				std::cerr << "Got " << toread << " bytes in time " << rtc.getElapsedSeconds()
					<< " rate " << (toread / rtc.getElapsedSeconds())/(1024.0*1024.0) << " MB/s." << std::endl;
				#endif
				
				return D;
			}

			template<typename data_type>
			::libmaus::autoarray::AutoArray<data_type> readMessage()
			{
				uint64_t tag;
				return readMessage<data_type>(tag);
			}

			void writeNumber(uint64_t const n)
			{
				uint8_t A[8] =
				{
					static_cast<uint8_t>((n >> (7*8)) & 0xFF),
					static_cast<uint8_t>((n >> (6*8)) & 0xFF),
					static_cast<uint8_t>((n >> (5*8)) & 0xFF),
					static_cast<uint8_t>((n >> (4*8)) & 0xFF),
					static_cast<uint8_t>((n >> (3*8)) & 0xFF),
					static_cast<uint8_t>((n >> (2*8)) & 0xFF),
					static_cast<uint8_t>((n >> (1*8)) & 0xFF),
					static_cast<uint8_t>((n >> (0*8)) & 0xFF)
				};
				write ( reinterpret_cast<char const *>(&A[0]), 8);
			}

			uint64_t readNumber()
			{
				uint8_t A[8];
				uint64_t const red = read(reinterpret_cast<char *>(&A[0]),sizeof(A)/sizeof(A[0]));
				
				if ( red != sizeof(A)/sizeof(A[0]) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Failed to SocketBase::readNumber(): received " << red << " out of " << sizeof(A)/sizeof(A[0]) << " octets.";
					se.finish();
					throw se;
				}
				
				uint64_t v = 0;
				
				for ( uint64_t i = 0; i < 8; ++i )
				{
					v <<= 8;
					v |= A[i];
				}
				
				return v;
			}

		};

		struct ClientSocket : public SocketBase
		{
			typedef ClientSocket this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
		
			// sockaddr_in recadr;                     

			ClientSocket(unsigned short port, char const * hostname)
			: SocketBase()
			{
				setAddress(hostname, remaddr);		
				remaddr.sin_port = htons(port);
				remaddrset = true;
				
				if ( connect(getFD(),reinterpret_cast<const sockaddr *>(&remaddr),sizeof(remaddr)) != 0 )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "connect() failed: " << strerror(errno);
					se.finish();
					throw se;		
				}
			}
			
			static SocketBase::unique_ptr_type baseCast(ClientSocket::unique_ptr_type & C)
			{
				int const fd = C->getFD();
				C->releaseFD();
				
				try
				{
					SocketBase::unique_ptr_type ptr ( new SocketBase(fd) );
					return UNIQUE_PTR_MOVE(ptr);
				}
				catch(...)
				{
					close(fd);
					throw;
				}
			}
			
			static SocketBase::unique_ptr_type baseAlloc(unsigned short const port, char const * const hostname)
			{
				ClientSocket::unique_ptr_type ptr ( new ClientSocket(port,hostname) );
				SocketBase::unique_ptr_type baseptr = baseCast(ptr);
				return UNIQUE_PTR_MOVE(baseptr);
			}
			static SocketBase::unique_ptr_type baseAlloc(unsigned short const port, std::string const & hostname)
			{

				SocketBase::unique_ptr_type baseptr = baseAlloc(port,hostname.c_str());
				return UNIQUE_PTR_MOVE(baseptr);
			}
		};

		struct ServerSocket : public SocketBase
		{
			typedef ServerSocket this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
		
			sockaddr_in recadr;
			
			unsigned short getPort() const
			{
				return ntohs(recadr.sin_port);
			}
			
			static unique_ptr_type allocateServerSocket(
				unsigned short & port, 
				unsigned int const backlog, 
				std::string const & hostname,
				unsigned int tries)
			{
				unique_ptr_type ptr(allocateServerSocket(port,backlog,hostname.c_str(),tries));
				return UNIQUE_PTR_MOVE(ptr);
			}

			static unique_ptr_type allocateServerSocket(
				unsigned short & port, 
				unsigned int const backlog, 
				char const * hostname,
				unsigned int tries)
			{
				for ( unsigned int i = 0; i < tries; ++i )
				{
					try
					{
						return unique_ptr_type ( new this_type(port,backlog,hostname) );
					}
					catch(std::exception const & ex)
					{
						// std::cerr << ex.what() << std::endl;
						port++;
					}
				}
				
				::libmaus::exception::LibMausException ex;
				ex.getStream() << "Failed to allocate ServerSocket (no ports available)";
				ex.finish();
				throw ex;
			}
			
			ServerSocket(unsigned short rport, unsigned int backlog, char const * hostname)
			: SocketBase()
			{
				memset(&recadr,0,sizeof(recadr));

				setAddress(hostname, recadr);		
				recadr.sin_port = htons(rport);
				
				if ( bind ( getFD(), reinterpret_cast<struct sockaddr *>(&recadr), sizeof(recadr) ) != 0 )
				{
					if ( errno == EADDRINUSE )
						throw std::runtime_error("bind: address is already in use.");
				
					// cleanup();
					::libmaus::exception::LibMausException se;
					se.getStream() << "bind() failed: " << strerror(errno);
					se.finish();
					throw se;		
				}
				
				if ( listen ( getFD(), backlog ) != 0 )
				{
					// cleanup();
					::libmaus::exception::LibMausException se;
					se.getStream() << "listen() failed: " << strerror(errno);
					se.finish();
					throw se;
				}
			}
			
			~ServerSocket()
			{
			}
			
			SocketBase::unique_ptr_type accept()
			{
				sockaddr_in aadr;
				socklen_t len = sizeof(aadr);
			
				int afd = ::accept(getFD(),reinterpret_cast<struct sockaddr *>(&aadr),&len);
				
				if ( afd < 0 )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "accept() failed: " << strerror(errno);
					se.finish();
					throw se;		
				}
				else
				{
					return SocketBase::unique_ptr_type(new SocketBase(afd,&aadr));
				}
			}

			SocketBase::shared_ptr_type acceptShared()
			{
				SocketBase::unique_ptr_type uptr(accept());
				SocketBase * ptr = uptr.release();
				SocketBase::shared_ptr_type sptr(ptr);
				return sptr;
			}
			
			bool waitForConnection(unsigned int const t)
			{
				fd_set fds;
				FD_ZERO(&fds);
				FD_SET(getFD(),&fds);
				struct timeval timeout = { 
					static_cast<long>(t), 
					static_cast<long>(0)
				};
			
				int const r = ::select(getFD()+1,&fds,0,0,&timeout);
				
				if ( r < 0 )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "select failed on socket." << std::endl;
					se.finish();
					throw se;
				}
				else if ( r == 0 )
				{
					return false;
				}
				else
				{
					return true;
				}
			}

			SocketBase::unique_ptr_type accept(std::string const & sid)
			{
				SocketBase::unique_ptr_type ptr;
				
				while ( ! ptr.get() )
				{
					try
					{
						SocketBase::unique_ptr_type cand = accept();
						std::string const remsid = cand->readString();
						if ( remsid == sid )
							ptr = UNIQUE_PTR_MOVE(cand);
					}
					catch(std::exception const & ex)
					{
						std::cerr << ex.what() << std::endl;
					}
				}
				
				return UNIQUE_PTR_MOVE(ptr);
			}
		};

		template<typename _data_type>
		struct SocketOutputBuffer
		{
			typedef _data_type data_type;
			typedef SocketOutputBuffer<data_type> this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			
			::libmaus::network::SocketBase * const socket;
			::libmaus::autoarray::AutoArray< data_type > B;
			data_type * const pa;
			data_type * pc;
			data_type * const pe;
			::libmaus::timing::RealTimeClock flushclock;
			double flushtime;
			uint64_t datatag;
			
			SocketOutputBuffer(
				::libmaus::network::SocketBase * const rsocket, 
				uint64_t const bufsize,
				uint64_t const rdatatag = 0
			)
			: socket(rsocket), B(bufsize), pa(B.begin()), pc(pa), pe(B.end()), datatag(rdatatag)
			{
			
			}
			
			void flush()
			{
				if ( pc != pa )
				{
					uint64_t const n = pc-pa;
					flushclock.start();
					socket->writeSingle<uint64_t>(datatag,n);	
					socket->writeMessage<data_type>(datatag,pa,n);
					flushtime = flushclock.getElapsedSeconds();
				}
				pc = pa;
			}
			
			bool put(data_type const & v)
			{
				*(pc++) = v;
				if ( pc == pe )
				{
					flush();
					return true;
				}
				else
				{
					return false;
				}
			}
		};

		template<typename _data_type>
		struct SocketOutputBufferIterator : public ::std::iterator< ::std::output_iterator_tag, void, void, void, void >
		{
			typedef _data_type data_type;
			typedef SocketOutputBufferIterator<data_type> this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			
			SocketOutputBuffer<data_type> * SOB;
			
			SocketOutputBufferIterator(SocketOutputBuffer<data_type> * const rSOB)
			: SOB(rSOB)
			{}
			
			this_type & operator*() { return *this; }
			this_type & operator++(int) { return *this; }
			this_type & operator++() { return *this; }
			this_type & operator=(data_type const & v) { SOB->put(v); return *this; }
		};

		template<typename _data_type>
		struct SocketInputBuffer
		{
			typedef _data_type data_type;
			typedef SocketInputBuffer<data_type> this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			
			::libmaus::network::SocketBase * const socket;
			uint64_t const limit;
			uint64_t proc;
			::libmaus::autoarray::AutoArray< data_type > B;
			data_type * pa;
			data_type * pc;
			data_type * pe;
			uint64_t const termtag;
			bool const requestblock;
			bool termreceived;
			
			static uint64_t getDefaultLimit()
			{
				return std::numeric_limits<uint64_t>::max();
			}

			static uint64_t getDefaultTermTag()
			{
				return std::numeric_limits<uint64_t>::max();
			}
			
			static bool getDefaultRequestBlock()
			{
				return false;
			}
			
			SocketInputBuffer(
				::libmaus::network::SocketBase * const rsocket,
				uint64_t const rlimit = getDefaultLimit(), // std::numeric_limits<uint64_t>::max(),
				uint64_t const rtermtag = getDefaultTermTag(), // std::numeric_limits<uint64_t>::max(),
				bool const rrequestblock = getDefaultRequestBlock()
			)
			: socket(rsocket), limit(rlimit), proc(0), B(), pa(0), pc(0), pe(0), termtag(rtermtag),
			  requestblock(rrequestblock), termreceived(false)
			{
			
			}
			
			void fill()
			{
				try
				{
					if ( requestblock )
					{
						socket->writeSingle<uint64_t>(0);
					}
					
					uint64_t tag, n, nn;
					socket->readMessage < uint64_t > (tag,&n,nn);
					
					if ( tag == termtag )
					{
						proc = limit;
						termreceived = true;
						// std::cerr << "Received term." << std::endl;
					}
					else
					{
						B = ::libmaus::autoarray::AutoArray< data_type >(n,false);
						pa = B.begin();
						pc = pa;
						socket->readMessage < data_type > (tag,pa,n);

						uint64_t const toadd = std::min(n,limit-proc);
						pe = pa+toadd;
						proc += toadd;
						
						// std::cerr << "Received " << toadd << " elements of data." << std::endl;
					}
				}
				catch(::std::exception const & ex)
				{
					std::cerr << ex.what() << std::endl;
				}
			}
			
			bool get(data_type & v)
			{
				if ( pc == pe )
				{
					if ( proc == limit )
						return false;
					fill();
				}
				if ( pc != pe )
				{
					v = *(pc++);
					return true;
				}
				else
				{
					return false;
				}
			}
			
			int get()
			{
				data_type v;
				bool const ok = get(v);
				if ( ! ok )
					return -1;
				else
					return v;
			}
		};

		template<typename _data_type>
		struct AsynchronousSocketInputBuffer : public ::libmaus::parallel::PosixThread
		{
			typedef _data_type data_type;
			typedef AsynchronousSocketInputBuffer<data_type> this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			
			::libmaus::network::SocketBase * const socket;
 			::libmaus::autoarray::AutoArray< data_type > B;
			data_type * pa;
			data_type * pc;
			data_type * pe;
			uint64_t const termtag;

 			::libmaus::autoarray::AutoArray< data_type > D;
			::libmaus::parallel::SynchronousQueue<uint64_t> fullqueue;
			::libmaus::parallel::SynchronousQueue<uint64_t> emptyqueue;
			
			static uint64_t getDefaultTermTag()
			{
				return std::numeric_limits<uint64_t>::max();
			}
			
			AsynchronousSocketInputBuffer(
				::libmaus::network::SocketBase * const rsocket,
				uint64_t const rtermtag = getDefaultTermTag()
			)
			: socket(rsocket),
			  B(), pa(0), pc(0), pe(0), 
			  termtag(rtermtag)
			{
				emptyqueue.enque(0);
				start();
			}
			
			virtual void * run()
			{
				try
				{
					bool termreceived = false;
					
					while ( ! termreceived )
					{
						emptyqueue.deque();
						
						socket->writeSingle<uint64_t>(0);
						
						uint64_t tag, n, nn;
						socket->readMessage < uint64_t > (tag,&n,nn);
						
						if ( tag == termtag )
						{
							termreceived = true;
							fullqueue.enque(termtag);
						}
						else
						{
							D = ::libmaus::autoarray::AutoArray< data_type >(n,false);
							socket->readMessage < data_type > (tag,D.get(),n);
							fullqueue.enque(0);
						}
					}
				}
				catch(std::exception const & ex)
				{
					std::cerr << "AsynchronousSocketInputBuffer failed:" << std::endl;
					std::cerr << ex.what() << std::endl;
					fullqueue.enque(termtag);
				}
				return 0;
			}
			
			bool fill()
			{
				try
				{
					uint64_t const tag = fullqueue.deque();
					
					if ( tag == termtag )
					{
						return false;
					}
					else
					{
						B = D;
						pa = B.begin();
						pc = B.begin();
						pe = B.end();
						emptyqueue.enque(0);
						return true;
					}					
				}
				catch(::std::exception const & ex)
				{
					std::cerr << ex.what() << std::endl;
					return false;
				}
			}
			
			bool get(data_type & v)
			{
				if ( pc == pe )
				{
					fill();
				}
				if ( pc != pe )
				{
					v = *(pc++);
					return true;
				}
				else
				{
					return false;
				}
			}
			
			int get()
			{
				data_type v;
				bool const ok = get(v);
				if ( ! ok )
					return -1;
				else
					return v;
			}
		};
	}
}
#endif
