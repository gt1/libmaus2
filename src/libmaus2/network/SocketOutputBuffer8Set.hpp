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

#if ! defined(SOCKETOUTPUTBUFFER8SET_HPP)
#define SOCKETOUTPUTBUFFER8SET_HPP

#include <libmaus2/network/SocketOutputBuffer8.hpp>
#include <libmaus2/util/IntervalTree.hpp>
#include <libmaus2/util/GenericIntervalTree.hpp>

namespace libmaus2
{
	namespace network
	{
		struct SocketOutputBuffer8Set
		{
			typedef SocketOutputBuffer8Set this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			typedef ::libmaus2::network::SocketOutputBuffer8 buffer_type;
			typedef buffer_type::unique_ptr_type buffer_ptr_type;

			typedef ::libmaus2::network::ClientSocket client_socket_type;
			typedef client_socket_type::unique_ptr_type client_socket_ptr_type;

			::libmaus2::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > const & HI;
			::libmaus2::autoarray::AutoArray < buffer_ptr_type > buffers;
			::libmaus2::util::GenericIntervalTree::unique_ptr_type IT;

			SocketOutputBuffer8Set(
				::libmaus2::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > const & rHI,
				int const tag, uint64_t const bufsize,
				::libmaus2::autoarray::AutoArray<client_socket_ptr_type> & hashsocks
			)
			: HI(rHI), buffers(HI.size()), IT(new ::libmaus2::util::GenericIntervalTree(HI /* ,0,HI.size() */))
			{
				for ( uint64_t i = 0; i < HI.size(); ++i )
					buffers[i] = UNIQUE_PTR_MOVE(buffer_ptr_type(new buffer_type(hashsocks[i].get(),tag,bufsize)));
			}
			void flush()
			{
				for ( uint64_t i = 0; i < buffers.size(); ++i )
					buffers[i]->flush();
			}

			buffer_type & getBuffer(uint64_t const hashval)
			{
				uint64_t const i = IT->find(hashval);
				assert ( i < HI.size() );
				assert ( hashval >= HI[i].first );
				assert ( hashval < HI[i].second );
				return *(buffers[i]);
			}
		};
	}
}
#endif
