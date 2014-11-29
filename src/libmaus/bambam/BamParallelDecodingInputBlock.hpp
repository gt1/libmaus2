/*
    libmaus
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
#if ! defined(LIBMAUS_BAMBAM_BAMPARALLELDECODINGINPUTBLOCK_HPP)
#define LIBMAUS_BAMBAM_BAMPARALLELDECODINGINPUTBLOCK_HPP

#include <libmaus/lz/BgzfInflateHeaderBase.hpp>

namespace libmaus
{
	namespace bambam
	{
		/**
		 * input block for BAM decoding
		 *
		 * An object encapsulates a bgzf compressed block
		 **/
		struct BamParallelDecodingInputBlock
		{
			typedef BamParallelDecodingInputBlock this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			//! input data
			libmaus::lz::BgzfInflateHeaderBase inflateheaderbase;
			//! size of block payload
			uint64_t volatile payloadsize;
			//! compressed data
			libmaus::autoarray::AutoArray<char> C;
			//! size of decompressed data
			uint64_t volatile uncompdatasize;
			//! true if this is the last block in the stream
			bool volatile final;
			//! stream id
			uint64_t volatile streamid;
			//! block id
			uint64_t volatile blockid;
			//! crc32
			uint32_t volatile crc;
			
			BamParallelDecodingInputBlock() 
			: 
				inflateheaderbase(),
				payloadsize(0),
				C(libmaus::lz::BgzfConstants::getBgzfMaxBlockSize(),false), 
				uncompdatasize(0),
				final(false) ,
				streamid(0),
				blockid(0),
				crc(0)
			{}
		
			/**
			 * read a bgzf compressed block from stream
			 **/	
			template<typename stream_type>
			void readBlock(stream_type & stream)
			{
				// read bgzf header
				payloadsize = inflateheaderbase.readHeader(stream);
				
				// read block
				stream.read(C.begin(),payloadsize + 8);

				// check that we have obtained the requested number of bytes
				if ( stream.gcount() != static_cast<int64_t>(payloadsize + 8) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "BamParallelDecodingInputBlock::readBlock(): unexpected eof" << std::endl;
					se.finish(false);
					throw se;
				}
				
				// compose crc
				crc = 
					(static_cast<uint32_t>(static_cast<uint8_t>(C[payloadsize+0])) <<  0) |
					(static_cast<uint32_t>(static_cast<uint8_t>(C[payloadsize+1])) <<  8) |
					(static_cast<uint32_t>(static_cast<uint8_t>(C[payloadsize+2])) << 16) |
					(static_cast<uint32_t>(static_cast<uint8_t>(C[payloadsize+3])) << 24)
				;
					
				// compose size of uncompressed block in bytes			
				uncompdatasize = 
					(static_cast<uint64_t>(static_cast<uint8_t>(C[payloadsize+4])) << 0)
					|
					(static_cast<uint64_t>(static_cast<uint8_t>(C[payloadsize+5])) << 8)
					|
					(static_cast<uint64_t>(static_cast<uint8_t>(C[payloadsize+6])) << 16)
					|
					(static_cast<uint64_t>(static_cast<uint8_t>(C[payloadsize+7])) << 24);

				// check that uncompressed size conforms with bgzf specs
				if ( uncompdatasize > libmaus::lz::BgzfConstants::getBgzfMaxBlockSize() )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "BamParallelDecodingInputBlock::readBlock(): uncompressed size is too large";
					se.finish(false);
					throw se;									
				}
				
				// check whether this is the final block and set the flag accordingly
				final = (uncompdatasize == 0) && (stream.peek() < 0);
			}
		};
	}
}
#endif
