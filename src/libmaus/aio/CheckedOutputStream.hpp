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


#if ! defined(LIBMAUS_AIO_CHECKEDOUTPUTSTREAM_HPP)
#define LIBMAUS_AIO_CHECKEDOUTPUTSTREAM_HPP

#include <fstream>
#include <string>
#include <cerrno>
#include <cstring>
#include <libmaus/exception/LibMausException.hpp>
#include <libmaus/util/unique_ptr.hpp>
#include <libmaus/util/shared_ptr.hpp>

namespace libmaus
{
	namespace aio
	{
		/**
		 * checked output stream similar to std::ofstream, but throws exceptions
		 * if write or flush operations fail. note that the write/flush functions
		 * are not virtual, so passing an object of this type as a std::ostream reference
		 * will loose the throwing functionality.
		 **/
		struct CheckedOutputStream : public std::ofstream
		{
			typedef CheckedOutputStream this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			private:
			std::string const filename;
			
			public:
			/**
			 * constructor
			 *
			 * @param rfilename name of file
			 * @param mode output mode (see std::ofstream)
			 **/
			CheckedOutputStream(std::string const & rfilename, std::ios_base::openmode mode = std::ios_base::out | std::ios_base::binary)
			: std::ofstream(rfilename.c_str(), mode), filename(rfilename)
			{
				if ( ! is_open() )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Failed to open file " << filename << " for writing in CheckedOutputStream: " << strerror(errno) << std::endl;
					se.finish();
					throw se;
				}
			}
			/**
			 * destructor (flushes output stream)
			 **/
			~CheckedOutputStream()
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
			CheckedOutputStream & write(char const * c, ::std::streamsize n)
			{
				std::ofstream::write(c,n);
				
				if ( ! *this )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Failed to write to file " << filename << " in CheckedOutputStream::write()" << strerror(errno) << std::endl;
					se.finish();
					throw se;
				}
				
				return *this;
			}
			
			/**
			 * flush streambuf object. throws an exception if stream state goes bad
			 **/
			CheckedOutputStream & flush()
			{
				std::ofstream::flush();
				
				if ( ! *this )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Failed to flush to file " << filename << " in CheckedOutputStream::flush()" << strerror(errno) << std::endl;
					se.finish();
					throw se;
				}
				
				return *this;
			}
		};
	}
}
#endif
