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
#include <libmaus/aio/CheckedInputStream.hpp>
#include <libmaus/util/Utf8DecoderBuffer.hpp>
#include <libmaus/util/UnsignedCharVariant.hpp>

namespace libmaus
{
	namespace aio
	{
		template<typename _stream_type>
		struct CircularBufferTemplate : public ::std::basic_streambuf<typename _stream_type::char_type>
		{
			private:
			typedef _stream_type stream_type;
			typedef typename stream_type::char_type char_type;
			typedef ::std::basic_streambuf<char_type> base_type;
			typedef typename ::libmaus::util::UnsignedCharVariant<char_type>::type unsigned_char_type;
			
			typename stream_type::unique_ptr_type Pstream;
			::std::basic_istream<char_type> & stream;
			
			uint64_t const buffersize;
			uint64_t const pushbackspace;
			::libmaus::autoarray::AutoArray<char_type> buffer;
			uint64_t streamreadpos;
			uint64_t const infilesize;

			CircularBufferTemplate(CircularBufferTemplate<stream_type> const &);
			CircularBufferTemplate & operator=(CircularBufferTemplate<stream_type> &);
			
			public:
			CircularBufferTemplate(
				std::string const & filename, 
				uint64_t const offset, 
				::std::size_t rbuffersize, 
				std::size_t rpushbackspace
			)
			: Pstream(new stream_type(filename)),
			  stream(*Pstream),
			  buffersize(rbuffersize),
			  pushbackspace(rpushbackspace),
			  buffer(buffersize+pushbackspace,false), 
			  streamreadpos(0),
			  infilesize(::libmaus::util::GetFileSize::getFileSize(stream))
			{
				stream.seekg(offset);
				base_type::setg(buffer.end(), buffer.end(), buffer.end());	
			}

			CircularBufferTemplate(
				std::basic_istream<char_type> & rstream,
				uint64_t const offset, 
				::std::size_t rbuffersize, 
				std::size_t rpushbackspace
			)
			: Pstream(),
			  stream(rstream),
			  buffersize(rbuffersize),
			  pushbackspace(rpushbackspace),
			  buffer(buffersize+pushbackspace,false), 
			  streamreadpos(0),
			  infilesize(::libmaus::util::GetFileSize::getFileSize(stream))
			{
				stream.seekg(offset);
				base_type::setg(buffer.end(), buffer.end(), buffer.end());	
			}
			
			uint64_t tellg() const
			{
				// std::cerr << "here." << std::endl;
				return streamreadpos - (base_type::egptr()-base_type::gptr());
			}
			
			private:
			// gptr as unsigned pointer
			unsigned_char_type const * uptr() const
			{
				return reinterpret_cast<unsigned_char_type const *>(base_type::gptr());
			}
			
			typename base_type::int_type underflow()
			{
				if ( base_type::gptr() < base_type::egptr() )
					return static_cast<typename base_type::int_type>(*uptr());
					
				assert ( base_type::gptr() == base_type::egptr() );
					
				char_type * midptr = buffer.begin() + pushbackspace;
				uint64_t const copyavail = 
					std::min(
						// previously read
						static_cast<uint64_t>(base_type::gptr()-base_type::eback()),
						// space we have to copy into
						static_cast<uint64_t>(midptr-buffer.begin())
					);
				::std::memmove(midptr-copyavail,base_type::gptr()-copyavail,copyavail*sizeof(char_type));

				if ( 
					static_cast<int64_t>(stream.tellg()) == static_cast<int64_t>(infilesize) 
				)
				{
					stream.seekg(0);
					stream.clear();
				}
				uint64_t const rspace = infilesize - stream.tellg();
				
				stream.read(midptr, std::min(rspace,static_cast<uint64_t>(buffer.end()-midptr)));
				size_t const n = stream.gcount();
				streamreadpos += n;

				base_type::setg(midptr-copyavail, midptr, midptr+n);

				if (!n)
					return base_type::traits_type::eof();
				
				return static_cast<typename base_type::int_type>(*uptr());
			}
		};

