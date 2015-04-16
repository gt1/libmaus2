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
#if ! defined(LIBMAUS_LZ_BGZFSTREAMWRAPPER_HPP)
#define LIBMAUS_LZ_BGZFSTREAMWRAPPER_HPP

#include <streambuf>
#include <libmaus/autoarray/AutoArray.hpp>

namespace libmaus
{
	namespace lz
	{
		template<typename stream_type>
		struct BgzfStreamWrapperBuffer : public ::std::streambuf
		{
			private:
			stream_type & stream;
			uint64_t const buffersize;
			uint64_t const pushbackspace;
			::libmaus::autoarray::AutoArray<char> buffer;
			uint64_t streamreadpos;

			BgzfStreamWrapperBuffer(BgzfStreamWrapperBuffer const &);
			BgzfStreamWrapperBuffer & operator=(BgzfStreamWrapperBuffer&);
			
			public:
			BgzfStreamWrapperBuffer(stream_type & rstream, ::std::size_t rbuffersize, std::size_t rpushbackspace)
			: stream(rstream), 
			  buffersize(rbuffersize),
			  pushbackspace(rpushbackspace),
			  buffer(buffersize+pushbackspace,false), streamreadpos(0)
			{
				setg(buffer.end(), buffer.end(), buffer.end());	
			}
			
			uint64_t tellg() const
			{
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
				
				size_t n = 0;
				bool done = false;
				
				while ( ! done )
				{
					BgzfInflateInfo const info = stream.readAndInfo(midptr, buffer.end()-midptr);						
					n = info.uncompressed;
					
					// non eof block
					if ( n )
					{
						streamreadpos += n;
						done = true;			
					}
					else
					{
						// eof block at end of stream
						if ( info.streameof )
						{
							done = true;
						}
						// intermediate empty block, skip it
						else
						{
						
						}
					}
				}

				setg(midptr-copyavail, midptr, midptr+n);

				if (!n)
					return traits_type::eof();
									
				return static_cast<int_type>(*uptr());
			}
		};

		template<typename stream_type>
		struct BgzfStreamWrapper : public BgzfStreamWrapperBuffer<stream_type>, public ::std::istream
		{
			BgzfStreamWrapper(stream_type & stream, uint64_t const buffersize, uint64_t const pushbackspace)
			: BgzfStreamWrapperBuffer<stream_type>(stream,buffersize,pushbackspace), ::std::istream(this)
			{
				
			}
			uint64_t tellg() const
			{
				return BgzfStreamWrapperBuffer<stream_type>::tellg();
			}
		};
	}
}
#endif
