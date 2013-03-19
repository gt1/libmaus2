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

#if ! defined(LIBMAUS_AIO_CIRCULARBUFFER_HPP)
#define LIBMAUS_AIO_CIRCULARBUFFER_HPP

#include <istream>
#include <fstream>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/util/GetFileSize.hpp>

namespace libmaus
{
	namespace aio
	{
		struct CircularBuffer : public ::std::streambuf
		{
			private:
			std::ifstream stream;
			uint64_t const buffersize;
			uint64_t const pushbackspace;
			::libmaus::autoarray::AutoArray<char> buffer;
			uint64_t streamreadpos;
			uint64_t const infilesize;

			CircularBuffer(CircularBuffer const &);
			CircularBuffer & operator=(CircularBuffer&);
			
			public:
			CircularBuffer(std::string const & filename, uint64_t const offset, ::std::size_t rbuffersize, std::size_t rpushbackspace)
			: stream(filename.c_str(),std::ios::binary),
			  buffersize(rbuffersize),
			  pushbackspace(rpushbackspace),
			  buffer(buffersize+pushbackspace,false), streamreadpos(0),
			  infilesize(::libmaus::util::GetFileSize::getFileSize(filename))
			{
				stream.seekg(offset);
				setg(buffer.end(), buffer.end(), buffer.end());	
			}
			
			uint64_t tellg() const
			{
				// std::cerr << "here." << std::endl;
				return streamreadpos - (egptr()-gptr());
			}
			
			private:
			// gptr as unsigned pointer
			uint8_t const * uptr() const
			{
				return reinterpret_cast<uint8_t const *>(gptr());
			}
			
			int_type underflow()
			{
				if ( gptr() < egptr() )
					return static_cast<int_type>(*uptr());
					
				assert ( gptr() == egptr() );
					
				char * midptr = buffer.begin() + pushbackspace;
				uint64_t const copyavail = 
					std::min(
						// previously read
						static_cast<uint64_t>(gptr()-eback()),
						// space we have to copy into
						static_cast<uint64_t>(midptr-buffer.begin())
					);
				::std::memmove(midptr-copyavail,gptr()-copyavail,copyavail);

				if ( static_cast<int64_t>(stream.tellg()) == static_cast<int64_t>(infilesize) )
				{
					stream.seekg(0);
					stream.clear();
				}
				uint64_t const rspace = infilesize - stream.tellg();
				
				stream.read(midptr, std::min(rspace,static_cast<uint64_t>(buffer.end()-midptr)));
				size_t const n = stream.gcount();
				streamreadpos += n;

				setg(midptr-copyavail, midptr, midptr+n);

				if (!n)
					return traits_type::eof();
				
				return static_cast<int_type>(*uptr());
			}
		};

		struct CircularReverseBuffer : public ::std::streambuf
		{
			private:
			std::ifstream stream;
			uint64_t const buffersize;
			uint64_t const pushbackspace;
			::libmaus::autoarray::AutoArray<char> buffer;
			uint64_t streamreadpos;
			uint64_t const infilesize;

			CircularReverseBuffer(CircularReverseBuffer const &);
			CircularReverseBuffer & operator=(CircularReverseBuffer&);
			
			public:
			CircularReverseBuffer(std::string const & filename, uint64_t const offset, ::std::size_t rbuffersize, std::size_t rpushbackspace)
			: stream(filename.c_str(),std::ios::binary),
			  buffersize(rbuffersize),
			  pushbackspace(rpushbackspace),
			  buffer(buffersize+pushbackspace,false), streamreadpos(0),
			  infilesize(::libmaus::util::GetFileSize::getFileSize(filename))
			{
				stream.seekg(offset);
				setg(buffer.end(), buffer.end(), buffer.end());	
			}
			
			uint64_t tellg() const
			{
				// std::cerr << "here." << std::endl;
				return streamreadpos - (egptr()-gptr());
			}
			
			private:
			// gptr as unsigned pointer
			uint8_t const * uptr() const
			{
				return reinterpret_cast<uint8_t const *>(gptr());
			}
			
			int_type underflow()
			{
				if ( gptr() < egptr() )
					return static_cast<int_type>(*uptr());
					
				assert ( gptr() == egptr() );
					
				char * midptr = buffer.begin() + pushbackspace;
				uint64_t const copyavail = 
					std::min(
						// previously read
						static_cast<uint64_t>(gptr()-eback()),
						// space we have to copy into
						static_cast<uint64_t>(midptr-buffer.begin())
					);
				::std::memmove(midptr-copyavail,gptr()-copyavail,copyavail);

				if ( static_cast<int64_t>(stream.tellg()) == static_cast<int64_t>(0) )
				{
					stream.seekg(infilesize);
					stream.clear();
				}
				
				uint64_t const rspace = stream.tellg();
				uint64_t const toread = std::min(rspace,static_cast<uint64_t>(buffer.end()-midptr));
				
				stream.seekg(-static_cast<int64_t>(toread),std::ios::cur);
				stream.clear();
				
				stream.read(midptr, toread);
				size_t const n = stream.gcount();
				assert ( n == toread );
				std::reverse(midptr,midptr+n);
				streamreadpos += n;

				stream.seekg(-static_cast<int64_t>(toread),std::ios::cur);
				stream.clear();

				setg(midptr-copyavail, midptr, midptr+n);

				if (!n)
					return traits_type::eof();
				
				return static_cast<int_type>(*uptr());
			}
		};
	}
}
#endif
