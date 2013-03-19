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
#if ! defined(LIBMAUS_NETWORK_FILEDESCRIPTORPASSER_HPP)
#define LIBMAUS_NETWORK_FILEDESCRIPTORPASSER_HPP

#include <libmaus/parallel/PosixProcess.hpp>
#include <libmaus/util/unique_ptr.hpp>
#include <libmaus/util/shared_ptr.hpp>
#include <libmaus/network/SocketPair.hpp>
#include <libmaus/network/Socket.hpp>

namespace libmaus
{
	namespace network
	{
		struct FileDescriptorPasser : public ::libmaus::parallel::PosixProcess
		{
			typedef FileDescriptorPasser this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			::libmaus::network::SocketPair * SP;
			uint64_t cid;
			::libmaus::network::SocketBase * passfd;
			std::string hostname;
			std::string semname;
			
			FileDescriptorPasser(
				::libmaus::network::SocketPair * rSP,
				uint64_t const rcid,
				::libmaus::network::SocketBase * rpassfd,
				std::string const & rhostname,
				std::string const & rsemname
			) : SP(rSP), cid(rcid), passfd(rpassfd), hostname(rhostname), semname(rsemname)
			{
				// std::cerr << "File descriptor passer for cid=" << cid << std::endl;
				start();
			}
			
			int run()
			{
				try
				{
					// std::cerr << "Semaphore " << semname << std::endl;
					::libmaus::parallel::NamedPosixSemaphore NPS(semname,false);
					NPS.wait();
					// std::cerr << "Got it for id " << cid << std::endl;
				
					// send process id
					::libmaus::network::SocketBase socket(SP->parentGet());
					socket.writeSingle<uint64_t>(cid);
					socket.writeString(hostname);
					socket.releaseFD();
				
					// send file descriptor
					SP->sendFd(passfd->getFD());
				}
				catch(std::exception const & ex)
				{
					std::cerr << "[DFDP1] FileDescriptorPasser::run() caught exception: " << ex.what() << std::endl;
				}
				return 0;
			}
		};
	}
}
#endif
