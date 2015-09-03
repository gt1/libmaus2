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
#if ! defined(LIBMAUS2_AIO_POSIXFDINPUTOUTPUTSTREAMBUFFER_HPP)
#define LIBMAUS2_AIO_POSIXFDINPUTOUTPUTSTREAMBUFFER_HPP

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

namespace libmaus2
{
	namespace aio
	{
		struct PosixFdInputOutputStreamBuffer : public ::std::streambuf
		{
			private:
			// get default block size
			static uint64_t getDefaultBlockSize()
			{
				return 64*1024;
			}

			// get optimal i/o block size
			static int64_t getOptimalIOBlockSize(int const fd, std::string const & fn)
			{
				int64_t const fsopt = libmaus2::aio::PosixFdInput::getOptimalIOBlockSize(fd,fn);
				
				if ( fsopt <= 0 )
					return getDefaultBlockSize();
				else
					return fsopt;
			}
			
			// file descriptor
			int fd;
			// close file at deconstruction if true
			bool closefd;
			// optimal block size for file system
			int64_t const optblocksize;
			// size of buffer
			uint64_t const buffersize;
			// buffer
			::libmaus2::autoarray::AutoArray<char> buffer;

			// read position
			uint64_t readpos;
			// write position
			uint64_t writepos;

			// open the file
			int doOpen(std::string const & filename, std::ios_base::openmode const cxxmode)
			{	
				if ( (cxxmode & std::ios::app) )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "libmaus2::aio::PosixFdInputOutputStreamBuffer::doOpen(): neither std::ios::app flag not supported" << std::endl;
					lme.finish();
					throw lme;				
				}
				if ( (cxxmode & std::ios::ate) )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "libmaus2::aio::PosixFdInputOutputStreamBuffer::doOpen(): neither std::ios::ate flag not supported" << std::endl;
					lme.finish();
					throw lme;
				}
				if ( ! ((cxxmode & std::ios::in) && (cxxmode & std::ios::out)) )
				{
				
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "libmaus2::aio::PosixFdInputOutputStreamBuffer::doOpen(): std::ios::in or std::ios::out not set " << std::endl;
					lme.finish();
					throw lme;
				}
				
				int mode = O_RDWR | O_CREAT;

				// truncate if requested
				if ( (cxxmode & std::ios::trunc) )
				{
					mode |= O_TRUNC;
				}
							
				int fd = -1;
				
