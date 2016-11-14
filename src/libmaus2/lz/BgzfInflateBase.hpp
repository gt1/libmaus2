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
#if ! defined(LIBMAUS2_LZ_BGZFINFLATEBASE_HPP)
#define LIBMAUS2_LZ_BGZFINFLATEBASE_HPP

#include <zlib.h>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/lz/BgzfConstants.hpp>
#include <libmaus2/lz/BgzfInflateInfo.hpp>
#include <libmaus2/lz/BgzfInflateHeaderBase.hpp>
#include <libmaus2/lz/BgzfInflateZStreamBase.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct BgzfInflateBase : public BgzfInflateHeaderBase, BgzfInflateZStreamBase
		{
			typedef BgzfInflateBase this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			static int checkCrc;

			// space for compressed input data
			::libmaus2::autoarray::AutoArray<uint8_t,::libmaus2::autoarray::alloc_type_memalign_pagesize> block;

			BgzfInflateBase()
			: BgzfInflateHeaderBase(), block(getBgzfMaxBlockSize(),false)
			{
			}

			private:
			/**
			 * read bgzf block data plus footer (checksum + uncompressed size)
			 *
			 * @param stream
			 * @param payloadsize length of compressed data
			 * @return length of uncompressed block
			 **/
			template<typename stream_type>
			std::pair<uint64_t,uint64_t> readData(stream_type & stream, uint64_t const payloadsize)
			{
				// read block
				stream.read(reinterpret_cast<char *>(block.begin()),payloadsize + 8);

				if ( stream.gcount() != static_cast<int64_t>(payloadsize + 8) )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "BgzfInflateBase::readData(): unexpected eof" << std::endl;
					se.finish(false);
					throw se;
				}

				#if defined(LIBMAUS2_BYTE_ORDER_LITTLE_ENDIAN) && defined(LIBMAUS2_HAVE_i386)
				uint32_t const checksum = *(reinterpret_cast<uint32_t const *>(block.begin()+payloadsize));
				#else
				uint32_t const checksum =
					(static_cast<uint32_t>(block[payloadsize+0]) << 0)
					|
					(static_cast<uint32_t>(block[payloadsize+1]) << 8)
					|
					(static_cast<uint32_t>(block[payloadsize+2]) << 16)
					|
					(static_cast<uint32_t>(block[payloadsize+3]) << 24);
				#endif

				#if defined(LIBMAUS2_BYTE_ORDER_LITTLE_ENDIAN) && defined(LIBMAUS2_HAVE_i386)
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

				if ( uncompdatasize > getBgzfMaxBlockSize() )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "BgzfInflateBase::readData(): uncompressed size is too large: " << uncompdatasize << " > " << getBgzfMaxBlockSize() << std::endl;
					se.finish(false);
					throw se;

				}

				return std::pair<uint64_t,uint64_t>(uncompdatasize,checksum);
			}

			public:
			struct BaseBlockInfo
			{
				uint64_t payloadsize;
				uint64_t uncompdatasize;
				uint64_t checksum;
				uint64_t compdatasize;

				BaseBlockInfo()
				{

				}

				BaseBlockInfo(
					uint64_t const rpayloadsize,
					uint64_t const runcompdatasize,
					uint64_t const rchecksum,
					uint64_t const rcompdatasize
				) : payloadsize(rpayloadsize), uncompdatasize(runcompdatasize), checksum(rchecksum), compdatasize(rcompdatasize)
				{

				}
			};

			/**
			 * read a bgzf block from stream
			 *
			 * @param stream
			 * @return pair of compressed payload size (gzip block minus header and footer) and uncompressed size
			 **/
			template<typename stream_type>
			BaseBlockInfo readBlock(stream_type & stream)
			{
				/* read block header */
				uint64_t const payloadsize = readHeader(stream);

				/* read block data and footer */
				std::pair<uint64_t,uint64_t> const P = readData(stream,payloadsize);
				uint64_t const uncompdatasize = P.first;
				uint64_t const checksum = P.second;

				/* check consistency */
				if ( (! payloadsize) && (uncompdatasize>0) )
				{
					libmaus2::exception::LibMausException se;
					se.getStream() << "BgzfInflateBase::readBlock: broken BGZF file (block payloadsize is zero but uncompressed data length is not)" << std::endl;
					se.finish();
					throw se;
				}

				return BaseBlockInfo(payloadsize,uncompdatasize,checksum,payloadsize + getBgzfHeaderSize() + getBgzfFooterSize());
			}

			/**
			 * decompress block in buffer to array decomp
			 *
			 * @param decomp space for decompressed data
			 * @param blockinfo pair as returned by readBlock method
			 * @return number of bytes stored in decomp
			 **/
			uint64_t decompressBlock(char * const decomp, BaseBlockInfo const & blockinfo)
			{
				zdecompress(block.begin(),blockinfo.payloadsize,decomp,blockinfo.uncompdatasize);

				if ( checkCrc )
				{
					uint32_t const checksum = computeCrc(reinterpret_cast<uint8_t const *>(decomp),blockinfo.uncompdatasize);
					if ( checksum != blockinfo.checksum )
					{
						libmaus2::exception::LibMausException se;
						se.getStream() << "BgzfInflateBase::decompressBlock: broken BGZF file (CRC mismatch)" << std::endl;
						se.finish();
						throw se;
					}
				}

				return blockinfo.uncompdatasize;
			}
		};
	}
}
#endif
