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
#if ! defined(LIBMAUS_NETWORK_SOCKETPAIR_HPP)
#define LIBMAUS_NETWORK_SOCKETPAIR_HPP

#include <libmaus2/exception/LibMausException.hpp>
#include <libmaus2/network/sendReceiveFd.h>

#include <cerrno>
#include <cstring>

#include <sys/types.h>
#include <sys/socket.h>
#include <cassert>
              
namespace libmaus2
{
	namespace network
	{
		struct SocketPair
		{
			typedef SocketPair this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
		
			int fd[2];
			
			SocketPair()
			{
				fd[0] = fd[1] = -1;
				
				// if ( socketpair(AF_LOCAL,SOCK_STREAM,PF_LOCAL,&fd[0]) < 0 )
				if ( socketpair(AF_LOCAL,SOCK_STREAM,0,&fd[0]) < 0 )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "socketpair() failed: " << strerror(errno) << std::endl;
					se.finish();
					throw se;
				}
			}
			~SocketPair()
			{
				closeParent();
				closeChild();
			}
			
			int parentRelease()
			{
				int const rfd = fd[1];
				fd[1] = -1;
				return rfd;
			}

			int childRelease()
			{
				int const rfd = fd[0];
				fd[0] = -1;
				return rfd;
			}
			
			int parentGet()
			{
				return fd[1];
			}
			
			int childGet()
			{
				return fd[0];
			}
			
			void closeParent()
			{
				if ( fd[0] != -1 )
					::close(fd[0]);
				#if 0
				else
					std::cerr << "WARNING: SocketPair::closeParent() called for file descriptor already closed." << std::endl;
				#endif
				fd[0] = -1;
			}
			
			void closeChild()
			{
				if ( fd[1] != -1 )
					::close(fd[1]);
				#if 0
				else
					std::cerr << "WARNING: SocketPair::closeChild() called for file descriptor already closed." << std::endl;
				#endif
				fd[1] = -1;
			}
			
			// send file descriptor parent -> child
			void sendFd(int const rfd)
			{
				libmaus2_network_sendFd_C(parentGet(),rfd);
			}
			
			// receive file descriptor from parent
			int receiveFd()
			{
				return libmaus2_network_receiveFd_C(childGet());
			}
			
			bool pending()
			{
				int const fd = childGet();
				assert ( fd != -1 );
				fd_set fds;
				FD_ZERO(&fds);
				FD_SET(fd,&fds);
				timeval to = { 0, 0 };
				int const ret = ::select(fd+1,&fds,0,0,&to);
				return ret > 0;
			}
		};
	}
}
#endif
