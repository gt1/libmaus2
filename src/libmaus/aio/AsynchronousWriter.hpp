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


#if ! defined(ASYNCHRONOUSWRITER_HPP)
#define ASYNCHRONOUSWRITER_HPP

#include <iostream>
#include <string>
#include <cstring>
#include <cerrno>
#include <stdexcept>
#include <cassert>
#include <fstream>
#include <libmaus/LibMausConfig.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/parallel/OMPLock.hpp>
#include <libmaus/exception/LibMausException.hpp>

#if defined(LIBMAUS_HAVE_AIO)
#include <aio.h>
#include <unistd.h>
#include <fcntl.h>

#if defined(__FreeBSD__)
#include <sys/uio.h>
#endif

namespace libmaus
{
	namespace aio
	{
		/**
		 * class for asynchronous output
		 **/
		struct AsynchronousWriter
		{
			private:
			int const fd;
			bool const releasefd;
			unsigned int const numbuffers;
			::libmaus::autoarray::AutoArray < ::libmaus::autoarray::AutoArray<char> > buffers;
			::libmaus::autoarray::AutoArray < aiocb > contexts;
			uint64_t low;
			uint64_t high;
			::libmaus::parallel::OMPLock lock;

			public:
			/**
			 * constructor from file descriptor
			 *
			 * @param rfd file descriptor
			 * @param rnumbuffers number of asynchronous contexts (buffered blocks)
			 **/
			AsynchronousWriter ( int const rfd, uint64_t rnumbuffers = 16 )
			: fd( rfd ), releasefd(false), numbuffers(rnumbuffers), buffers(numbuffers), contexts(numbuffers), low(0), high(0)
			{
				if ( fd < 0 )
				{
					throw std::runtime_error("Failed to open file.");
				}
				fcntl (fd, F_SETFL, O_APPEND);
			}
			/**
			 * constructor from filename
			 *
			 * @param filename name of file
			 * @param rnumbuffers number of asynchronous contexts (buffered blocks)
			 **/
			AsynchronousWriter ( std::string const & filename, uint64_t rnumbuffers = 16 )
			: fd( open(filename.c_str(),O_WRONLY|O_CREAT|O_APPEND|O_TRUNC,0644) ), releasefd(true), numbuffers(rnumbuffers), buffers(numbuffers), contexts(numbuffers), low(0), high(0)
			{
				if ( fd < 0 )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Failed to open file " << filename << ": " << strerror(errno) << std::endl;
					se.finish();
					throw se;
				}
			}
			/**
			 * destructor
			 **/
			~AsynchronousWriter()
			{
				flush();
				if ( releasefd )
				{
					close(fd);
				}
				else
				{
				}
			}
			
			/**
			 * register block for writing
			 *
			 * @param sa block start pointer/iterator (inclusive)
			 * @param se block end pointer/iterator (exclusive)
			 **/
			template<typename iterator>
			void write(iterator sa, iterator se)
			{
				lock.lock();

				// wait for free context if all buffers are in use
				if ( high-low == numbuffers )
				{
					aiocb *waitlist[1] = { &contexts[low%numbuffers] };
					aio_suspend (waitlist,1,0);
					low++;
				}
				
				uint64_t const len = se-sa;

				buffers[high % numbuffers] = ::libmaus::autoarray::AutoArray<char>(len);
				std::copy ( sa, se, buffers[high%numbuffers].get() );
				memset ( &contexts[high%numbuffers], 0, sizeof(aiocb) );
				contexts[high%numbuffers].aio_fildes = fd;
				contexts[high%numbuffers].aio_buf = buffers[high % numbuffers].get();
				contexts[high%numbuffers].aio_nbytes = len;
				contexts[high%numbuffers].aio_offset = 0;
				contexts[high%numbuffers].aio_sigevent.sigev_notify = SIGEV_NONE;
				aio_write( & contexts[high%numbuffers] );		

				high++;		
				
				lock.unlock();
			}
			
			/**
			 * wait until all write requests have finished
			 **/
			void flush()
			{
				while ( high-low )
				{
					aiocb *waitlist[1] = { &contexts[low%numbuffers] };
					aio_suspend (waitlist,1,0);
					ssize_t ret = aio_return ( &contexts[low%numbuffers] );
					assert ( ret == static_cast<ssize_t>(contexts[low%numbuffers].aio_nbytes) ) ;
					// std::cerr << "WRITTEN: " << ret << " requested " << contexts[low%numbuffers].aio_nbytes << std::endl;
					
					low++;
				}

				if ( fd != STDOUT_FILENO )
					if ( fsync(fd) )
						std::cerr << "Failure in fsync: " << strerror(errno) << std::endl;
			}
		};
	}
}
#else
namespace libmaus
{
	namespace aio
	{
		/**
		 * asynchronous write replacement class for systems which do not support asynchronous io
		 **/
		struct AsynchronousWriter
		{
			private:
			std::ofstream ostr;
			::libmaus::parallel::OMPLock lock;
			
			public:
			/**
			 * constructor
			 *
			 * @param filename name of file
			 **/
			AsynchronousWriter ( std::string const & filename, uint64_t = 16 )
			: ostr ( filename.c_str() )
			{
			
			}
			
			/**
			 * destructor
			 **/
			~AsynchronousWriter()
			{
				ostr.close();
			}
			
			/**
			 * write block
			 *
			 * @param sa block start pointer/iterator (inclusive)
			 * @param se block end pointer/iterator (exclusive)			 
			 **/
			template<typename iterator>
			void write(iterator sa, iterator se)
			{
				lock.lock();
				::libmaus::autoarray::AutoArray<char> buf(se-sa);
				std::copy(sa,se,buf.get());
				ostr.write(buf.get(),se-sa);
				lock.unlock();
			}
			
			/**
			 * flush output file
			 **/
			void flush()
			{
				lock.lock();
				ostr.flush();
				lock.unlock();
			}
		};
	}
}
#endif

#endif
