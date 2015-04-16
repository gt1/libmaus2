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


#if ! defined(LIBMAUS_AIO_CHECKEDINPUTOUTPUTSTREAM_HPP)
#define LIBMAUS_AIO_CHECKEDINPUTOUTPUTSTREAM_HPP

#include <fstream>
#include <string>
#include <cerrno>
#include <cstring>
#include <libmaus2/exception/LibMausException.hpp>
#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/util/shared_ptr.hpp>

namespace libmaus2
{
	namespace aio
	{
		/**
		 * checked file stream similar to std::fstream, but throws exceptions
		 * if read, write or flush operations fail. note that the read/write/flush functions
		 * are not virtual, so passing an object of this type as a std::iostream reference
		 * will loose the throwing functionality.
		 **/
		struct CheckedInputOutputStream : public std::fstream
		{
			//! this type
			typedef CheckedInputOutputStream this_type;
			//! unique pointer type
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			//! shared pointer type
			typedef ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			
			private:
			std::string const filename;
			
			public:
			/**
			 * constructor
			 *
			 * @param rfilename name of file
			 * @param mode output mode (see std::fstream)
			 **/
			CheckedInputOutputStream(
				std::string const & rfilename, 
				std::ios_base::openmode mode = std::ios::in | std::ios_base::out | std::ios_base::binary | std::ios::trunc
			)
			: std::fstream(rfilename.c_str(), mode), filename(rfilename)
			{
				if ( ! is_open() )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Failed to open file " << filename << " in CheckedInputOutputStream: " << strerror(errno) << std::endl;
					se.finish();
					throw se;
				}
			}
			/**
			 * destructor (flushes output stream)
			 **/
			~CheckedInputOutputStream()
			{
				flush();
			}
			
			/**
			 * write n bytes from c to output stream. throws an exception if operation fails
			 * 
			 * @param c buffer to be written
			 * @param n size of buffer
			 * @return *this
			 **/
			CheckedInputOutputStream & write(char const * c, ::std::streamsize n)
			{
				std::fstream::write(c,n);
				
				if ( ! *this )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Failed to write to file " << filename << " in CheckedInputOutputStream::write()" << strerror(errno) << std::endl;
					se.finish();
					throw se;
				}
				
				return *this;
			}
			
			/**
			 * flush streambuf object. throws an exception if stream state goes bad
			 **/
			CheckedInputOutputStream & flush()
			{
				std::fstream::flush();
				
				if ( ! *this )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Failed to flush to file " << filename << " in CheckedInputOutputStream::flush()" << strerror(errno) << std::endl;
					se.finish();
					throw se;
				}
				
				return *this;
			}

			/**
			 * read n bytes from stream to c. throws an exception if n!=0 and no bytes were read
			 *
			 * @param c output buffer space
			 * @param n number of bytes to be read
			 * @return *this
			 **/
			CheckedInputOutputStream & read(char * c, ::std::streamsize const n)
			{
				std::fstream::read(c,n);
				
				if ( (n != 0) && (! gcount()) )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Failed to read from file " << filename << std::endl;
					se.finish();
					throw se;				
				}
				
				return *this;
			}

			/**
			 * read n bytes from stream to c in blocks of size b. 
			 * throws an exception if n!=0 and no bytes were read
			 *
			 * @param c output buffer space
			 * @param n number of bytes to be read
			 * @param b block size
			 * @return *this
			 **/
			CheckedInputOutputStream & read(char * c, ::std::streamsize n, ::std::streamsize const b)
			{
				while ( n )
				{
					::std::streamsize const toread = std::min(n,b);
					read(c,toread);
					c += toread;
					n -= toread;
				}
				
				return *this;
			}
			
			/**
			 * seek stream to absolute position pos. throws an exception if not successfull
			 *
			 * @param pos new absolute position
			 * @return *this
			 **/
			CheckedInputOutputStream & seekg(::std::streampos pos)
			{
				std::fstream::seekg(pos);
				
				if ( ! *this )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Failed to seek to position " << pos << " in file " << filename << std::endl;
					se.finish();
					throw se;								
				}
				
				return *this;
			}

			/**
			 * seek stream to relative position pos. throws an exception if not successfull
			 *
			 * @param off seek offset
			 * @param dir seek direction (see std::fstream::seekg)
			 * @return *this
			 **/
			CheckedInputOutputStream & seekg(::std::streamoff off, ::std::ios_base::seekdir dir)
			{
				std::fstream::seekg(off,dir);
				
				if ( ! *this )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Failed to seek to offset " << off << " in file " << filename << std::endl;
					se.finish();
					throw se;								
				}
				
				return *this;
			}			

			/**
			 * seek stream to absolute position pos. throws an exception if not successfull
			 *
			 * @param pos new absolute position
			 * @return *this
			 **/
			CheckedInputOutputStream & seekp(::std::streampos pos)
			{
				std::fstream::seekp(pos);
				
				if ( ! *this )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Failed to seek to position " << pos << " in file " << filename << std::endl;
					se.finish();
					throw se;								
				}
				
				return *this;
			}

			/**
			 * seek stream to relative position pos. throws an exception if not successfull
			 *
			 * @param off seek offset
			 * @param dir seek direction (see std::fstream::seekg)
			 * @return *this
			 **/
			CheckedInputOutputStream & seekp(::std::streamoff off, ::std::ios_base::seekdir dir)
			{
				std::fstream::seekp(off,dir);
				
				if ( ! *this )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Failed to seek to offset " << off << " in file " << filename << std::endl;
					se.finish();
					throw se;								
				}
				
				return *this;
			}			
		};
	}
}
#endif
