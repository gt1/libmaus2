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

#if ! defined(SOCKETOUTPUTBUFFER8_HPP)
#define SOCKETOUTPUTBUFFER8_HPP

#include <libmaus2/network/Socket.hpp>

namespace libmaus2
{
	namespace network
	{
		template<typename _data_type>
		struct SocketOutputBufferTemplate
		{
			typedef _data_type data_type;
			typedef SocketOutputBufferTemplate<data_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr < this_type > :: type unique_ptr_type;

			::libmaus2::network::SocketBase * dst;
			int const tag;
			::libmaus2::autoarray::AutoArray<data_type> B;
			data_type * const pa;
			data_type * pc;
			data_type * const pe;

			SocketOutputBufferTemplate(
				::libmaus2::network::SocketBase * rdst, 
				int const rtag,
				uint64_t const bufsize)
			: dst(rdst), tag(rtag), B(bufsize), pa(B.get()), pc(pa), pe(pa+B.getN())
			{
			}
			~SocketOutputBufferTemplate()
			{
				flush();
			}

			void flush()
			{
				writeBuffer();
			}

			void writeBuffer()
			{
				if ( pc-pa )
					dst->writeMessage ( tag , B.get() , pc-pa );
				pc = pa;
			}

			void put(data_type const c)
			{
				*(pc++) = c;
				if ( pc == pe )
					writeBuffer();
			}
		};
		
		typedef SocketOutputBufferTemplate<uint64_t> SocketOutputBuffer8;
		typedef SocketOutputBufferTemplate<uint32_t> SocketOutputBuffer4;
		typedef SocketOutputBufferTemplate<uint16_t> SocketOutputBuffer2;
		typedef SocketOutputBufferTemplate<uint8_t> SocketOutputBuffer1;
	}
}
#endif
