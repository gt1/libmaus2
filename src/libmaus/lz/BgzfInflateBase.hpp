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
#if ! defined(LIBMAUS_LZ_BGZFINFLATEBASE_HPP)
#define LIBMAUS_LZ_BGZFINFLATEBASE_HPP

#include <zlib.h>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/lz/BgzfConstants.hpp>
#include <libmaus/lz/BgzfInflateInfo.hpp>
#include <libmaus/lz/BgzfInflateHeaderBase.hpp>
#include <libmaus/lz/BgzfInflateZStreamBase.hpp>

namespace libmaus
{
	namespace lz
	{
		struct BgzfInflateBase : public BgzfInflateHeaderBase, BgzfInflateZStreamBase
		{
			typedef BgzfInflateBase this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

			// space for compressed input data
			::libmaus::autoarray::AutoArray<uint8_t,::libmaus::autoarray::alloc_type_memalign_cacheline> block;

			BgzfInflateBase()
			: BgzfInflateHeaderBase(), block(getBgzfMaxBlockSize(),false)
			{
			}

			/**
			 * read bgzf block data plus footer (checksum + uncompressed size)
			 *
			 * @param stream
			 * @param payloadsize length of compressed data
			 * @return length of uncompressed block
			 **/
			template<typename stream_type>
			uint64_t readData(stream_type & stream, uint64_t const payloadsize)
			{
				// read block
				stream.read(reinterpret_cast<char *>(block.begin()),payloadsize + 8);

				if ( stream.gcount() != static_cast<int64_t>(payloadsize + 8) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "BgzfInflateBase::readData(): unexpected eof" << std::endl;
					se.finish(false);
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
					
				if ( uncompdatasize > getBgzfMaxBlockSize() )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "BgzfInflateBase::readData(): uncompressed size is too large";
					se.finish(false);
					throw se;									
				
				}
			
				return uncompdatasize;
			}

			/**
			 * read a bgzf block from stream
			 *
			 * @param stream
			 * @return pair of compressed payload size (gzip block minus header and footer) and uncompressed size
			 **/
			template<typename stream_type>
			std::pair<uint64_t,uint64_t> readBlock(stream_type & stream)
			{
				/* read block header */
				uint64_t const payloadsize = readHeader(stream);

				/* read block data and footer */
				uint64_t const uncompdatasize = readData(stream,payloadsize);

				/* check consistency */				
				if ( (! payloadsize) && (uncompdatasize>0) )
				{
					libmaus::exception::LibMausException se;
					se.getStream() << "BgzfInflateBase::readBlock: broken BGZF file (block payloadsize is zero but uncompressed data length is not)" << std::endl;
					se.finish();
					throw se;
				}
				
				return std::pair<uint64_t,uint64_t>(payloadsize,uncompdatasize);
			}
			
			/**
			 * decompress block in buffer to array decomp
			 *
			 * @param decomp space for decompressed data
			 * @param blockinfo pair as returned by readBlock method
			 * @return number of bytes stored in decomp
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
