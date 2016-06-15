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
#include <libmaus2/aio/StreamLock.hpp>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <poll.h>

#include <cmath>
#include <fstream>

namespace libmaus2
{
	namespace aio
	{
		struct PosixFdOutputStreamBuffer : public ::std::streambuf
		{
			private:
			static double const warnThreshold;
			static int const check;
			static uint64_t volatile totalout;
			static libmaus2::parallel::PosixSpinLock totaloutlock;

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

			std::string filename;
			std::string checkfilename;
			int fd;
			int checkfd;
			bool closefd;
			int64_t const optblocksize;
			uint64_t const buffersize;
			::libmaus2::autoarray::AutoArray<char> buffer;
			uint64_t writepos;

			static off_t doSeekAbsolute(int const fd, std::string const & filename, uint64_t const p, int const whence /* = SEEK_SET */)
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

			static off_t doGetFileSize(int const fd, std::string const & filename)
			{
				// current position in stream
				off_t const cur = doSeekAbsolute(fd,filename,0,SEEK_CUR);
				// end position of stream
				off_t const end = doSeekAbsolute(fd,filename,0,SEEK_END);
				// go back to previous current
				off_t const final = doSeekAbsolute(fd,filename,cur,SEEK_SET);

				if ( final != cur )
				{
					libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
					std::cerr << "libmaus2::aio::PosixFdOutputStreamBuffer::doGetFileSize(" << fd << "," << filename << "), failed to seek back to original position: " << final << " != " << cur << std::endl;
				}

				// return end position
				return end;
			}

			static void doClose(int const fd, std::string const & filename)
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

			static void doFlush(int const fd, std::string const & filename)
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

			static uint64_t
				doWrite(
					int const fd,
					std::string const & filename,
					char * p,
					uint64_t n,
					int64_t const optblocksize,
					uint64_t writepos
				)
			{
				while ( n )
				{
					size_t const towrite = std::min(n,static_cast<uint64_t>(optblocksize));

					#if defined(__linux__)
					pollfd pfd = { fd, POLLOUT, 0 };
					double const time_bef_poll = getTime();
					int const polltimeout = (warnThreshold > 0.0) ? static_cast<int>(std::floor(warnThreshold+0.5) * 1000ull) : -1;
					int const ready = poll(&pfd, 1, polltimeout);
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
							totaloutlock.lock();
							totalout += w;
							totaloutlock.unlock();

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
					#else
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
							totaloutlock.lock();
							totalout += w;
							totaloutlock.unlock();

							assert ( w <= static_cast<int64_t>(n) );
							n -= w;
							writepos += w;
						}
					}
					#endif
				}

				assert ( ! n );

				return writepos;
			}

			void doSync()
			{
				uint64_t const n = pptr()-pbase();
				pbump(-n);
				char * const p = pbase();
				uint64_t const origwritepos = writepos;
				writepos = doWrite(fd,filename,p,n,optblocksize,origwritepos);
				if ( checkfd != -1 )
				{
					uint64_t const checkwritepos = doWrite(checkfd,filename,p,n,optblocksize,origwritepos);
					if ( checkwritepos != writepos )
					{
						libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
						std::cerr << "libmaus2::aio::PosixFdOutputStreamBuffer:doSync(): checkwritepos!=writepos, " << checkwritepos << "!=" << writepos << std::endl;
					}
				}
			}

			public:
			PosixFdOutputStreamBuffer(int const rfd, int64_t const rbuffersize)
			:
			  filename(),
			  checkfilename(),
			  fd(rfd),
			  checkfd(-1),
			  closefd(false),
			  optblocksize(getOptimalIOBlockSize(fd,std::string())),
			  buffersize((rbuffersize <= 0) ? optblocksize : rbuffersize),
			  buffer(buffersize,false),
			  writepos(0)
			{
				setp(buffer.begin(),buffer.end()-1);
			}

			PosixFdOutputStreamBuffer(
				std::string const & rfilename,
				int64_t const rbuffersize,
				int const rflags = O_WRONLY | O_CREAT | O_TRUNC,
				int const rmode = 0644
			)
			:
			  filename(rfilename),
			  checkfilename(filename + ".check"),
			  fd(doOpen(filename,rflags,rmode)),
			  checkfd(check ? doOpen(checkfilename,rflags,rmode) : -1),
			  closefd(true),
			  optblocksize(getOptimalIOBlockSize(fd,filename)),
			  buffersize((rbuffersize <= 0) ? optblocksize : rbuffersize),
			  buffer(buffersize,false),
			  writepos(0)
			{
				setp(buffer.begin(),buffer.end()-1);
			}

