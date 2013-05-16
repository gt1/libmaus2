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
#if ! defined(LIBMAUS_LZ_BGZFINFLATE_HPP)
#define LIBMAUS_LZ_BGZFINFLATE_HPP

#include <zlib.h>
#include <libmaus/autoarray/AutoArray.hpp>

namespace libmaus
{
	namespace lz
	{
		template<typename _stream_type>
		struct BgzfInflate
		{
			typedef _stream_type stream_type;
			static unsigned int const maxblocksize = 64*1024;

			private:
			static unsigned int const headersize = 18;
			
			z_stream strm;
			::libmaus::autoarray::AutoArray<uint8_t,::libmaus::autoarray::alloc_type_memalign_cacheline> header;
			::libmaus::autoarray::AutoArray<uint8_t,::libmaus::autoarray::alloc_type_memalign_cacheline> block;
			stream_type & stream;
			uint64_t gcnt;

			void init()
			{
				memset(&strm,0,sizeof(z_stream));
						
				strm.zalloc = Z_NULL;
				strm.zfree = Z_NULL;
				strm.opaque = Z_NULL;
				strm.avail_in = 0;
				strm.next_in = Z_NULL;
						
				int const ret = inflateInit2(&strm,-15);
							
				if (ret != Z_OK)
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "BgzfInflate::init() failed in inflateInit2";
					se.finish();
					throw se;
				}
			
			}

			public:	
			BgzfInflate(stream_type & rstream)
			: header(headersize,false), block(maxblocksize,false), stream(rstream), gcnt(0)
			{
				init();
			}
			~BgzfInflate()
			{
				inflateEnd(&strm);
			}
			
			uint64_t read(char * const decomp, uint64_t const n)
			{
				gcnt = 0;
				stream.read(reinterpret_cast<char *>(header.begin()),headersize);
				
				if ( stream.gcount() == 0 )
				{
					std::cerr << "gcount is 0" << std::endl;
					return 0;
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
				// read block
				stream.read(reinterpret_cast<char *>(block.begin()),payloadsize + 8);
				
				if ( stream.gcount() != static_cast<int64_t>(payloadsize + 8) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "BgzfInflate::decompressBlock(): unexpected eof";
					se.finish();
					throw se;							
				}
				
				uint32_t const uncompdatasize = 
					(static_cast<uint32_t>(block[payloadsize+4]) << 0)
					|
					(static_cast<uint32_t>(block[payloadsize+5]) << 8)
					|
					(static_cast<uint32_t>(block[payloadsize+6]) << 16)
					|
					(static_cast<uint32_t>(block[payloadsize+7]) << 24);
					
				if ( uncompdatasize > n )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "BgzfInflate::decompressBlock(): uncompressed size is too large";
					se.finish();
					throw se;									
				
				}

				if ( inflateReset(&strm) != Z_OK )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "BgzfInflate::decompressBlock(): inflateReset failed";
					se.finish();
					throw se;									
				}
				
				strm.avail_in = payloadsize;
				strm.next_in = (block.begin());
				strm.avail_out = uncompdatasize;
				strm.next_out = reinterpret_cast<Bytef *>(decomp);
				
				if ( (inflate(&strm,Z_FINISH) != Z_STREAM_END) || (strm.avail_out != 0) || (strm.avail_in != 0) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "BgzfInflate::decompressBlock(): inflate failed";
					se.finish();
					throw se;												
				}
				
				gcnt = uncompdatasize;
				
				return gcnt;
			}
			
			uint64_t gcount() const
			{
				return gcnt;
			}
		};
	}
}
#endif
