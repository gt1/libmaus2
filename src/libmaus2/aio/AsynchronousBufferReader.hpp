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
#if ! defined(ASYNCHRONOUSBUFFERREADER_HPP)
#define ASYNCHRONOUSBUFFERREADER_HPP

#include <libmaus2/LibMausConfig.hpp>
#if defined(LIBMAUS2_HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include <iostream>
#include <string>
#include <cstring>
#include <stdexcept>
#include <cassert>
#include <libmaus2/LibMausConfig.hpp>
#include <libmaus2/exception/LibMausException.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/util/GetFileSize.hpp>

#if defined(LIBMAUS2_HAVE_AIO)
#include <aio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>

namespace libmaus2
{
	namespace aio
	{
		/**
		 * class for asynchronous blockwise input
		 **/
		struct AsynchronousBufferReader
		{
			private:
			std::string filename;
			int const fd;
			unsigned int const numbuffers;
			unsigned int const bufsize;
			::libmaus2::autoarray::AutoArray < char > bufferspace;
			::libmaus2::autoarray::AutoArray < char * > buffers;
			::libmaus2::autoarray::AutoArray < aiocb > contexts;
			uint64_t low;
			uint64_t high;
			uint64_t offset;

			void enqueRead()
			{
				memset ( &contexts[high%numbuffers], 0, sizeof(aiocb) );
				contexts[high%numbuffers].aio_fildes = fd;
				contexts[high%numbuffers].aio_buf = buffers[high % numbuffers];
				contexts[high%numbuffers].aio_nbytes = bufsize;
				contexts[high%numbuffers].aio_offset = offset + high*bufsize;
				contexts[high%numbuffers].aio_sigevent.sigev_notify = SIGEV_NONE;
				aio_read( & contexts[high%numbuffers] );
				high++;
			}
			void waitForNextBuffer()
			{
				aiocb *waitlist[1] = { &contexts[low%numbuffers] };
				int r = aio_suspend (waitlist,1,0);

				if ( r != 0 )
				{
					std::cerr << "ERROR: " << strerror(errno) << std::endl;
				}
			}

			public:
			/**
			 * destructor
			 **/
			~AsynchronousBufferReader()
			{
				flush();
				close(fd);
			}

			/**
			 * constructor
			 *
			 * @param rfilename file name
			 * @param rnumbuffers number of buffers
			 * @param rbufsize size of each buffer
			 * @param roffset initial file offset
			 **/
			AsynchronousBufferReader ( std::string const & rfilename, uint64_t rnumbuffers = 16, uint64_t rbufsize = 32, uint64_t roffset = 0 )
			: filename(rfilename),
			  fd( open(filename.c_str(),O_RDONLY ) ),
			  numbuffers(rnumbuffers), bufsize(rbufsize),
			  bufferspace ( numbuffers * bufsize ),
			  buffers ( numbuffers ),
			  contexts(numbuffers), low(0), high(0), offset(roffset)
			{
				if ( fd < 0 )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "::libmaus2::aio::AsynchronousBufferReader: Failed to open file " << filename << ": " << strerror(errno);
					se.finish();

					/*
					std::cerr << se.s << std::endl;

					kill ( getpid(), SIGSTOP );
					*/

					throw se;
				}

				for ( unsigned int i = 0; i < numbuffers; ++i )
					buffers[i] = bufferspace.get() + i*bufsize;

				while ( high < numbuffers )
					enqueRead();
			}

			/**
			 * get next buffer
			 *
			 * @param data reference to pair that will be filled with block info. the pair will be filled with a data pointer and the block size
			 * @return true if block was available, false if there are no more blocks
			 **/
			bool getBuffer(std::pair < char const *, ssize_t > & data)
			{
				ssize_t red = -1;

				assert ( low != high );
				waitForNextBuffer();
				red = aio_return(&contexts[low%numbuffers]);
				data = std::pair < char const *, ssize_t > (buffers[low%numbuffers],red );

				low++;
				return red > 0;
			}
			/**
			 * return the last block obtained via getBuffer
			 **/
			void returnBuffer()
			{
				enqueRead();
			}
			/**
			 * wait for all read requests already submitted to finish
			 **/
			void flush()
			{
				while ( high-low )
				{
					waitForNextBuffer();
					low++;
				}
			}
		};
	}
}
#else

#include <fstream>

namespace libmaus2
{
	namespace aio
	{
		/**
		 * synchronous input with asynchronous api (for systems without asynchronous input)
		 **/
		struct AsynchronousBufferReader : libmaus2::aio::InputStreamInstance
		{
			private:
			uint64_t const bufsize;
			::libmaus2::autoarray::AutoArray<char> abuffer;
			char * const buffer;
			bool av;

