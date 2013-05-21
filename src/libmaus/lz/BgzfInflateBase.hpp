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
#if ! defined(LIBMAUS_LZ_BGZFINFLATEBASE_HPP)
#define LIBMAUS_LZ_BGZFINFLATEBASE_HPP

#include <zlib.h>
#include <libmaus/autoarray/AutoArray.hpp>

namespace libmaus
{
	namespace lz
	{
		struct BgzfInflateHeaderBase
		{
			static unsigned int const headersize = 18;
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
		
		struct BgzfInflateZStreamBase
		{
			z_stream inflatestrm;
		
			void zinit()
			{
				memset(&inflatestrm,0,sizeof(z_stream));
						
				inflatestrm.zalloc = Z_NULL;
				inflatestrm.zfree = Z_NULL;
				inflatestrm.opaque = Z_NULL;
				inflatestrm.avail_in = 0;
				inflatestrm.next_in = Z_NULL;
						
				int const ret = inflateInit2(&inflatestrm,-15);
							
				if (ret != Z_OK)
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "BgzfInflate::init() failed in inflateInit2";
					se.finish();
					throw se;
				}
			}
			
			BgzfInflateZStreamBase()
			{
				zinit();
			}

			~BgzfInflateZStreamBase()
			{
				inflateEnd(&inflatestrm);				
			}
			
			void zreset()
			{
				if ( inflateReset(&inflatestrm) != Z_OK )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "BgzfInflate::decompressBlock(): inflateReset failed";
					se.finish();
					throw se;									
				}			
			}
			
			void zdecompress(
				uint8_t       * const in,
				unsigned int const inlen,
				char          * const out,
				unsigned int const outlen
			)
			{
				zreset();

				inflatestrm.avail_in = inlen;
				inflatestrm.next_in = in;
				inflatestrm.avail_out = outlen;
				inflatestrm.next_out = reinterpret_cast<Bytef *>(out);
				
				if ( (inflate(&inflatestrm,Z_FINISH) != Z_STREAM_END) || (inflatestrm.avail_out != 0) || (inflatestrm.avail_in != 0) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "BgzfInflate::decompressBlock(): inflate failed";
					se.finish();
					throw se;												
				}
			}
		};

		struct BgzfInflateBase : public BgzfInflateHeaderBase, BgzfInflateZStreamBase
		{
			static unsigned int const maxblocksize = 64*1024;

			::libmaus::autoarray::AutoArray<uint8_t,::libmaus::autoarray::alloc_type_memalign_cacheline> block;

			BgzfInflateBase()
			: BgzfInflateHeaderBase(), block(maxblocksize,false)
			{
			}

			template<typename stream_type>
			uint64_t readData(stream_type & stream, uint64_t const payloadsize)
			{
				// read block
				stream.read(reinterpret_cast<char *>(block.begin()),payloadsize + 8);

				if ( stream.gcount() != static_cast<int64_t>(payloadsize + 8) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "BgzfInflate::decompressBlock(): unexpected eof";
					se.finish();
					throw se;
				}
				
				#if defined(LIBMAUS_BYTE_ORDER_LITTLE_ENDIAN) && defined(LIBMAUS_HAVE_i386)
				uint32_t const uncompdatasize = *(reinterpret_cast<uint32_t const *>(block.begin()+payloadsize+4));
				#else
				uint32_t const uncompdatasize = 
					(static_cast<uint32_t>(block[payloadsize+4]) << 0)
					|
					(static_cast<uint32_t>(block[payloadsize+5]) << 8)
					|
					(static_cast<uint32_t>(block[payloadsize+6]) << 16)
					|
					(static_cast<uint32_t>(block[payloadsize+7]) << 24);
				#endif
					
				if ( uncompdatasize > maxblocksize )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "BgzfInflate::decompressBlock(): uncompressed size is too large";
					se.finish();
					throw se;									
				
				}
			
				return uncompdatasize;
			}

			template<typename stream_type>
			std::pair<uint64_t,uint64_t> readBlock(stream_type & stream)
			{
				/* read block header */
				uint64_t const payloadsize = readHeader(stream);
				if ( ! payloadsize )
					return std::pair<uint64_t,uint64_t>(0,0);

				/* read block data */
				uint64_t const uncompdatasize = readData(stream,payloadsize);
				
				return std::pair<uint64_t,uint64_t>(payloadsize,uncompdatasize);
			}
			
			/**
			 * decompress block in buffer to array decomp
			 **/
			uint64_t decompressBlock(char * const decomp, std::pair<uint64_t,uint64_t> const & blockinfo)
			{
				zdecompress(block.begin(),blockinfo.first,decomp,blockinfo.second);
				return blockinfo.second;
			}
		};
	}
}
#endif
