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
#if ! defined(LIBMAUS2_AIO_LINUXSTREAMINGPOSIXFDOUTPUTSTREAMBUFFER_HPP)
#define LIBMAUS2_AIO_LINUXSTREAMINGPOSIXFDOUTPUTSTREAMBUFFER_HPP

#include <libmaus2/LibMausConfig.hpp>
#if defined(LIBMAUS2_HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include <ostream>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/aio/PosixFdInput.hpp>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#if defined(LIBMAUS2_HAVE_FEATURES_H)
#include <features.h>
#endif

namespace libmaus2
{
	namespace aio
	{
		struct LinuxStreamingPosixFdOutputStreamBuffer : public ::std::streambuf
		{
			private:
			static uint64_t getDefaultBlockSize()
			{
				return 64*1024;
			}

			static int64_t getOptimalIOBlockSize(int const fd, std::string const & fn)
			{
				int64_t const fsopt = libmaus2::aio::PosixFdInput::getOptimalIOBlockSize(fd,fn);

				if ( fsopt <= 0 )
					return getDefaultBlockSize();
				else
					return fsopt;
			}

			int fd;
			bool closefd;
			int64_t const optblocksize;
			uint64_t const buffersize;
			::libmaus2::autoarray::AutoArray<char> buffer;
			std::pair<uint64_t,uint64_t> prevwrite;

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
							libmaus2::exception::LibMausException se;
							se.getStream() << "LinuxStreamingPosixFdOutputStreamBuffer::doClose(): close() failed: " << strerror(error) << std::endl;
							se.finish();
							throw se;
						}
					}
				}
			}

			int doOpen(std::string const & filename)
			{
				int fd = -1;

				while ( (fd = open(filename.c_str(),O_WRONLY | O_CREAT | O_TRUNC,0644)) < 0 )
				{
					int const error = errno;

					switch ( error )
					{
						case EINTR:
						case EAGAIN:
							break;
						default:
						{
							libmaus2::exception::LibMausException se;
							se.getStream() << "LinuxStreamingPosixFdOutputStreamBuffer::doOpen(): open("<<filename<<") failed: " << strerror(error) << std::endl;
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
							libmaus2::exception::LibMausException se;
							se.getStream() << "LinuxStreamingPosixFdOutputStreamBuffer::doSync(): fsync() failed: " << strerror(error) << std::endl;
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
								libmaus2::exception::LibMausException se;
								se.getStream() << "LinuxStreamingPosixFdOutputStreamBuffer::doSync(): write() failed: " << strerror(error) << std::endl;
								se.finish();
								throw se;
							}
						}
					}
					else
					{
						assert ( w <= static_cast<int64_t>(n) );

						#if defined(__GLIBC__)
						#if __GLIBC_PREREQ(2,6)
						// non block buffer cache flush request for range we have just written
						if ( w )
						{
							::sync_file_range(fd, prevwrite.first+prevwrite.second, w, SYNC_FILE_RANGE_WRITE);
						}
						if ( prevwrite.second )
						{
							// blocking buffer cache flush request for previous buffer
							::sync_file_range(fd, prevwrite.first, prevwrite.second, SYNC_FILE_RANGE_WAIT_BEFORE|SYNC_FILE_RANGE_WRITE|SYNC_FILE_RANGE_WAIT_AFTER);
							// tell kernel we no longer need the pages
							::posix_fadvise(fd, prevwrite.first, prevwrite.second, POSIX_FADV_DONTNEED);
						}
						#endif
						#endif

						prevwrite.first  = prevwrite.first + prevwrite.second;
						prevwrite.second = w;

						n -= w;
					}
				}

				assert ( ! n );
			}

			public:
			LinuxStreamingPosixFdOutputStreamBuffer(int const rfd, int64_t const rbuffersize)
			: fd(rfd), closefd(false),
			  optblocksize((rbuffersize < 0) ? getOptimalIOBlockSize(fd,std::string()) : rbuffersize),
			  buffersize(optblocksize),
			  buffer(buffersize,false), prevwrite(0,0)
			{
				setp(buffer.begin(),buffer.end()-1);
			}

			LinuxStreamingPosixFdOutputStreamBuffer(std::string const & fn, int64_t const rbuffersize)
			: fd(doOpen(fn)), closefd(true),
			  optblocksize((rbuffersize < 0) ? getOptimalIOBlockSize(fd,std::string()) : rbuffersize),
			  buffersize(optblocksize),
			  buffer(buffersize,false), prevwrite(0,0)
			{
				setp(buffer.begin(),buffer.end()-1);
			}

			~LinuxStreamingPosixFdOutputStreamBuffer()
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