		typedef CircularBufferTemplate< ::libmaus::aio::CheckedInputStream  > CircularBuffer;
		typedef CircularBufferTemplate< ::libmaus::util::Utf8DecoderWrapper > Utf8CircularBuffer;

		template<typename _stream_type>
		struct CircularReverseBufferTemplate : public ::std::basic_streambuf<typename _stream_type::char_type>
		{
			protected:
			typedef _stream_type stream_type;
			typedef typename stream_type::char_type char_type;
			typedef ::std::basic_streambuf<char_type> base_type;
			typedef typename ::libmaus::util::UnsignedCharVariant<char_type>::type unsigned_char_type;

			typename stream_type::unique_ptr_type Pstream;
			::std::basic_istream<char_type> & stream;
			
			uint64_t const buffersize;
			uint64_t const pushbackspace;
			::libmaus::autoarray::AutoArray<char_type> buffer;
			uint64_t streamreadpos;
			uint64_t const infilesize;

			CircularReverseBufferTemplate(CircularReverseBufferTemplate const &);
			CircularReverseBufferTemplate & operator=(CircularReverseBufferTemplate&);
			
			public:
			CircularReverseBufferTemplate(std::string const & filename, uint64_t const offset, ::std::size_t rbuffersize, std::size_t rpushbackspace)
			: Pstream(new stream_type(filename)),
			  stream(*Pstream),
			  buffersize(rbuffersize),
			  pushbackspace(rpushbackspace),
			  buffer(buffersize+pushbackspace,false), streamreadpos(0),
			  infilesize(::libmaus::util::GetFileSize::getFileSize(stream))
			{
				stream.seekg(offset);
				base_type::setg(buffer.end(), buffer.end(), buffer.end());	
			}

			CircularReverseBufferTemplate(std::basic_istream<char_type> & rstream, uint64_t const offset, ::std::size_t rbuffersize, std::size_t rpushbackspace)
			: Pstream(),
			  stream(rstream),
			  buffersize(rbuffersize),
			  pushbackspace(rpushbackspace),
			  buffer(buffersize+pushbackspace,false), streamreadpos(0),
			  infilesize(::libmaus::util::GetFileSize::getFileSize(stream))
			{
				stream.seekg(offset);
				base_type::setg(buffer.end(), buffer.end(), buffer.end());	
			}
			
			uint64_t tellg() const
			{
				// std::cerr << "here." << std::endl;
				return streamreadpos - (base_type::egptr()-base_type::gptr());
			}
			
			private:
			// gptr as unsigned pointer
			unsigned_char_type const * uptr() const
			{
				return reinterpret_cast<unsigned_char_type const *>(base_type::gptr());
			}
			
			typename base_type::int_type underflow()
			{
				if ( base_type::gptr() < base_type::egptr() )
					return static_cast<typename base_type::int_type>(*uptr());
					
				assert ( base_type::gptr() == base_type::egptr() );
					
				char_type * midptr = buffer.begin() + pushbackspace;
				uint64_t const copyavail = 
					std::min(
						// previously read
						static_cast<uint64_t>(base_type::gptr()-base_type::eback()),
						// space we have to copy into
						static_cast<uint64_t>(midptr-buffer.begin())
					);
				::std::memmove(midptr-copyavail,base_type::gptr()-copyavail,copyavail*sizeof(char_type));

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

				base_type::setg(midptr-copyavail, midptr, midptr+n);

				if (!n)
					return base_type::traits_type::eof();
				
				return static_cast<typename base_type::int_type>(*uptr());
			}
		};

		typedef CircularReverseBufferTemplate< ::libmaus::aio::CheckedInputStream  > CircularReverseBuffer;
		typedef CircularReverseBufferTemplate< ::libmaus::util::Utf8DecoderWrapper > Utf8CircularReverseBuffer;
	}
}
#endif