			public:
			/**
			 * constructor
			 *
			 * @param filename file name
			 * @param rnumbufs number of buffers
			 * @param rbufsize size of each buffer
			 * @param offset initial file offset
			 **/
			AsynchronousBufferReader(
				std::string const & filename,
				uint64_t const rnumbufs,
				uint64_t const rbufsize,
				uint64_t const offset
			)
			: libmaus2::aio::InputStreamInstance(filename), bufsize(rnumbufs * rbufsize),
                          abuffer(bufsize), buffer(abuffer.get()), av(true)
			{
				libmaus2::aio::InputStreamInstance::seekg(offset,std::ios::beg);
			}
			/**
			 * get next buffer
			 *
			 * @param data reference to pair that will be filled with block info. the pair will be filled with a data pointer and the block size
			 * @return true if block was available, false if there are no more blocks
			 **/
			bool getBuffer(std::pair < char const *, ssize_t > & data)
			{
				assert ( av );

				libmaus2::aio::InputStreamInstance::read(buffer,bufsize);

				data.first = buffer;
				data.second = libmaus2::aio::InputStreamInstance::gcount();
				av = false;

				// std::cerr << "Got " << data.second << std::endl;

				return data.second > 0;
			}
			/**
			 * return the last block obtained via getBuffer
			 **/
			void returnBuffer()
			{
				av = true;
			}
			/**
			 * wait for all read requests already submitted to finish
			 **/
			void flush()
			{}
		};
	}
}
#endif

#include <string>
#include <list>
#include <fstream>
#include <libmaus2/util/unique_ptr.hpp>

namespace libmaus2
{
	namespace aio
	{
		/**
		 * asynchronous input for a list of files
		 **/
		struct AsynchronousBufferReaderList
		{
			private:
			typedef std::string element_type;
			typedef std::list<element_type> container_type;
			typedef container_type::const_iterator iterator_type;

			typedef AsynchronousBufferReader reader_type;
			typedef ::libmaus2::util::unique_ptr< reader_type >::type reader_ptr_type;

			container_type const C;
			iterator_type ita;
			iterator_type ite;
			uint64_t const numbuffers;
			uint64_t const bufsize;

			reader_ptr_type reader;

			public:
			/**
			 * constructor
			 *
			 * @param ina file name list begin iterator
			 * @param ine file name list end iterator
			 * @param rnumbuffers number of buffers
			 * @param rbufsize size of each buffer
			 * @param offset file offset
			 **/
			template<typename in_iterator_type>
			AsynchronousBufferReaderList(
				in_iterator_type ina,
				in_iterator_type ine,
				uint64_t rnumbuffers = 16,
				uint64_t rbufsize = 32,
				uint64_t offset = 0)
			:
				C(ina,ine), ita(C.begin()), ite(C.end()), numbuffers(rnumbuffers), bufsize(rbufsize)
			{
				while ( ita != ite && offset >= ::libmaus2::util::GetFileSize::getFileSize(*ita) )
				{
					offset -= ::libmaus2::util::GetFileSize::getFileSize(*ita);
					ita++;
				}

				if ( ita != ite )
				{
					reader_ptr_type treader(new AsynchronousBufferReader(*ita, numbuffers,bufsize,offset));
					reader = UNIQUE_PTR_MOVE(treader);
				}
			}
			/**
			 * destructor
			 **/
			~AsynchronousBufferReaderList()
			{
				if ( reader.get() )
					reader->flush();
			}
			/**
			 * get next buffer
			 *
			 * @param data reference to pair that will be filled with block info. the pair will be filled with a data pointer and the block size
			 * @return true if block was available, false if there are no more blocks
			 **/
			bool getBuffer(std::pair < char const *, ssize_t > & data)
			{
				if ( ! (reader.get()) )
					return false;

				if ( reader->getBuffer(data) )
					return true;

				if ( ita != ite )
				{
					while ( ++ita != ite )
					{
						reader.reset();
						reader_ptr_type treader(new AsynchronousBufferReader(*ita,numbuffers,bufsize,0));
						reader = UNIQUE_PTR_MOVE(treader);
						if ( reader->getBuffer(data) )
							return true;
					}
				}

				return false;
			}
			/**
			 * return the last block obtained via getBuffer
			 **/
			void returnBuffer()
			{
				if ( reader.get() )
					reader->returnBuffer();
			}
		};
	}
}
#endif
