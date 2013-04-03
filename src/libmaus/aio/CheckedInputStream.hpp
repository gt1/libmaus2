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
#if ! defined(LIBMAUS_AIO_CHECKEDINPUTSTREAM_HPP)
#define LIBMAUS_AIO_CHECKEDINPUTSTREAM_HPP

#include <libmaus/util/unique_ptr.hpp>
#include <libmaus/util/shared_ptr.hpp>
#include <libmaus/exception/LibMausException.hpp>
#include <fstream>
#include <string>

namespace libmaus
{
	namespace aio
	{
		struct CheckedInputStream : std::ifstream
		{
			typedef CheckedInputStream this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			std::string const filename;
			
			CheckedInputStream(std::string const & rfilename, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::binary )
			: std::ifstream(rfilename.c_str(), mode), filename(rfilename)
			{
				if ( ! is_open() )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Cannot open file " << filename << std::endl;
					se.finish();
					throw se;
				}
			}
			
			CheckedInputStream & read(char * c, ::std::streamsize const n)
			{
				std::ifstream::read(c,n);
				
				if ( (n != 0) && (! gcount()) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Failed to read from file " << filename << std::endl;
					se.finish();
					throw se;				
				}
				
				return *this;
			}
			
			CheckedInputStream & seekg(::std::streampos pos)
			{
				std::ifstream::seekg(pos);
				
				if ( ! *this )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Failed to seek to position " << pos << " in file " << filename << std::endl;
					se.finish();
					throw se;								
				}
				
				return *this;
			}

			CheckedInputStream & seekg(::std::streamoff off, ::std::ios_base::seekdir dir)
			{
				std::ifstream::seekg(off,dir);
				
				if ( ! *this )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Failed to seek to offset " << off << " in file " << filename << std::endl;
					se.finish();
					throw se;								
				}
				
				return *this;
			}
			
			static int getSymbolAtPosition(std::string const & filename, uint64_t const offset)
			{
				this_type CIS(filename);
				CIS.seekg(offset);
				return CIS.get();
			}
		};
	}
}
#endif
