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
#if ! defined(LIBMAUS2_AIO_MEMORYOUTPUTSTREAMBUFFER_HPP)
#define LIBMAUS2_AIO_MEMORYOUTPUTSTREAMBUFFER_HPP

#include <libmaus2/LibMausConfig.hpp>
#if defined(LIBMAUS2_HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include <ostream>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/aio/MemoryFileContainer.hpp>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace libmaus2
{
	namespace aio
	{
		struct MemoryOutputStreamBuffer : public ::std::streambuf
		{
			private:
			// get default block size
			static uint64_t getDefaultBlockSize()
			{
				return 64*1024;
			}

			// memory file
			libmaus2::aio::MemoryFileAdapter::shared_ptr_type fd;
			uint64_t const buffersize;
			::libmaus2::autoarray::AutoArray<char> buffer;
			uint64_t writepos;

			void doClose()
			{
			}

			// open the file
			libmaus2::aio::MemoryFileAdapter::shared_ptr_type doOpen(std::string const & filename)
			{
				libmaus2::aio::MemoryFileAdapter::shared_ptr_type ptr(libmaus2::aio::MemoryFileContainer::getEntry(filename));
				ptr->truncate();
				return ptr;
			}

			void doSync()
			{
				uint64_t n = pptr()-pbase();
				pbump(-n);

				char * p = pbase();

				while ( n )
				{
					ssize_t const w = fd->write(p,n);

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
								se.getStream() << "MemoryOutputStreamBuffer::doSync(): write() failed: " << strerror(error) << std::endl;
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


			public:
			MemoryOutputStreamBuffer(std::string const & fn, int64_t const rbuffersize)
			:
			  fd(doOpen(fn)),
			  buffersize((rbuffersize < 0) ? getDefaultBlockSize() : rbuffersize),
			  buffer(buffersize,false),
			  writepos(0)
			{
				setp(buffer.begin(),buffer.end()-1);
			}

			~MemoryOutputStreamBuffer()
			{
				sync();
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
					fd->lseek(sp,SEEK_SET);
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
					return seekpos(writepos + (pptr()-pbase()));
				else if ( way == ::std::ios_base::beg )
					return seekpos(off,which);
				else if ( way == ::std::ios_base::end )
					return seekpos(fd->getFileSize() + off, which);
				else
					return -1;
			}

		};
	}
}
#endif
