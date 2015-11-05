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

#if ! defined(FASTCLIENT_HPP)
#define FASTCLIENT_HPP

#include <libmaus2/network/Socket.hpp>
#include <libmaus2/fastx/SocketFastAReader.hpp>
#include <libmaus2/fastx/SocketFastQReader.hpp>
#include <libmaus2/fastx/CompactFastDecoder.hpp>
#include <libmaus2/network/SocketFastReaderBase.hpp>

namespace libmaus2
{
        namespace fastx
        {
                template<typename _socket_base>
                struct FastClient: public ::libmaus2::network::ClientSocket, public _socket_base
                {
                        typedef _socket_base socket_base;
                        typedef FastClient<socket_base> this_type;
                        typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
                        typedef typename socket_base::pattern_type pattern_type;

                        FastClient(
                                unsigned short const fqserverport,
                                std::string const shostname,
                                std::vector< ::libmaus2::fastx::FastInterval > const & index,
                                uint64_t const i
                                )
                        : ClientSocket(fqserverport,shostname.c_str()), _socket_base(this,index,i) {}
                };

                typedef FastClient< ::libmaus2::fastx::SocketFastAReader > FastAClient;
                typedef FastClient< ::libmaus2::fastx::SocketFastQReader > FastQClient;
                typedef FastClient< ::libmaus2::fastx::CompactFastSocketDecoder > FastCClient;
        }
}
#endif
