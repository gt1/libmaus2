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
#if ! defined(LIBMAUS_LZ_BGZFINFLATEHEADERBASE_HPP)
#define LIBMAUS_LZ_BGZFINFLATEHEADERBASE_HPP

#include <zlib.h>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/lz/BgzfConstants.hpp>

namespace libmaus
{
	namespace lz
	{
		struct BgzfInflateHeaderBase : public BgzfConstants
		{
			::libmaus::autoarray::AutoArray<uint8_t,::libmaus::autoarray::alloc_type_memalign_cacheline> header;
		
			BgzfInflateHeaderBase()
			: header(headersize,false)
			{
			}

			template<typename stream_type>
			uint64_t readHeader(stream_type & stream)
			{
				stream.read(reinterpret_cast<char *>(header.begin()),headersize);
				
				/* end of file */
				if ( stream.gcount() == 0 )
					return false;
				
				if ( stream.gcount() != headersize )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "BgzfInflate::decompressBlock(): unexpected EOF while reading header";
					se.finish();
					throw se;						
				}
				
				if ( header[0] != 31 || header[1] != 139 || header[2] != 8 || header[3] != 4 ||
					header[10] != 6 || header[11] != 0 ||
					header[12] != 66 || header[13] != 67 || header[14] != 2 || header[15] != 0
				)
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "BgzfInflate::decompressBlock(): invalid header data";
					se.finish();
					throw se;			
				}
			
				uint64_t const cblocksize = (static_cast<uint32_t>(header[16]) | (static_cast<uint32_t>(header[17]) << 8)) + 1;
				
				if ( cblocksize < 18 + 8 )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "BgzfInflate::decompressBlock(): invalid header data";
					se.finish();
					throw se;					
				}
				
				// size of compressed data
				uint64_t const payloadsize = cblocksize - (18 + 8);

				return payloadsize;
			}
		};
	}
}
#endif