			~PosixFdOutputStreamBuffer()
			{
				sync();
				if ( closefd )
				{
					if ( fd != -1 )
					{
						doClose(fd,filename);
						fd = -1;
					}

					bool const havecheckfile = (checkfd != -1);

					if ( havecheckfile )
					{
						doClose(checkfd,filename);
						checkfd = -1;
					}

					if ( havecheckfile )
					{
						// instantiate streams
						::std::ifstream istr0(filename.c_str(),std::ios::binary);
						::std::ifstream istr1(filename.c_str(),std::ios::binary);

						// see if they are both open
						bool ok0 = istr0.is_open();
						bool ok1 = istr1.is_open();

						bool gok = true;

						if ( ok0 != ok1 )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							std::cerr << "libmaus2::aio::PosixFdOutputStreamBuffer:~PosixFdOutputStreamBuffer(): " << filename << " is_open ok0=" << ok0 << " ok1=" << ok1 << std::endl;
							gok = false;
						}
						else
						{
							gok = ok0;

							uint64_t const n = 64*1024;
							libmaus2::autoarray::AutoArray<char> BU0(n,false);
							libmaus2::autoarray::AutoArray<char> BU1(n,false);

							while ( istr0 && istr1 )
							{
								istr0.read(BU0.begin(),n);
								istr1.read(BU1.begin(),n);

								if ( istr0.gcount() != istr1.gcount() )
								{
									libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
									std::cerr << "libmaus2::aio::PosixFdOutputStreamBuffer:~PosixFdOutputStreamBuffer(): " << filename << " gcount istr0=" << istr0.gcount() << " != istr1=" << istr1.gcount() << std::endl;
									gok = false;
									break;
								}
								else if ( ! std::equal(BU0.begin(),BU0.begin()+istr0.gcount(),BU1.begin()) )
								{
									libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
									std::cerr << "libmaus2::aio::PosixFdOutputStreamBuffer:~PosixFdOutputStreamBuffer(): " << filename << " data equality failure" << std::endl;
									gok = false;
									break;
								}
							}

							ok0 = istr0 ? true : false;
							ok1 = istr1 ? true : false;

							if ( ok0 != ok1 )
							{
								libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
								std::cerr << "libmaus2::aio::PosixFdOutputStreamBuffer:~PosixFdOutputStreamBuffer(): " << filename << " final eq failure" << std::endl;
								gok = false;
							}
						}

						istr0.close();
						istr1.close();

						{
							if ( !gok )
							{
								libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
								std::cerr << "libmaus2::aio::PosixFdOutputStreamBuffer:~PosixFdOutputStreamBuffer(): " << filename << " FAILED" << std::endl;
							}
						}

						if ( checkfilename.size() )
							::remove(checkfilename.c_str());
					}
				}
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
				doFlush(fd,filename);
				if ( checkfd != -1 )
					doFlush(checkfd,filename);
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
					sp = doSeekAbsolute(fd,filename,sp,SEEK_SET);
					if ( checkfd != -1 )
					{
						::std::streampos const checksp = doSeekAbsolute(checkfd,filename,sp,SEEK_SET);
						if ( checksp != sp )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							std::cerr << "libmaus2::aio::PosixFdOutputStreamBuffer:seekpos(): checksp != sp, " << checksp << "!=" << sp << std::endl;
						}
					}
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
					int64_t const filesize = static_cast<int64_t>(doGetFileSize(fd,filename));
					if ( checkfd != -1 )
					{
						int64_t const checkfilesize = static_cast<int64_t>(doGetFileSize(checkfd,filename));
						if ( checkfilesize != filesize )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							std::cerr << "libmaus2::aio::PosixFdOutputStreamBuffer::seekoff(): checkfilesize != filesize, " << checkfilesize << "!=" << filesize << std::endl;
						}
					}
					int64_t const spos = filesize + off;
					return seekpos(spos, which);
				}
				else
				{
					return -1;
				}
			}

			static uint64_t getTotalOut()
			{
				uint64_t ltotalout;
				totaloutlock.lock();
				ltotalout = totalout;
				totaloutlock.unlock();
				return ltotalout;
			}
		};
	}
}
#endif
