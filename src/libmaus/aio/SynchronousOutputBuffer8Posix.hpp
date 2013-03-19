/**
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
**/

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
		struct SynchronousOutputBuffer8Posix
		{
			typedef SynchronousOutputBuffer8Posix this_type;
			typedef ::libmaus::util::unique_ptr < this_type > :: type unique_ptr_type;
			
			typedef uint64_t value_type;

			std::string const filename;
			::libmaus::autoarray::AutoArray<value_type> B;
			value_type * const pa;
			value_type * pc;
			value_type * const pe;
			value_type ptr;

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
			~SynchronousOutputBuffer8Posix()
			{
				flush();
			}

                        void flush()
                        {
                                writeBuffer();
                        }

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
