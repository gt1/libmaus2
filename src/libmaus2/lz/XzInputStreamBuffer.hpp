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
#if ! defined(LIBMAUS2_LZ_XZINPUTSTREAMBUFFER_HPP)
#define LIBMAUS2_LZ_XZINPUTSTREAMBUFFER_HPP

#include <streambuf>
#include <libmaus2/lz/XzDecoder.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct XzInputStreamBuffer : public ::std::streambuf
		{
			typedef XzInputStreamBuffer this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
		
			private:
			std::istream & stream;
			libmaus2::lz::XzDecoder xzdec;
			uint64_t const buffersize;
			uint64_t const pushbackspace;
			::libmaus2::autoarray::AutoArray<char> buffer;
			uint64_t streamreadpos;

			XzInputStreamBuffer(XzInputStreamBuffer const &);
			XzInputStreamBuffer & operator=(XzInputStreamBuffer&);
			
			public:
			XzInputStreamBuffer(std::istream & rstream, ::std::size_t rbuffersize, std::size_t rpushbackspace)
			: stream(rstream), 
			  xzdec(stream),
			  buffersize(rbuffersize),
			  pushbackspace(rpushbackspace),
			  buffer(buffersize+pushbackspace,false), streamreadpos(0)
			{
				setg(buffer.end(), buffer.end(), buffer.end());	
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

				size_t const n = xzdec.read(reinterpret_cast<uint8_t *>(midptr), buffer.end()-midptr);
				streamreadpos += n;

				setg(midptr-copyavail, midptr, midptr+n);

				if (!n)
					return traits_type::eof();
				
				return static_cast<int_type>(*uptr());
			}
		};
	}
}
#endif
