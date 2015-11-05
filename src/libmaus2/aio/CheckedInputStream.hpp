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
#if ! defined(LIBMAUS2_AIO_CHECKEDINPUTSTREAM_HPP)
#define LIBMAUS2_AIO_CHECKEDINPUTSTREAM_HPP

#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/util/shared_ptr.hpp>
#include <libmaus2/exception/LibMausException.hpp>
#include <fstream>
#include <string>

namespace libmaus2
{
	namespace aio
	{
		/**
		 * checked input stream. this is a variant of std::ifstream which checks the
		 * state of the input stream after calls to read and seekg. If the stream
		 * fails, the functions throw exceptions. Note that the read/seekg functions
		 * are not virtual, so passing an object of this class as an std::istream &
		 * will loose the throwing functionality.
		 **/
		struct CheckedInputStream : std::ifstream
		{
			//! this type
			typedef CheckedInputStream this_type;
			//! unique pointer type
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			//! shared pointer type
			typedef ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			private:
			std::string const filename;
			char * buffer;

			void cleanup()
			{
				if ( buffer )
				{
					delete [] buffer;
					buffer = 0;
					rdbuf()->pubsetbuf(0,0);
				}
			}

			CheckedInputStream & operator=(CheckedInputStream &);
			CheckedInputStream(CheckedInputStream const &);

			public:
			/**
			 * constructor
			 *
			 * @param rfilename file name
			 * @param mode file open mode (as for std::ifstream)
			 **/
			CheckedInputStream(std::string const & rfilename, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::binary )
			: std::ifstream(rfilename.c_str(), mode), filename(rfilename), buffer(0)
			{
				if ( ! is_open() )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "CheckedInputStream::CheckedInputStream(): Cannot open file " << filename << std::endl;
					se.finish();
					throw se;
				}
			}
			/**
			 * destructor
			 **/
			~CheckedInputStream()
			{
				cleanup();
			}

			/**
			 * set size of underlying streambuf object
			 **/
			void setBufferSize(uint64_t const newbufsize)
			{
				cleanup();
				buffer = new char[newbufsize];
				rdbuf()->pubsetbuf(buffer,newbufsize);
			}

			/**
			 * read n bytes from stream to c. throws an exception if n!=0 and no bytes were read
			 *
			 * @param c output buffer space
			 * @param n number of bytes to be read
			 * @return *this
			 **/
			CheckedInputStream & read(char * c, ::std::streamsize const n)
			{
				std::ifstream::read(c,n);

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
			CheckedInputStream & read(char * c, ::std::streamsize n, ::std::streamsize const b)
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
			CheckedInputStream & seekg(::std::streampos pos)
			{
				std::ifstream::seekg(pos);

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
			 * @param dir seek direction (see std::ifstream::seekg)
			 * @return *this
			 **/
			CheckedInputStream & seekg(::std::streamoff off, ::std::ios_base::seekdir dir)
			{
				std::ifstream::seekg(off,dir);

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
			 * get a single symbol from file filename at position offset
			 *
			 * @param filename name of file
			 * @param offset file offset
			 * @return symbol/byte at position offset in file filename
			 **/
			static int getSymbolAtPosition(std::string const & filename, uint64_t const offset)
			{
				this_type CIS(filename);
				CIS.seekg(offset);
				return CIS.get();
			}

			/**
			 * return size of file filename
			 *
			 * @param filename name of file
			 * @return size of file filename in bytes
			 **/
			static uint64_t getFileSize(std::string const & filename)
			{
				this_type CIS(filename);
				CIS.seekg(0,std::ios::end);
				return CIS.tellg();
			}
		};
	}
}
#endif
