/*
    libmaus2
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS_FASTX_FASTASTREAMWRAPPER_HPP)
#define LIBMAUS_FASTX_FASTASTREAMWRAPPER_HPP

#include <streambuf>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/fastx/FastALineParser.hpp>

namespace libmaus2
{
	namespace fastx
	{
		struct FastAStreamWrapper : public ::std::streambuf
		{
			private:
			::libmaus2::fastx::FastALineParser & parser;
			uint64_t const buffersize;
			uint64_t const pushbackspace;
			::libmaus2::autoarray::AutoArray<char> buffer;
			uint64_t streamreadpos;

			FastAStreamWrapper(FastAStreamWrapper const &);
			FastAStreamWrapper & operator=(FastAStreamWrapper &);
			
			public:
			FastAStreamWrapper(::libmaus2::fastx::FastALineParser & rparser, ::std::size_t rbuffersize, std::size_t rpushbackspace)
			: parser(rparser), 
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
				if ( copyavail )
					::std::memmove(midptr-copyavail,gptr()-copyavail,copyavail);
				
				char * inptr = midptr;
				uint64_t readavail = buffer.end()-midptr;
				uint64_t n = 0;
				::libmaus2::fastx::FastALineParserLineInfo info;
				bool running = true;

				while ( running )
				{
					bool const lineok = parser.getNextLine(info);
					
					// no more lines?
					if ( ! lineok )
					{
						running = false;
					}
					else
					{
						switch ( info.linetype )
						{
							case ::libmaus2::fastx::FastALineParserLineInfo::libmaus2_fastx_fasta_id_line:
							case ::libmaus2::fastx::FastALineParserLineInfo::libmaus2_fastx_fasta_id_line_eof:
								parser.putback(info);
								running = false;
								break;
							case ::libmaus2::fastx::FastALineParserLineInfo::libmaus2_fastx_fasta_base_line:
							{
								// amount of data to be copied
								uint64_t const linecopy = std::min(info.linelen,readavail);

								// copy data
								memcpy(inptr,info.line,linecopy);
								
								// adjust stream info
								inptr += linecopy;
								readavail -= linecopy;
								n += linecopy;
								
								// adjust line info
								info.line += linecopy;
								info.linelen -= linecopy;
								
								// put back rest if any
								if ( info.linelen )
									parser.putback(info);
									
								// quit loop if space is now full
								if ( ! readavail )
									running = false;
									
								break;
							}
						}
					}
				}

				setg(midptr-copyavail, midptr, midptr+n);
				streamreadpos += n;

				if (!n)
					return traits_type::eof();
				
				return static_cast<int_type>(*uptr());
			}
		};
	}
}
#endif