				while ( (fd = open(filename.c_str(),mode,0644)) < 0 )
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
							se.getStream() << "PosixInputOutputStreamBuffer::doOpen(): open("<<filename<< "," << mode << ") failed: " << strerror(error) << std::endl;
							se.finish();
							throw se;
						}
					}					
				}
				
				return fd;
			}

			// close the file descriptor
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
							se.getStream() << "PosixInputOutputStreamBuffer::doClose(): close() failed: " << strerror(error) << std::endl;
							se.finish();
							throw se;
						}
					}					
				}
			}

			// flush the file
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
							se.getStream() << "PosixInputOutputStreamBuffer::doSync(): fsync() failed: " << strerror(error) << std::endl;
							se.finish();
							throw se;
						}
					}					
				}
			}
			

			// seek
			off_t doSeek(int64_t const p, int const whence)
			{
				off_t off = static_cast<off_t>(-1);
			
				while ( (off=::lseek(fd,p,whence)) == static_cast<off_t>(-1) )
				{
					int const error = errno;
					
					switch ( error )
					{
						case EINTR:
						case EAGAIN:
							// try again
							break;
						default:
						{
							libmaus2::exception::LibMausException se;
							se.getStream() << "PosixInputOutputStreamBuffer::doSeek(): seek() failed: " << strerror(error) << std::endl;
							se.finish();
							throw se;
						}
					}					
				}
								
				return off;
			}
			
			// write buffer contents
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
								se.getStream() << "PosixInputOutputStreamBuffer::doSync(): write() failed: " << strerror(error) << std::endl;
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
				
				assert ( ! n );
			}
			
			size_t doRead(char * buffer, size_t count)
			{
				ssize_t r = -1;
				
				while ( (r=::read(fd,buffer,count)) < 0 )
				{
					int const error = errno;
					
					switch ( error )
					{
						case EINTR:
						case EAGAIN:
							// try again
							break;
						default:
						{
							libmaus2::exception::LibMausException se;
							se.getStream() << "PosixInputOutputStreamBuffer::doRead(): read() failed: " << strerror(error) << std::endl;
							se.finish();
							throw se;
						}
					}					
				}
				
				return r;
			}

			// gptr as unsigned pointer
			uint8_t const * uptr() const
			{
				return reinterpret_cast<uint8_t const *>(gptr());
			}
			
			void checkWriteBuffer()
			{
				// if write buffer is not empty, then flush it
				if ( pptr() != pbase() )
				{
					doSync();
										
					// get write position
					assert ( static_cast<off_t>(writepos) == doSeek(0,SEEK_CUR) );
				}			
			}

			public:
			PosixFdInputOutputStreamBuffer(int const rfd, int64_t const rbuffersize)
			: fd(rfd), 
			  closefd(false), 
			  optblocksize((rbuffersize < 0) ? getOptimalIOBlockSize(fd,std::string()) : rbuffersize),
			  buffersize(optblocksize), 
			  buffer(buffersize,false),
			  readpos(0),
			  writepos(0)
			{
				setg(buffer.end(),buffer.end(),buffer.end());
				setp(buffer.begin(),buffer.end()-1);
			}

			PosixFdInputOutputStreamBuffer(std::string const & fn, std::ios_base::openmode const cxxmode, int64_t const rbuffersize)
			: 
			  fd(doOpen(fn,cxxmode)), 
			  closefd(true), 
			  optblocksize((rbuffersize < 0) ? getOptimalIOBlockSize(fd,fn) : rbuffersize),
			  buffersize(optblocksize), 
			  buffer(buffersize,false),
			  readpos(0),
			  writepos(0)
			{
				// empty get buffer
				setg(buffer.end(),buffer.end(),buffer.end());
				// empty put buffer
				setp(buffer.begin(),buffer.end()-1);
			}

			~PosixFdInputOutputStreamBuffer()
			{
				sync();
				if ( closefd )
					doClose();
			}

			int sync()
			{
				// write any data in the put buffer
				doSync();
				// flush file
				doFlush();
				return 0; // no error, -1 for error
			}			

			int_type underflow()
			{
				// if there is still data, then return it
				if ( gptr() < egptr() )
					return static_cast<int_type>(*uptr());
					
				assert ( gptr() == egptr() );

				// load data
				size_t const g = doRead(buffer.begin(),buffersize);
				
				// set buffer pointers
				setg(buffer.begin(),buffer.begin(),buffer.begin()+g);

				// update start of buffer position
				readpos += g;

				if ( g )
					return static_cast<int_type>(*uptr());
				else
					return traits_type::eof();
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

			/**
			 * seek to absolute position
			 **/
			::std::streampos seekpos(::std::streampos sp, ::std::ios_base::openmode /* which */)
			{			
				// flush write buffer before seeking anywhere
				checkWriteBuffer();
				// seek
				off_t const off = doSeek(sp,SEEK_SET);
				
				if ( off == static_cast<off_t>(-1) )
					return -1;
				
				// empty get buffer
				setg(buffer.end(),buffer.end(),buffer.end());
				// empty put buffer
				setp(buffer.begin(),buffer.end()-1);
				// set positions
				readpos = off;
				writepos = off;
				
				return off;
			}
			
			/**
			 * relative seek
			 **/
			::std::streampos seekoff(::std::streamoff off, ::std::ios_base::seekdir way, ::std::ios_base::openmode which)
			{
				// absolute seek
				if ( way == ::std::ios_base::beg )
				{
					return seekpos(off,which);
				}
				// seek relative to current position
				else if ( way == ::std::ios_base::cur )
				{
					if ( which == std::ios_base::in )
					{
						return seekpos(readpos + (gptr()-eback()) + off,which);
					}
					else if ( which == std::ios_base::out )
					{
						return seekpos(writepos + (pptr()-pbase()) + off,which);
					}
					else
					{
						return -1;
					}
				}
				// seek relative to end of file
				else if ( way == ::std::ios_base::end )
				{
					off_t const curoff = doSeek(0, SEEK_CUR);
					off_t const endoff = doSeek(0, SEEK_END);
					off_t const curag = doSeek(curoff,SEEK_SET);
					
					if ( curag != curoff )
						return -1;
					
					if ( endoff == static_cast<off_t>(-1) )
						return -1;
					
					return seekpos(endoff+off,which);
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
