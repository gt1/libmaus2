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
#if ! defined(LIBMAUS2_AIO_LINESPLITTINGPOSIXFDOUTPUTBUFFER_HPP)
#define LIBMAUS2_AIO_LINESPLITTINGPOSIXFDOUTPUTBUFFER_HPP

#include <libmaus2/LibMausConfig.hpp>
#if defined(LIBMAUS2_HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include <ostream>
#include <sstream>
#include <iomanip>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/aio/PosixFdInput.hpp>
#include <libmaus2/aio/FileRemoval.hpp>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace libmaus2
{
	namespace aio
	{
		struct LineSplittingPosixFdOutputStreamBuffer : public ::std::streambuf
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

			std::string fn;
			uint64_t linemod;
			uint64_t linecnt;
			uint64_t fileno;
			uint64_t written;
			std::string openfilename;

			int fd;
			int64_t const optblocksize;
			uint64_t const buffersize;
			::libmaus2::autoarray::AutoArray<char> buffer;

			bool reopenpending;

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
							se.getStream() << "LineSplittingPosixFdOutputStreamBuffer::doSync(): fsync() failed: " << strerror(error) << std::endl;
							se.finish();
							throw se;
						}
					}
				}
			}

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
							se.getStream() << "LineSplittingPosixFdOutputStreamBuffer::doClose(): close() failed: " << strerror(error) << std::endl;
							se.finish();
							throw se;
						}
					}
				}

				openfilename = std::string();
				written = 0;
			}

			int doOpen()
			{
				std::string const filename = getFileName(fileno++);

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
							se.getStream() << "LineSplittingPosixFdOutputStreamBuffer::doOpen(): open("<<filename<<") failed: " << strerror(error) << std::endl;
							se.finish();
							throw se;
						}
					}
				}

				written = 0;
				openfilename = filename;

				return fd;
			}

			void doSync(char * p, uint64_t n)
			{
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
								se.getStream() << "LineSplittingPosixFdOutputStreamBuffer::doSync(): write() failed: " << strerror(error) << std::endl;
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

			void doSync()
			{
				// number of bytes in buffer
				uint64_t n = pptr()-pbase();
				pbump(-n);
				char * p = pbase();

				while ( n )
				{
					if ( reopenpending )
					{
						doFlush();
						doClose();
						fd = doOpen();
						reopenpending = false;
					}

					char * pe = p + n;
					char * pc = p;

					while ( pc != pe )
						if ( *(pc++) == '\n' && ((++linecnt) % linemod) == 0 )
						{
							reopenpending = true;
							break;
						}

					uint64_t const t = pc - p;

					doSync(p,t);

					p += t;
					n -= t;
				}
			}

			public:
			LineSplittingPosixFdOutputStreamBuffer(std::string const & rfn, uint64_t const rlinemod, int64_t const rbuffersize)
			:
			  fn(rfn),
			  linemod(rlinemod),
			  linecnt(0),
			  fileno(0),
			  written(0),
			  fd(doOpen()),
			  optblocksize((rbuffersize < 0) ? getOptimalIOBlockSize(fd,fn) : rbuffersize),
			  buffersize(optblocksize),
			  buffer(buffersize,false),
			  reopenpending(false)
			{
				setp(buffer.begin(),buffer.end()-1);
			}

			~LineSplittingPosixFdOutputStreamBuffer()
			{
				sync();

				std::string deletefilename = openfilename;
				bool const deletefile = ( (written == 0) && fileno == 1 );

				doClose();

				// delete empty file if no data was written
				if ( deletefile )
					libmaus2::aio::FileRemoval::removeFile(deletefilename);
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

			std::string getFileName(uint64_t const id) const
			{
				std::ostringstream fnostr;

				fnostr << fn << "_" << std::setw(6) << std::setfill('0') << id << std::setw(0);

				return fnostr.str();
			}

			uint64_t getNumFiles() const
			{
				return fileno;
			}
		};
	}
}
#endif
