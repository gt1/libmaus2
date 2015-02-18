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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_DECOMPRESSEDBLOCK_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_DECOMPRESSEDBLOCK_HPP

#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/bambam/parallel/InputBlock.hpp>
#include <libmaus/bambam/parallel/MemInputBlock.hpp>
#include <libmaus/lz/BgzfInflateZStreamBase.hpp>
#include <libmaus/lz/BgzfInflateZStreamBaseAllocator.hpp>
#include <libmaus/lz/BgzfInflateZStreamBaseTypeInfo.hpp>
#include <libmaus/parallel/LockedFreeList.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			/**
			 * class representing a decompressed bam block
			 **/
			struct DecompressedBlock
			{
				typedef DecompressedBlock this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
	
				//! decompressed data
				libmaus::autoarray::AutoArray<char> D;
				//! size of uncompressed data
				uint64_t uncompdatasize;
				//! next byte pointer
				char const * P;
				//! true if this is the last block in the stream
				bool final;
				//! stream id
				uint64_t streamid;
				//! block id
				uint64_t blockid;
				
				DecompressedBlock() 
				: 
					D(libmaus::lz::BgzfConstants::getBgzfMaxBlockSize(),false), 
					uncompdatasize(0), 
					P(0),
					final(false),
					streamid(0),
					blockid(0)
				{}
				
				/**
				 * computes and returns the crc32 of the block
				 **/
				uint32_t computeCrc() const
				{
					uint32_t crc = crc32(0L,Z_NULL,0);
					crc = crc32(crc,reinterpret_cast<Bytef const*>(P),uncompdatasize);
					return crc;
				}
	
				uint64_t decompressBlock(
					libmaus::lz::BgzfInflateZStreamBase * decoder,
					char * in,
					unsigned int const inlen,
					unsigned int const outlen,
					unsigned int const outoff = 0
				)
				{
					assert ( outoff + outlen <= D.size() );
					decoder->zdecompress(reinterpret_cast<uint8_t *>(in),inlen,D.begin()+outoff,outlen);
					return outlen;
				}

				uint64_t decompressBlock(
					libmaus::lz::BgzfInflateZStreamBase * decoder,
					uint8_t * in,
					unsigned int const inlen,
					unsigned int const outlen,
					unsigned int const outoff = 0
				)
				{
					assert ( outoff + outlen <= D.size() );
					decoder->zdecompress(in,inlen,D.begin()+outoff,outlen);
					return outlen;
				}
	
				uint64_t decompressBlock(
					libmaus::lz::BgzfInflateZStreamBase * decoder,
					InputBlock * inblock,
					unsigned int const outoff = 0
				)
				{
					uncompdatasize = inblock->uncompdatasize;
					final = inblock->final;
					P = D.begin();
					return decompressBlock(decoder,inblock->C.begin(),inblock->payloadsize,uncompdatasize,outoff);
				}

				uint64_t decompressBlock(
					libmaus::lz::BgzfInflateZStreamBase * decoder,
					MemInputBlock * inblock,
					unsigned int const outoff = 0
				)
				{
					uncompdatasize = inblock->uncompdatasize;
					final = inblock->final;
					P = D.begin();
					return decompressBlock(decoder,inblock->C,inblock->payloadsize,uncompdatasize,outoff);
				}
	
				uint64_t decompressBlock(
					libmaus::parallel::LockedFreeList<
						libmaus::lz::BgzfInflateZStreamBase,
						libmaus::lz::BgzfInflateZStreamBaseAllocator,
						libmaus::lz::BgzfInflateZStreamBaseTypeInfo
						> & deccont,
					char * in,
					unsigned int const inlen,
					unsigned int const outlen,
					unsigned int const outoff = 0
				)
				{
					libmaus::lz::BgzfInflateZStreamBase::shared_ptr_type decoder = deccont.get();				
					uint64_t const r = decompressBlock(decoder.get(),in,inlen,outlen,outoff);
					deccont.put(decoder);
					return r;
				}

				uint64_t decompressBlock(
					libmaus::parallel::LockedFreeList<
						libmaus::lz::BgzfInflateZStreamBase,
						libmaus::lz::BgzfInflateZStreamBaseAllocator,
						libmaus::lz::BgzfInflateZStreamBaseTypeInfo
						> & deccont,
					uint8_t * in,
					unsigned int const inlen,
					unsigned int const outlen,
					unsigned int const outoff = 0
				)
				{
					libmaus::lz::BgzfInflateZStreamBase::shared_ptr_type decoder = deccont.get();				
					uint64_t const r = decompressBlock(decoder.get(),in,inlen,outlen,outoff);
					deccont.put(decoder);
					return r;
				}
				
				uint64_t decompressBlock(
					libmaus::parallel::LockedFreeList<
						libmaus::lz::BgzfInflateZStreamBase,
						libmaus::lz::BgzfInflateZStreamBaseAllocator,
						libmaus::lz::BgzfInflateZStreamBaseTypeInfo
						> & deccont,
					InputBlock & inblock
				)
				{
					uncompdatasize = inblock.uncompdatasize;
					final = inblock.final;
					P = D.begin();
					return decompressBlock(deccont,inblock.C.begin(),inblock.payloadsize,uncompdatasize);
				}

				uint64_t decompressBlock(
					libmaus::parallel::LockedFreeList<
						libmaus::lz::BgzfInflateZStreamBase,
						libmaus::lz::BgzfInflateZStreamBaseAllocator,
						libmaus::lz::BgzfInflateZStreamBaseTypeInfo
						> & deccont,
					MemInputBlock & inblock
				)
				{
					uncompdatasize = inblock.uncompdatasize;
					final = inblock.final;
					P = D.begin();
					return decompressBlock(deccont,inblock.C,inblock.payloadsize,uncompdatasize);
				}
			};
		}
	}
}
#endif
