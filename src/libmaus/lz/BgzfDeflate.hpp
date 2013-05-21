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
#if ! defined(LIBMAUS_LZ_BGZFDEFLATE_HPP)
#define LIBMAUS_LZ_BGZFDEFLATE_HPP

#include <libmaus/lz/GzipHeader.hpp>
#include <libmaus/lz/BgzfDeflateBase.hpp>
#include <zlib.h>

namespace libmaus
{
	namespace lz
	{
		template<typename _stream_type>
		struct BgzfDeflate : public BgzfDeflateBase
		{	
			typedef BgzfDeflateBase base_type;
			typedef _stream_type stream_type;

			stream_type & stream;

			BgzfDeflate(stream_type & rstream, int const level = Z_DEFAULT_COMPRESSION, bool const rflushmode = false)
			: BgzfDeflateBase(level,rflushmode), stream(rstream)
			{
			}

			void streamWrite(uint8_t const * p, uint64_t const n)
			{
				stream.write(reinterpret_cast<char const *>(p),n);

				if ( ! stream )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "failed to write compressed data to bgzf stream." << std::endl;
					se.finish();
					throw se;				
				}		
			}

			void flush()
			{
				uint64_t const outbytes = base_type::flush(flushmode);
				/* write data to stream */
				streamWrite(outbuf.begin(),outbytes);
			}
			
			void write(char const * const cp, unsigned int n)
			{
				uint8_t const * p = reinterpret_cast<uint8_t const *>(cp);
			
				while ( n )
				{
					unsigned int const towrite = std::min(n,static_cast<unsigned int>(pe-pc));
					std::copy(p,p+towrite,pc);

					p += towrite;
					pc += towrite;
					n -= towrite;
					
					if ( pc == pe )
						flush();
				}
			}
			
			void put(uint8_t const c)
			{
				*(pc++) = c;
				if ( pc == pe )
					flush();
			}
			
			void addEOFBlock()
			{
				// flush, if there is any uncompressed data
				if ( pc != pa )
					flush();
				// set compression level to default
				deflatereinit();
				// write empty block
				flush();
			}
		};                                                                                                                                                                                                                                                                                                                                                                                                                                                        
	}
}
#endif
