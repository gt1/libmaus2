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
#if ! defined(LIBMAUS_LZ_BGZFHEADERFUNCTIONS_HPP)
#define LIBMAUS_LZ_BGZFHEADERFUNCTIONS_HPP

#include <libmaus/types/types.hpp>
#include <libmaus/lz/BgzfConstants.hpp>
#include <libmaus/lz/GzipHeader.hpp>
#include <libmaus/exception/LibMausException.hpp>
#include <cstring>
#include <zlib.h>

namespace libmaus
{
	namespace lz
	{
		struct BgzfDeflateHeaderFunctions : public BgzfConstants
		{
			static void deflateinitz(z_stream * strm, int const level)
			{
				memset ( strm , 0, sizeof(z_stream) );
				strm->zalloc = Z_NULL;
				strm->zfree = Z_NULL; 
				strm->opaque = Z_NULL;
				int ret = deflateInit2(strm, level, Z_DEFLATED, -15 /* window size */,
					8 /* mem level, gzip default */, Z_DEFAULT_STRATEGY);
				if ( ret != Z_OK )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "deflateInit2 failed." << std::endl;
					se.finish();
					throw se;
				}
			}

			static void deflatedestroyz(z_stream * strm)
			{
				deflateEnd(strm);		
			}
			
			struct LocalDeflateInfo
			{
				z_stream strm;
				
				LocalDeflateInfo(int const level)
				{
					deflateinitz(&strm,level);
				}
				~LocalDeflateInfo()
				{
					deflatedestroyz(&strm);
				}
			};
			
			static uint64_t getFlushBound(unsigned int const blocksize, int const level)
			{
				LocalDeflateInfo LDI(level);
				return deflateBound(&(LDI.strm),blocksize);
			}
			
			static uint64_t getReqBufSpace(int const level)
			{
				return headersize + footersize + getFlushBound(maxblocksize,level);
			}

			static uint64_t getReqBufSpaceTwo(int const level)
			{
				uint64_t const halfblocksize = (maxblocksize+1)/2;
				uint64_t const singleblocksize = headersize + footersize + getFlushBound(halfblocksize,level);
				uint64_t const doubleblocksize = 2 * singleblocksize;
				return doubleblocksize;
			}
			
			static void setupHeader(uint8_t * const outbuf)
			{
				outbuf[0] = ::libmaus::lz::GzipHeader::ID1;
				outbuf[1] = ::libmaus::lz::GzipHeader::ID2;
				outbuf[2] = 8; // CM
				outbuf[3] = 4; // FLG, extra data
				outbuf[4] = outbuf[5] = outbuf[6] = outbuf[7] = 0; // MTIME
				outbuf[8] = 0; // XFL
				outbuf[9] = 255; // undefined OS
				outbuf[10] = 6; outbuf[11] = 0; // xlen=6
				outbuf[12] = 'B';
				outbuf[13] = 'C';
				outbuf[14] = 2; outbuf[15] = 0; // length of field
				outbuf[16] = outbuf[17] = 0; // block length, to be filled later
			}

			static uint8_t const * fillHeaderFooter(
				uint8_t const * const pa,
				uint8_t * const outbuf,
				unsigned int const payloadsize, 
				unsigned int const uncompsize
			)
			{
				// set size of compressed block in header
				unsigned int const blocksize = headersize/*header*/+8/*footer*/+payloadsize-1;
				assert ( blocksize < 64*1024 );
				outbuf[16] = (blocksize >> 0) & 0xFF;
				outbuf[17] = (blocksize >> 8) & 0xFF;
				
				uint8_t * footptr = outbuf + headersize + payloadsize;

				// compute crc of uncompressed data
				uint32_t crc = crc32(0,0,0);
				crc = crc32(crc, reinterpret_cast<Bytef const *>(pa), uncompsize);
				
				// crc
				*(footptr++) = (crc >> 0)  & 0xFF;
				*(footptr++) = (crc >> 8)  & 0xFF;
				*(footptr++) = (crc >> 16) & 0xFF;
				*(footptr++) = (crc >> 24) & 0xFF;
				// uncompressed size
				*(footptr++) = (uncompsize >> 0) & 0xFF;
				*(footptr++) = (uncompsize >> 8) & 0xFF;
				*(footptr++) = (uncompsize >> 16) & 0xFF;
				*(footptr++) = (uncompsize >> 24) & 0xFF;
				
				return footptr;
			}

			static uint64_t getOutBufSizeTwo(int const level)
			{
				return std::max(static_cast<uint64_t>(maxblocksize),getReqBufSpaceTwo(level));
			}
		};
	}
}
#endif
