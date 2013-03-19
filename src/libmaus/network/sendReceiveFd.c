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
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

extern int libmaus_network_sendFd_C(int const socket, int const fd);
extern int libmaus_network_receiveFd_C(int const socket);

int libmaus_network_sendFd_C(int const socket, int const fd)
{
	char message[1] = { '\0' };
	struct iovec io_vec[1] = { [0].iov_len  = sizeof(message), [0].iov_base = &message[0] };
	char ancillary[CMSG_SPACE(sizeof(int))];
	struct msghdr hdr;
	struct cmsghdr *chdr;
	unsigned char * cdata;
	size_t i;

	/* erase structures */
	memset(&hdr, 0, sizeof(struct msghdr));
	memset(ancillary, 0, sizeof(ancillary));

	/* see man page for sendmsg */
	hdr.msg_iov = io_vec;
	hdr.msg_iovlen = 1;
	hdr.msg_control = ancillary;
	hdr.msg_controllen = sizeof(ancillary);
	chdr             = CMSG_FIRSTHDR(&hdr);
	chdr->cmsg_len   = CMSG_LEN(sizeof(int));
	chdr->cmsg_level = SOL_SOCKET;
	chdr->cmsg_type  = SCM_RIGHTS;
	cdata = CMSG_DATA(chdr);
	*((int *)cdata) = fd;
	#if 0
	for ( i = 0; i < sizeof(int); ++i )
		cdata[i] = (fd >> (8*i)) & 0xFF;
	#endif

	return sendmsg(socket, &hdr, 0);
}

#include <stdio.h>

int libmaus_network_receiveFd_C(int const socket)
{
	char message[1];
	struct iovec io_vec[1] = { [0].iov_len  = sizeof(message), [0].iov_base = &message[0] };
	struct msghdr hdr;
	struct cmsghdr *chdr;
	char ancillary[CMSG_SPACE(sizeof(int))];
	#if defined(__APPLE__) || defined(__FreeBSD__)
	int const recflags = 0;
	#else
	int const recflags = MSG_CMSG_CLOEXEC;
	#endif

	/* initialize structures */
	memset(&hdr, 0, sizeof(struct msghdr));
	memset(&ancillary[0], 0, sizeof(ancillary));
	hdr.msg_iov = io_vec;
	hdr.msg_iovlen = 1;
	hdr.msg_control = ancillary;
	hdr.msg_controllen = sizeof(ancillary);

	/* receive message and check for completeness */
	if ( recvmsg(socket, &hdr, recflags) < 0 || (hdr.msg_flags & MSG_CTRUNC) )
		return -1;
	/* extract file descriptor */
	for(chdr = CMSG_FIRSTHDR(&hdr); chdr; chdr = CMSG_NXTHDR(&hdr, chdr))
		/* check type of data (should not be necessary...) */
		if( (chdr->cmsg_level == SOL_SOCKET) && (chdr->cmsg_type == SCM_RIGHTS) )
		{
			size_t i;
			int fd;
			unsigned char const * data = CMSG_DATA(chdr);
			
			#if 0
			for ( i = 0; i < sizeof(int); ++i )
				fd |= ((int)data[i]) << (8*i);
			#endif
			fd = *((int *)(data));
				
			// fprintf(stderr,"Got fd %d\n", fd);
			
			return fd;
		}

	return -1;
}
