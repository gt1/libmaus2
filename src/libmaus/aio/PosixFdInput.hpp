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
#if ! defined(LIBMAUS_AIO_POSIXFDINPUT_HPP)
#define LIBMAUS_AIO_POSIXFDINPUT_HPP

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#if defined(__linux__)
#include <sys/vfs.h>
#elif defined(__APPLE__) || defined(__FreeBSD__)
#include <sys/param.h>
#include <sys/mount.h>          
#endif

#include <cerrno>
#include <cstring>
#include <libmaus/exception/LibMausException.hpp>
#include <libmaus/lz/StreamWrapper.hpp>

namespace libmaus
{
	namespace aio
	{
		struct PosixFdInput
		{
			public:
			typedef PosixFdInput this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			private:
			std::string filename;
			int fd;
			ssize_t gcnt;
			
			public:
			static int getDefaultFlags()
			{
				#if defined(__APPLE__) || defined(__FreeBSD__)
				return O_RDONLY;
				#else
				return O_RDONLY|O_LARGEFILE;
				#endif
			}
			
			PosixFdInput(int const rfd) : filename(), fd(rfd), gcnt(0)
			{
			}
			
			PosixFdInput(std::string const & rfilename, int const rflags = getDefaultFlags())
			: filename(rfilename), fd(-1), gcnt(0)
			{
				while ( fd == -1 )
				{
					fd = ::open(filename.c_str(),rflags);
					
					if ( fd < 0 )
					{
						switch ( errno )
						{
							case EINTR:
							case EAGAIN:
								break;
							default:
							{
								int const error = errno;
								libmaus::exception::LibMausException se;
								se.getStream() << "PosixFdInput(" << filename << "," << rflags << "): " << strerror(error) << std::endl;
								se.finish();
								throw se;
							}
						}
					}
				}
			}
			
			ssize_t read(char * const buffer, uint64_t const n)
			{
				ssize_t r = -1;
				gcnt = 0;

				while ( fd >= 0 && r < 0 )
				{
					r = ::read(fd,buffer,n);
					
					if ( r < 0 )
					{
						switch ( errno )
						{
							case EINTR:
							case EAGAIN:
								break;
							default:
							{
								int const error = errno;
								libmaus::exception::LibMausException se;
								se.getStream() << "PosixFdInput::read(" << filename << "," << n << "): " << strerror(error) << std::endl;
								se.finish();
								throw se;
							}
						}
					}
					else
					{
						gcnt = r;
					}
				}

				return gcnt;
			}

			off_t lseek(uint64_t const n)
			{
				off_t r = static_cast<off_t>(-1);
				
				while ( fd >= 0 && r == static_cast<off_t>(-1) )
				{
					r = ::lseek(fd,n,SEEK_SET);
					
					if ( r < 0 )
					{
						switch ( errno )
						{
							case EINTR:
							case EAGAIN:
								break;
							default:
							{
								int const error = errno;
								libmaus::exception::LibMausException se;
								se.getStream() << "PosixFdInput::lseek(" << filename << "," << n << "): " << strerror(error) << std::endl;
								se.finish();
								throw se;
							}
						}
					}
				}

				return r;
			}
			
			ssize_t gcount() const
			{
				return gcnt;
			}
			
			void close()
			{	
				int r = -1;
				
				while ( fd >= 0 && r == -1 )
				{
					r = ::close(fd);
					
					if ( r < 0 )
					{
						switch ( errno )
						{
							case EINTR:
								break;
							default:
							{
								int const error = errno;
								libmaus::exception::LibMausException se;
								se.getStream() << "PosixFdInput::close(" << filename << "): " << strerror(error) << std::endl;
								se.finish();
								throw se;
							}
						}
					}
					else
					{
						fd = -1;
					}
				}
			}

			uint64_t size()
			{
				int r = -1;
				struct stat sb;

				while ( fd >= 0 && r < 0 )
				{
					r = fstat(fd,&sb);
					
					if ( r < 0 )
					{
						switch ( errno )
						{
							case EINTR:
							case EAGAIN:
								break;
							default:
							{
								int const error = errno;
								libmaus::exception::LibMausException se;
								se.getStream() << "PosixFdInput::size(" << filename << "): " << strerror(error) << std::endl;
								se.finish();
								throw se;
							}
						}
					}
				}

				if ( ! S_ISREG(sb.st_mode) )
				{
					libmaus::exception::LibMausException se;
					se.getStream() << "PosixFdInput::size(" << filename << "," << fd << "): file descriptor does not designate a (regular) file" << std::endl;
					se.finish();
					throw se;				
				}

				return sb.st_size;
			}

			int64_t sizeChecked()
			{
				int r = -1;
				struct stat sb;

				while ( fd >= 0 && r < 0 )
				{
					r = fstat(fd,&sb);
					
					if ( r < 0 )
					{
						switch ( errno )
						{
							case EINTR:
							case EAGAIN:
								break;
							default:
							{
								int const error = errno;
								libmaus::exception::LibMausException se;
								se.getStream() << "PosixFdInput::size(" << filename << "): " << strerror(error) << std::endl;
								se.finish();
								throw se;
							}
						}
					}
				}

				if ( S_ISREG(sb.st_mode) )
					return sb.st_size;
				else
					return -1;
			}
			
			int64_t getOptimalIOBlockSize()
			{
				return getOptimalIOBlockSize(fd, filename);
			}
			
			static int64_t getOptimalIOBlockSize(int const fd, std::string const & filename)
			{
				struct statfs buf;
				int r = -1;

				while ( fd >= 0 && r < 0 )
				{
					r = fstatfs(fd,&buf);
					
					if ( r < 0 )
					{
						switch ( errno )
						{
							case EINTR:
							case EAGAIN:
								break;
							// file system does not support statfs
							case ENOSYS:
								return -1;
							#if defined(__FreeBSD__)
							case EINVAL:
								return -1;
							#endif
							default:
							{
								int const error = errno;
								libmaus::exception::LibMausException se;
								se.getStream() << "PosixFdInput::size(" << filename << "): " << strerror(error) << std::endl;
								se.finish();
								throw se;
							}
						}
					}
				}

				return buf.f_bsize;
			}
		};
	}
}
#endif
