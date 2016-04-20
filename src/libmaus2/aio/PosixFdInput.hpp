/*
    libmaus2
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
#if ! defined(LIBMAUS2_AIO_POSIXFDINPUT_HPP)
#define LIBMAUS2_AIO_POSIXFDINPUT_HPP

#include <libmaus2/LibMausConfig.hpp>
#if defined(LIBMAUS2_HAVE_UNISTD_H)
#include <unistd.h>
#endif

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
#include <libmaus2/exception/LibMausException.hpp>
#include <libmaus2/lz/StreamWrapper.hpp>
#include <libmaus2/aio/StreamLock.hpp>

#if defined(__linux__)
#include <sys/time.h>
#include <poll.h>
#endif

namespace libmaus2
{
	namespace aio
	{
		struct PosixFdInput
		{
			public:
			typedef PosixFdInput this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			private:
			std::string filename;
			int fd;
			ssize_t gcnt;
			bool const closeOnDeconstruct;

			static double const warnThreshold;

			static std::map<std::string,uint64_t> const blocksizeoverride;

			static double getTime()
			{
				#if defined(__linux__)
				if ( warnThreshold > 0.0 )
				{
					struct timeval tv;
					struct timezone tz;
					int const r = gettimeofday(&tv,&tz);
					if ( r < 0 )
						return 0.0;
					else
						return static_cast<double>(tv.tv_sec) + (static_cast<double>(tv.tv_usec)/1e6);
				}
				else
				#endif
					return 0.0;
			}

			static void printWarning(char const * const functionname, double const time, std::string const & filename, int const fd)
			{
				if ( warnThreshold > 0.0 && time >= warnThreshold )
				{
					libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
					std::cerr << "[W] warning PosixFdInput: " << functionname << "(" << fd << ")" << " took " << time << "s ";
					if ( filename.size() )
						std::cerr << " on " << filename;
					std::cerr << std::endl;
				}
			}

			static void printWarningRead(char const * const functionname, double const time, std::string const & filename, int const fd, uint64_t const size)
			{
				if ( warnThreshold > 0.0 && time >= warnThreshold )
				{
					libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
					std::cerr << "[W] warning PosixFdInput: " << functionname << "(" << fd << "," << size << ")" << " took " << time << "s ";
					if ( filename.size() )
						std::cerr << " on " << filename;
					std::cerr << std::endl;
				}
			}

			public:
			static int getDefaultFlags()
			{
				#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__)
				return O_RDONLY;
				#else
				return O_RDONLY|O_LARGEFILE;
				#endif
			}

			~PosixFdInput()
			{
				if ( closeOnDeconstruct && fd >= 0 )
					close();
			}

			static bool tryOpen(std::string const & filename, int const rflags = getDefaultFlags())
			{
				char const * cfilename = filename.c_str();
				int fd = -1;

				// try to open the file
				while ( fd == -1 )
				{
					double const time_bef = getTime();
					fd = ::open(cfilename,rflags);
					double const time_aft = getTime();
					printWarning("open",time_aft-time_bef,filename,fd);

					if ( fd == -1 )
					{
						switch ( errno )
						{
							case EINTR:
							case EAGAIN:
								break;
							default:
								// cannot open file
								return false;
						}
					}
				}

				assert ( fd != -1 );

				// close file
				while ( fd != -1 )
				{
					double const time_bef = getTime();
					int const r = ::close(fd);
					double const time_aft = getTime();
					printWarning("close",time_aft-time_bef,filename,fd);

					if ( r == -1 )
					{
						switch ( errno )
						{
							case EINTR:
							case EAGAIN:
								break;
							default:
							{
								int const error = errno;
								libmaus2::exception::LibMausException se;
								se.getStream() << "PosixFdInput::tryOpen(" << filename << "," << rflags << "): " << strerror(error) << std::endl;
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

				// could open file
				return true;
			}

			PosixFdInput(int const rfd) : filename(), fd(rfd), gcnt(0), closeOnDeconstruct(false)
			{
			}

			PosixFdInput(std::string const & rfilename, int const rflags = getDefaultFlags())
			: filename(rfilename), fd(-1), gcnt(0), closeOnDeconstruct(true)
			{
				while ( fd == -1 )
				{
					double const time_bef = getTime();
					fd = ::open(filename.c_str(),rflags);
					double const time_aft = getTime();
					printWarning("open",time_aft-time_bef,filename,fd);

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
								libmaus2::exception::LibMausException se;
								se.getStream() << "PosixFdInput(" << filename << "," << rflags << "): " << strerror(error) << std::endl;
								se.finish();
								throw se;
							}
						}
					}
				}
			}

			ssize_t read(char * const buffer, uint64_t n)
			{
				ssize_t r = -1;
				gcnt = 0;

				while ( fd >= 0 && r < 0 )
				{
					#if defined(__linux__)
					pollfd pfd = { fd, POLLIN, 0 };
					double const time_bef_poll = getTime();
					int const ready = poll(&pfd, 1, std::floor(warnThreshold+0.5) * 1000ull);
					double const time_aft_poll = getTime();

					if ( ready == 1 && (pfd.revents & POLLIN) )
					{
						double const time_bef = getTime();
						r = ::read(fd,buffer,n);
						double const time_aft = getTime();
						printWarningRead("read",time_aft-time_bef,filename,fd,n);

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
									libmaus2::exception::LibMausException se;
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
					else if ( ready == 1 && (pfd.revents & POLLHUP) )
					{
						n = 0;
					}
					else
					{
						printWarningRead("poll",time_aft_poll-time_bef_poll,filename,fd,n);
					}
					#else
					double const time_bef = getTime();
					r = ::read(fd,buffer,n);
					double const time_aft = getTime();
					printWarningRead("read",time_aft-time_bef,filename,fd,n);

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
								libmaus2::exception::LibMausException se;
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
					#endif
				}

				return gcnt;
			}

			off_t lseek(uint64_t const n)
			{
				off_t r = static_cast<off_t>(-1);

				while ( fd >= 0 && r == static_cast<off_t>(-1) )
				{
					double const time_bef = getTime();
					r = ::lseek(fd,n,SEEK_SET);
					double const time_aft = getTime();
					printWarning("lseek",time_aft-time_bef,filename,fd);

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
								libmaus2::exception::LibMausException se;
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
					double const time_bef = getTime();
					r = ::close(fd);
					double const time_aft = getTime();
					printWarning("close",time_aft-time_bef,filename,fd);

					if ( r == 0 )
					{
						fd = -1;
					}
					else
					{
						assert ( r == -1 );

						int const error = errno;

						switch ( error )
						{
							case EINTR:
								break;
							default:
							{
								libmaus2::exception::LibMausException se;
								se.getStream() << "PosixFdInput::close(" << filename << "," << fd << "): " << strerror(error) << std::endl;
								se.finish();
								throw se;
							}
						}
					}
				}
			}

			uint64_t size()
			{
				int r = -1;
				struct stat sb;

				while ( fd >= 0 && r < 0 )
				{
					double const time_bef = getTime();
					r = fstat(fd,&sb);
					double const time_aft = getTime();
					printWarning("fstat",time_aft-time_bef,filename,fd);

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
								libmaus2::exception::LibMausException se;
								se.getStream() << "PosixFdInput::size(" << filename << "): " << strerror(error) << std::endl;
								se.finish();
								throw se;
							}
						}
					}
				}

				if ( ! S_ISREG(sb.st_mode) )
				{
					libmaus2::exception::LibMausException se;
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
					double const time_bef = getTime();
					r = fstat(fd,&sb);
					double const time_aft = getTime();
					printWarning("fstat",time_aft-time_bef,filename,fd);

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
								libmaus2::exception::LibMausException se;
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

			static int64_t getOptimalIOBlockSize(
				#if defined(LIBMAUS2_HAVE_STATFS)
				int const fd,
				#else
				int const,
				#endif
				std::string const & filename
			)
			{
				int64_t override = -1;
				for ( std::map<std::string,uint64_t>::const_iterator ita = blocksizeoverride.begin(); ita != blocksizeoverride.end(); ++ita )
				{
					std::string const & key = ita->first;

					if ( key.size() <= filename.size() && filename.substr(0,key.size()) == key )
						override = static_cast<int64_t>(ita->second);
				}

				if ( override > 0 )
					return override;

				#if defined(LIBMAUS2_HAVE_STATFS)
				struct statfs buf;
				int r = -1;

				while ( fd >= 0 && r < 0 )
				{
					double const time_bef = getTime();
					r = fstatfs(fd,&buf);
					double const time_aft = getTime();
					printWarning("fstatfs",time_aft-time_bef,filename,fd);

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
								libmaus2::exception::LibMausException se;
								se.getStream() << "PosixFdInput::size(" << filename << "): " << strerror(error) << std::endl;
								se.finish();
								throw se;
							}
						}
					}
				}

				return buf.f_bsize;
				#else
				return -1;
				#endif
			}
		};
	}
}
#endif
