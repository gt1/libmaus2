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
#if ! defined(LIBMAUS_AIO_POSIXFDOUTPUTSTREAMBUFFER_HPP)
#define LIBMAUS_AIO_POSIXFDOUTPUTSTREAMBUFFER_HPP

#include <ostream>
#include <libmaus/autoarray/AutoArray.hpp>

namespace libmaus
{
	namespace aio
	{
		struct PosixFdOutputStreamBuffer : public ::std::streambuf
		{
			private:
			int fd;
			bool closefd;
			uint64_t const buffersize;
			::libmaus::autoarray::AutoArray<char> buffer;

			void doClose()
			{
				while ( close(fd) < 0 )
				{
					int const error = errno;
					
					switch ( error )
					{
						case EINTR:
						case EAGAIN:
							break;
						default:
						{
							libmaus::exception::LibMausException se;
							se.getStream() << "PosixOutputStreamBuffer::doClose(): close() failed: " << strerror(error) << std::endl;
							se.finish();
							throw se;
						}
					}					
				}
			}

			int doOpen(std::string const & filename)
			{
				int fd = -1;
				
				while ( (fd = open(filename.c_str(),O_WRONLY | O_CREAT,0644)) < 0 )
				{
					int const error = errno;
					
					switch ( error )
					{
						case EINTR:
						case EAGAIN:
							break;
						default:
						{
							libmaus::exception::LibMausException se;
							se.getStream() << "PosixOutputStreamBuffer::doOpen(): open("<<filename<<") failed: " << strerror(error) << std::endl;
							se.finish();
							throw se;
						}
					}					
				}
				
				return fd;
			}

			void doFlush()
			{
				while ( fsync(fd) < 0 )
				{
					int const error = errno;
					
					switch ( error )
					{
						case EINTR:
						case EAGAIN:
							break;
						case EROFS:
						case EINVAL:
							// descriptor does not support flushing
							return;
						default:
						{
							libmaus::exception::LibMausException se;
							se.getStream() << "PosixOutputStreamBuffer::doSync(): fsync() failed: " << strerror(error) << std::endl;
							se.finish();
							throw se;
						}
					}					
				}
			}
			
			void doSync()
			{
				uint64_t n = pptr()-pbase();
				pbump(-n);

				char * p = pbase();

				while ( n )
				{
					ssize_t const w = ::write(fd,p,n);
					
					if ( w < 0 )
					{
						int const error = errno;
						
						switch ( error )
						{
							case EINTR:
							case EAGAIN:
								break;
							default:
							{
								libmaus::exception::LibMausException se;
								se.getStream() << "PosixOutputStreamBuffer::doSync(): write() failed: " << strerror(error) << std::endl;
								se.finish();
								throw se;
							}
						}
					}
					else
					{
						assert ( w <= static_cast<int64_t>(n) );
						n -= w;
					}
				}
				
				assert ( ! n );
			}

			public:
			PosixFdOutputStreamBuffer(int const rfd, uint64_t const rbuffersize)
			: fd(rfd), closefd(false), buffersize(rbuffersize), buffer(buffersize,false)
			{
				setp(buffer.begin(),buffer.end()-1);
			}

			PosixFdOutputStreamBuffer(std::string const & fn, uint64_t const rbuffersize)
			: fd(doOpen(fn)), closefd(true), buffersize(rbuffersize), buffer(buffersize,false)
			{
				setp(buffer.begin(),buffer.end()-1);
			}

			~PosixFdOutputStreamBuffer()
			{
				sync();
				if ( closefd )
					doClose();
			}
			
			int_type overflow(int_type c = traits_type::eof())
			{
				if ( c != traits_type::eof() )
				{
					*pptr() = c;
					pbump(1);
					doSync();
				}

				return c;
			}
			
			int sync()
			{
				doSync();
				doFlush();
				return 0; // no error, -1 for error
			}			
		};
	}
}
#endif
