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
#if ! defined(LIBMAUS_NETWORK_URLINPUTSTREAMBUFFERWRAPPER_HPP)
#define LIBMAUS_NETWORK_URLINPUTSTREAMBUFFERWRAPPER_HPP

#include <libmaus/network/HttpSocketInputStreamBuffer.hpp>
#include <libmaus/network/FtpSocketInputStreamBuffer.hpp>
#include <libmaus/network/FileUrl.hpp>
#include <libmaus/aio/PosixFdInputStreamBuffer.hpp>

namespace libmaus
{
	namespace network
	{
		struct UrlInputStreamBufferWrapper : public UrlBase
		{
			private:
			libmaus::network::HttpSocketInputStreamBuffer::unique_ptr_type httpstreambuffer;
			libmaus::network::FtpSocketInputStreamBuffer::unique_ptr_type ftpstreambuffer;
			libmaus::aio::PosixFdInput::unique_ptr_type fileinput;
			libmaus::aio::PosixFdInputStreamBuffer::unique_ptr_type filestreambuffer;
			
			public:
			UrlInputStreamBufferWrapper(std::string const & url, uint64_t const bufsize, uint64_t const pushbacksize = 0)
			{
				if ( isAbsoluteUrl(url) )
				{
					std::string const prot = getProtocol(url);
					
					if ( prot == "http" || prot == "https" )
					{
						libmaus::network::HttpSocketInputStreamBuffer::unique_ptr_type tptr(
							new libmaus::network::HttpSocketInputStreamBuffer(url,bufsize,pushbacksize)
						);
						httpstreambuffer = UNIQUE_PTR_MOVE(tptr);
					}
					else if ( prot == "ftp" )
					{
						libmaus::network::FtpSocketInputStreamBuffer::unique_ptr_type tptr(
							new libmaus::network::FtpSocketInputStreamBuffer(url,bufsize,pushbacksize)
						);
						ftpstreambuffer = UNIQUE_PTR_MOVE(tptr);
					}
					else if ( prot == "file" )
					{
						FileUrl fu(url);
						libmaus::aio::PosixFdInput::unique_ptr_type tfileinput(
							new libmaus::aio::PosixFdInput(fu.filename)
						);
						fileinput = UNIQUE_PTR_MOVE(tfileinput);
						libmaus::aio::PosixFdInputStreamBuffer::unique_ptr_type tptr(
							new libmaus::aio::PosixFdInputStreamBuffer(*fileinput,bufsize,pushbacksize)
						);
						filestreambuffer = UNIQUE_PTR_MOVE(tptr);
					}
					else
					{
						
						libmaus::exception::LibMausException lme;
						lme.getStream() << "UrlInputStreamBufferWrapper(): unsupported protocol " << url << "\n";
						lme.finish();
						throw lme;
					}
				}
				else
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "UrlInputStreamBufferWrapper(): " << url << " is not a recognised URL" << "\n";
					lme.finish();
					throw lme;
				}
			}
			virtual ~UrlInputStreamBufferWrapper()
			{
				filestreambuffer.reset();
				fileinput.reset();
				ftpstreambuffer.reset();
				httpstreambuffer.reset();
			}
			
			std::streambuf * getStreamBuf()
			{
				if ( httpstreambuffer.get() )
					return httpstreambuffer.get();
				else if ( ftpstreambuffer.get() )
					return ftpstreambuffer.get();
				else
					return filestreambuffer.get();
			}
		};	
	}
}
#endif
