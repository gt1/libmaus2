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
#if ! defined(LIBMAUS2_AIO_POSIXFDOUTPUTSTREAMBUFFER_HPP)
#define LIBMAUS2_AIO_POSIXFDOUTPUTSTREAMBUFFER_HPP

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

#include <poll.h>

namespace libmaus2
{
	namespace aio
	{
		struct PosixFdOutputStreamBuffer : public ::std::streambuf
		{
			private:
			static double const warnThreshold;

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
					std::cerr << "[W] warning PosixFdOutputStreamBuffer: " << functionname << "(" << fd << ")" << " took " << time << "s ";
					if ( filename.size() )
						std::cerr << " on " << filename;
					std::cerr << std::endl;
				}
			}

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
			uint64_t writepos;
			std::string filename;

			off_t doSeekAbsolute(uint64_t const p, int const whence = SEEK_SET)
			{
				off_t off = static_cast<off_t>(-1);

				while ( off == static_cast<off_t>(-1) )
				{
					double const time_bef = getTime();
					off = ::lseek(fd,p,whence);
					double const time_aft = getTime();
					printWarning("lseek",time_aft-time_bef,filename,fd);

					if ( off == static_cast<off_t>(-1) )
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
								se.getStream() << "PosixOutputStreamBuffer::doSeekkAbsolute(): lseek() failed: " << strerror(error) << std::endl;
								se.finish();
								throw se;
							}
						}
					}
				}

				return off;
			}

			off_t getFileSize()
			{
				// current position in stream
				off_t const cur = doSeekAbsolute(0,SEEK_CUR);
				// end position of stream
				off_t const end = doSeekAbsolute(0,SEEK_END);
				// go back to previous current
				doSeekAbsolute(cur,SEEK_SET);
				// return end position
				return end;
			}

			void doClose()
			{
				int r = -1;

				while ( r < 0 )
				{
					double const time_bef = getTime();
					r = ::close(fd);
					double const time_aft = getTime();
					printWarning("close",time_aft-time_bef,filename,fd);

					if ( r < 0 )
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
								se.getStream() << "PosixOutputStreamBuffer::doClose(): close() failed: " << strerror(error) << std::endl;
								se.finish();
								throw se;
							}
						}
					}
				}
			}

			static int doOpen(std::string const & filename, int const flags = O_WRONLY | O_CREAT | O_TRUNC, int const mode = 0644)
			{
				int fd = -1;

				while ( fd < 0 )
				{
					double const time_bef = getTime();
					fd = ::open(filename.c_str(),flags,mode);
					double const time_aft = getTime();
					printWarning("open",time_aft-time_bef,filename,fd);

					if ( fd < 0 )
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
								se.getStream() << "PosixOutputStreamBuffer::doOpen(): open("<<filename<<") failed: " << strerror(error) << std::endl;
								se.finish();
								throw se;
							}
						}
					}
				}

				return fd;
			}

			void doFlush()
			{
				int r = -1;

				while ( r < 0 )
				{
					double const time_bef = getTime();
					r = ::fsync(fd);
					double const time_aft = getTime();
					printWarning("fsync",time_aft-time_bef,filename,fd);

					if ( r < 0 )
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
								se.getStream() << "PosixOutputStreamBuffer::doFlush(): fsync() failed: " << strerror(error) << std::endl;
								se.finish();
								throw se;
							}
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
					size_t const towrite = std::min(n,static_cast<uint64_t>(optblocksize));

					pollfd pfd = { fd, POLLOUT, 0 };
					double const time_bef_poll = getTime();
					int const ready = poll(&pfd, 1, std::floor(warnThreshold+0.5) * 1000ull);
					double const time_aft_poll = getTime();

					if ( ready == 1 && (pfd.revents & POLLOUT) )
					{
						double const time_bef = getTime();
						ssize_t const w = ::write(fd,p,towrite);
						double const time_aft = getTime();
						printWarning("write",time_aft-time_bef,filename,fd);

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
							writepos += w;
						}
					}
					// file descriptor not ready for writing
					else
					{
						printWarning("poll",time_aft_poll-time_bef_poll,filename,fd);
					}
				}

				assert ( ! n );
			}

			public:
			PosixFdOutputStreamBuffer(int const rfd, int64_t const rbuffersize)
			: fd(rfd),
			  closefd(false),
			  optblocksize(getOptimalIOBlockSize(fd,std::string())),
			  buffersize((rbuffersize <= 0) ? optblocksize : rbuffersize),
			  buffer(buffersize,false),
			  writepos(0),
			  filename()
			{
				setp(buffer.begin(),buffer.end()-1);
			}

			PosixFdOutputStreamBuffer(std::string const & fn, int64_t const rbuffersize, int const rflags = O_WRONLY | O_CREAT | O_TRUNC, int const rmode = 0644)
			:
			  fd(doOpen(fn,rflags,rmode)),
			  closefd(true),
			  optblocksize(getOptimalIOBlockSize(fd,fn)),
			  buffersize((rbuffersize <= 0) ? optblocksize : rbuffersize),
			  buffer(buffersize,false),
			  writepos(0),
			  filename(fn)
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

                        /**
                         * seek to absolute position
                         **/
                        ::std::streampos seekpos(::std::streampos sp, ::std::ios_base::openmode which = ::std::ios_base::in | ::std::ios_base::out)
                        {
                        	if ( (which & ::std::ios_base::out) )
                        	{
                        		doSync();
                        		doSeekAbsolute(sp);
                        		writepos = sp;
                        		return sp;
				}
				else
				{
					return -1;
				}
                        }

			/**
			 * relative seek
			 **/
			::std::streampos seekoff(::std::streamoff off, ::std::ios_base::seekdir way, ::std::ios_base::openmode which)
			{
				if ( way == ::std::ios_base::cur )
				{
					/* do not flush buffer if off == 0 (as for tellg) */
					if ( off == 0 )
					{
						if ( (which & ::std::ios_base::out) )
							return writepos + (pptr()-pbase());
						else
							return -1;
					}
					/* seek via absolute position */
					else
					{
						return seekpos(writepos + (pptr()-pbase()) + off, which);
					}
				}
				else if ( way == ::std::ios_base::beg )
				{
					return seekpos(off,which);
				}
				else if ( way == ::std::ios_base::end )
				{
					return seekpos(static_cast<int64_t>(getFileSize()) + off, which);
				}
				else
				{
					return -1;
				}
			}
		};
	}
}
#endif
