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
#if ! defined(LIBMAUS_AIO_SYNCHRONOUSOUTPUTBUFFER8POSIX_HPP)
#define LIBMAUS_AIO_SYNCHRONOUSOUTPUTBUFFER8POSIX_HPP

#include <libmaus/types/types.hpp>
#include <libmaus/aio/AsynchronousWriter.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/util/unique_ptr.hpp>
#include <string>

namespace libmaus
{
	namespace aio
	{
		/**
		 * synchronous block wise output for 8 byte (uint64_t) numbers. this class does not keep
		 * the output file open across method calls.
		 **/
		struct SynchronousOutputBuffer8Posix
		{
			//! this type
			typedef SynchronousOutputBuffer8Posix this_type;
			//! unique pointer type
			typedef ::libmaus::util::unique_ptr < this_type > :: type unique_ptr_type;
			
			//! value type
			typedef uint64_t value_type;

			private:
			//! output file name
			std::string const filename;
			//! output buffer
			::libmaus::autoarray::AutoArray<value_type> B;
			//! output buffer start pointer
			value_type * const pa;
			//! output buffer current pointer
			value_type * pc;
			//! output buffer end pointer
			value_type * const pe;
			//! output position
			value_type ptr;

			/**
			 * write contents of the buffer to the file
			 **/
                        void writeBuffer()
                        {
                                int const fd = open(filename.c_str(),O_WRONLY);
                                
                                if ( fd < 0 )
                                {
				        ::libmaus::exception::LibMausException se;
				        se.getStream() << "Failed to open buffer file " << filename << " in SynchronousOutputBuffer8Posix::writeBuffer(): "
				                << strerror(errno);
				        se.finish();
				        throw se;                                
                                }
                                
                                if ( lseek(fd,ptr,SEEK_SET) == static_cast<off_t>(-1) )
                                {
                                        close(fd);

				        ::libmaus::exception::LibMausException se;
				        se.getStream() << "Failed to seek in file " << filename << " in SynchronousOutputBuffer8Posix::writeBuffer()";
				        se.finish();
				        throw se;                                
                                }
                                
                                uint64_t const towrite = reinterpret_cast<char const *>(pc)-reinterpret_cast<char const *>(pa);
                                ssize_t const written = ::write(fd,pa,towrite);
                        
				if ( written != static_cast<ssize_t>(towrite) )
				{
				        close(fd);
				        
				        ::libmaus::exception::LibMausException se;
				        se.getStream() << "Failed to write buffer in SynchronousOutputBuffer8Posix::writeBuffer()";
				        se.finish();
				        throw se;
				}

				#if 0
				if ( fsync(fd) < 0 )
				{
				        close(fd);

				        ::libmaus::exception::LibMausException se;
				        se.getStream() << "Failed to flush file in SynchronousOutputBuffer8Posix::writeBuffer()";
				        se.finish();
				        throw se;
				}
				#endif
				
				if ( close(fd) )
				{
				        ::libmaus::exception::LibMausException se;
				        se.getStream() << "Failed to close file properly in SynchronousOutputBuffer8Posix::writeBuffer()";
				        se.finish();
				        throw se;				
				}

				ptr += towrite;
                                pc = pa;
                        }

			public:
			/**
			 * constructor
			 *
			 * @param rfilename output file name
			 * @param bufsize output buffer size
			 * @param truncate if true, then truncate during constructio
			 **/
			SynchronousOutputBuffer8Posix(std::string const & rfilename, uint64_t const bufsize, bool truncate = true)
			: filename(rfilename), B(bufsize), pa(B.get()), pc(pa), pe(pa+B.getN()), ptr(0)
			{
				if ( truncate )
				{
					int const tres = ::truncate(filename.c_str(),0);
					if ( tres )
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "SynchronousOutputBuffer8Posix::SynchronousOutputBuffer8Posix(): truncate() failed: " << strerror(errno) << std::endl;
						se.finish();
						throw se;
					}
				}
                        }
                        /**
                         * destructor, flushes the buffer
                         **/
			~SynchronousOutputBuffer8Posix()
			{
				flush();
			}

			/**
			 * flush the buffer
			 **/
                        void flush()
                        {
                                writeBuffer();
                        }

                        /**
                         * put one element c in the buffer
                         *
                         * @param c element to be put in the buffer
                         **/
                        void put(value_type const c)
                        {
                                *(pc++) = c;
                                if ( pc == pe )
                                        writeBuffer();
                        }
                };
	}
}
#endif
