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
#if ! defined(LIBMAUS2_NETWORK_LOGRECEIVER_HPP)
#define LIBMAUS2_NETWORK_LOGRECEIVER_HPP

#include <libmaus2/network/FileDescriptorPasser.hpp>
#include <libmaus2/network/LogReceiverTestProcess.hpp>
#include <libmaus2/parallel/PosixSemaphore.hpp>

namespace libmaus2
{
	namespace network
	{
		struct LogReceiver : public ::libmaus2::parallel::PosixProcess
		{
			typedef LogReceiver this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			static std::string computeSessionId();

			struct ControlDescriptor
			{
				::libmaus2::network::SocketBase::shared_ptr_type controlsock;
				uint64_t id;
				std::string hostname;

				ControlDescriptor() : id(0) {}
				ControlDescriptor(
					::libmaus2::network::SocketBase::shared_ptr_type & rcontrolsock,
					uint64_t const rid,
					std::string const & rhostname)
				: controlsock(rcontrolsock), id(rid), hostname(rhostname) {}
			};

			std::string const sid;
			std::string const hostname;
			std::string const logfileprefix;
			unsigned short port;
			::libmaus2::network::ServerSocket::unique_ptr_type logsocket;
			::libmaus2::network::SocketPair::unique_ptr_type SP;
			::libmaus2::network::SocketBase::unique_ptr_type pcsocket;
			::libmaus2::network::SocketPair::unique_ptr_type SPpass;
			::libmaus2::parallel::NamedPosixSemaphore passsem;

			::libmaus2::network::LogReceiverTestProcess::unique_ptr_type constructLogReceiverTestProcess(
				uint64_t const id,
				::libmaus2::network::DispatchCallback * dc = 0
			);
			std::vector<int> getOpenFds() const;

			LogReceiver(
				std::string const & rlogfileprefix,
				unsigned int rport,
				unsigned int const rbacklog,
				unsigned int const tries = 64*1024
			);
			~LogReceiver();

			std::string getLogFileNamePrefix(uint64_t const clientid) const;
			bool controlDescriptorPending();
			ControlDescriptor getControlDescriptor();

			int run();
		};
	}
}
#endif
