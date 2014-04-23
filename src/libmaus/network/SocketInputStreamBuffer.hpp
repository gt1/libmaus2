/*
    libmaus
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if !defined(LIBMAUS_AIO_SOCKETINPUTSTREAMBUFFER_HPP)
#define LIBMAUS_AIO_SOCKETINPUTSTREAMBUFFER_HPP

#include <streambuf>
#include <istream>
#include <libmaus/network/Socket.hpp>

namespace libmaus
{
	namespace network
	{
		struct SocketInputStreamBuffer : public ::std::streambuf
		{
			private:
			::libmaus::network::SocketInputInterface & stream;

			uint64_t const blocksize;
			uint64_t const putbackspace;
			
			::libmaus::autoarray::AutoArray<char> buffer;

			SocketInputStreamBuffer(SocketInputStreamBuffer const &);
			SocketInputStreamBuffer & operator=(SocketInputStreamBuffer &);
						
			public:
			SocketInputStreamBuffer(::libmaus::network::SocketInputInterface & rstream, uint64_t const rblocksize, uint64_t const rputbackspace = 0)
			: 
			  stream(rstream),
			  blocksize(rblocksize),
			  putbackspace(rputbackspace),
			  buffer(putbackspace + blocksize,false)
			{
				// set empty buffer
				setg(buffer.end(), buffer.end(), buffer.end());
			}

			private:
			int_type underflow()
			{
				// if there is still data, then return it
				if ( gptr() < egptr() )
					return static_cast<int_type>(*(reinterpret_cast<uint8_t const *>(gptr())));

				assert ( gptr() == egptr() );

				// number of bytes for putback buffer
				uint64_t const putbackcopy = std::min(
					static_cast<uint64_t>(gptr() - eback()),
					putbackspace
				);
				
				// copy bytes
				#if 0
				std::copy(
					gptr()-putbackcopy,
					gptr(),
					buffer.begin() + putbackspace - putbackcopy
				);
				#endif
				std::memmove(
					buffer.begin() + putbackspace - putbackcopy,
					gptr()-putbackcopy,
					putbackcopy
				);
				
				// load data
				uint64_t const uncompressedsize = stream.readPart(
						buffer.begin()+putbackspace,
						buffer.size()-putbackspace
					);

				// set buffer pointers
				setg(
					buffer.begin()+putbackspace-putbackcopy,
					buffer.begin()+putbackspace,
					buffer.begin()+putbackspace+uncompressedsize
				);

				if ( uncompressedsize )
					return static_cast<int_type>(*(reinterpret_cast<uint8_t const *>(gptr())));
				else
					return traits_type::eof();
			}
		};
	}
}
#endif
