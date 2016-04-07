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
#if ! defined(LIBMAUS2_LZ_BGZFHEADERFUNCTIONS_HPP)
#define LIBMAUS2_LZ_BGZFHEADERFUNCTIONS_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/lz/BgzfConstants.hpp>
#include <libmaus2/lz/GzipHeader.hpp>
#include <libmaus2/lz/ZlibInterface.hpp>
#include <libmaus2/exception/LibMausException.hpp>
#include <cstring>
#include <zlib.h>

namespace libmaus2
{
	namespace lz
	{
		struct BgzfDeflateHeaderFunctions : public BgzfConstants
		{
			private:
			// maximum space used for compressed version of a block of size blocksize
			static uint64_t getFlushBound(unsigned int const blocksize, int const level)
			{
				if ( level >= Z_DEFAULT_COMPRESSION && level <= Z_BEST_COMPRESSION )
				{
					LocalDeflateInfo LDI(level);
					return LDI.zintf->z_deflateBound(blocksize);
				}
				else
				{
					return std::max(blocksize,getBgzfMaxBlockSize()) << 1;
				}
			}

			static uint64_t getReqBufSpace(int const level)
			{
				return getBgzfHeaderSize() + getBgzfFooterSize() + getFlushBound(getBgzfMaxBlockSize(),level);
			}

			public:
			static void deflateinitz(libmaus2::lz::ZlibInterface * p, int const level)
			{
				p->eraseContext();

				p->setZAlloc(Z_NULL);
				p->setZFree(Z_NULL);
				p->setOpaque(Z_NULL);
				int ret = p->z_deflateInit2(level, Z_DEFLATED, -15 /* window size */,
					8 /* mem level, gzip default */, Z_DEFAULT_STRATEGY);
				if ( ret != Z_OK )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "deflateInit2 failed." << std::endl;
					se.finish();
					throw se;
				}
			}

			static void deflatedestroyz(libmaus2::lz::ZlibInterface * p)
			{
				p->z_deflateEnd();
			}

			struct LocalDeflateInfo
			{
				libmaus2::lz::ZlibInterface::unique_ptr_type zintf;

				LocalDeflateInfo(int const level)
				: zintf(libmaus2::lz::ZlibInterface::construct())
				{
					deflateinitz(zintf.get(),level);
				}
				~LocalDeflateInfo()
				{
					deflatedestroyz(zintf.get());
				}
			};

			static uint64_t getReqBufSpaceTwo(int const level)
			{
				uint64_t const halfblocksize = (getBgzfMaxBlockSize()+1)/2;
				uint64_t const singleblocksize = getBgzfHeaderSize() + getBgzfFooterSize() + getFlushBound(halfblocksize,level);
				uint64_t const doubleblocksize = 2 * singleblocksize;
				return doubleblocksize;
			}

			static void setupHeader(uint8_t * const outbuf)
			{
				outbuf[0] = ::libmaus2::lz::GzipHeader::ID1;
				outbuf[1] = ::libmaus2::lz::GzipHeader::ID2;
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
				libmaus2::lz::ZlibInterface * zintf,
				uint8_t const * const pa,
				uint8_t * const outbuf,
				unsigned int const payloadsize,
				unsigned int const uncompsize
			)
			{
				// set size of compressed block in header
				unsigned int const blocksize = getBgzfHeaderSize()/*header*/+8/*footer*/+payloadsize-1;
				assert ( blocksize < getBgzfMaxBlockSize() );
				outbuf[16] = (blocksize >> 0) & 0xFF;
				outbuf[17] = (blocksize >> 8) & 0xFF;

				uint8_t * footptr = outbuf + getBgzfHeaderSize() + payloadsize;

				// compute crc of uncompressed data
				uint32_t crc = zintf->z_crc32(0,0,0);
				crc = zintf->z_crc32(crc, reinterpret_cast<Bytef const *>(pa), uncompsize);

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
				return std::max(static_cast<uint64_t>(getBgzfMaxBlockSize()),getReqBufSpaceTwo(level));
			}
		};
	}
}
#endif
