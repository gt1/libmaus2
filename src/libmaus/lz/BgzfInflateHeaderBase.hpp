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
			typedef BgzfInflateHeaderBase this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			::libmaus::autoarray::AutoArray<uint8_t,::libmaus::autoarray::alloc_type_memalign_cacheline> header;
		
			BgzfInflateHeaderBase()
			: header(getBgzfHeaderSize(),false)
			{
			}
			
			template<typename iterator>
			static bool hasSufficientData(iterator ita, iterator ite)
			{
				if ( ite-ita < static_cast<ptrdiff_t>(getBgzfHeaderSize()) )
					return false;

				uint64_t const cblocksize = (
					(static_cast<uint32_t>(static_cast<uint8_t>(ita[16]))     ) | 
					(static_cast<uint32_t>(static_cast<uint8_t>(ita[17])) << 8)
				) + 1;
				
				return ite-ita >= static_cast<ptrdiff_t>(cblocksize);
			}
			
			
			template<typename iterator>
			static int64_t getBlockSize(iterator ita, iterator ite)
			{
				if ( ! hasSufficientData(ita,ite) )
					return -1;
					
				
				uint64_t const cblocksize = (
					(static_cast<uint32_t>(static_cast<uint8_t>(ita[16]))     ) | 
					(static_cast<uint32_t>(static_cast<uint8_t>(ita[17])) << 8)
				) + 1;
				
				return cblocksize;
			}

			template<typename stream_type>
			uint64_t readHeader(stream_type & stream)
			{
				stream.read(reinterpret_cast<char *>(header.begin()),getBgzfHeaderSize());
				
				/* end of file */
				if ( stream.gcount() == 0 )
					return false;
				
				if ( static_cast<ssize_t>(stream.gcount()) != static_cast<ssize_t>(getBgzfHeaderSize()) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "BgzfInflate::decompressBlock(): unexpected EOF while reading header";
					se.finish(false);
					throw se;						
				}
				
				if ( header[0] != 31 || header[1] != 139 || header[2] != 8 || header[3] != 4 ||
					header[10] != 6 || header[11] != 0 ||
					header[12] != 66 || header[13] != 67 || header[14] != 2 || header[15] != 0
				)
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "BgzfInflate::decompressBlock(): invalid header data";
					se.finish(false);
					throw se;			
				}
			
				uint64_t const cblocksize = (static_cast<uint32_t>(header[16]) | (static_cast<uint32_t>(header[17]) << 8)) + 1;

				if ( cblocksize < getBgzfHeaderSize() + getBgzfFooterSize() )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "BgzfInflate::decompressBlock(): invalid header data";
					se.finish(false);
					throw se;					
				}
				
				// size of compressed data
				uint64_t const payloadsize = cblocksize - (getBgzfHeaderSize() + getBgzfFooterSize());

				return payloadsize;
			}
		};
	}
}
#endif
