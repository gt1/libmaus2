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
#if ! defined(LIBMAUS_BAMBAM_BAMDECOMPRESSIONCONTROL_HPP)
#define LIBMAUS_BAMBAM_BAMDECOMPRESSIONCONTROL_HPP

#include <libmaus/lz/BgzfInflateBase.hpp>
#include <libmaus/lz/BgzfInflateHeaderBase.hpp>
#include <libmaus/parallel/PosixSpinLock.hpp>
#include <libmaus/parallel/PosixMutex.hpp>
#include <libmaus/parallel/LockedFreeList.hpp>
#include <libmaus/util/FreeList.hpp>
#include <libmaus/util/GrowingFreeList.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct BamParallelDecodingDecoderContainer
		{
			typedef BamParallelDecodingDecoderContainer this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			typedef libmaus::lz::BgzfInflateZStreamBase decoder_type;
			
			libmaus::parallel::PosixSpinLock lock;
			uint64_t const numthreads;
			libmaus::util::FreeList<decoder_type> decoders;
			
			BamParallelDecodingDecoderContainer(uint64_t const rnumthreads)
			: lock(), numthreads(rnumthreads), decoders(numthreads)
			{
			
			}
			
			decoder_type * get()
			{
				libmaus::parallel::ScopePosixSpinLock llock(lock);
				decoder_type * decoder = decoders.get();
				return decoder;
			}
			
			void put(decoder_type * decoder)
			{
				libmaus::parallel::ScopePosixSpinLock llock(lock);
				decoders.put(decoder);
			}
		};

		struct BamParallelDecodingInputBlock
		{
			typedef BamParallelDecodingInputBlock this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			//! input data
			libmaus::lz::BgzfInflateHeaderBase inflateheaderbase;
			//! size of block payload
			uint64_t payloadsize;
			//! decompressed data
			libmaus::autoarray::AutoArray<char> C;
			//! size of decompressed data
			uint64_t uncompdatasize;
			//! true if this is the last block in the stream
			bool final;
			
			BamParallelDecodingInputBlock() 
			: 
				inflateheaderbase(),
				payloadsize(0),
				C(libmaus::lz::BgzfConstants::getBgzfMaxBlockSize(),false), 
				uncompdatasize(0),
				final(false) 
			{}
			
			template<typename stream_type>
			void readBlock(stream_type & stream)
			{
				payloadsize = inflateheaderbase.readHeader(stream);
				
				// read block
				stream.read(C.begin(),payloadsize + 8);

				if ( stream.gcount() != static_cast<int64_t>(payloadsize + 8) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "BamParallelDecodingInputBlock::readBlock(): unexpected eof" << std::endl;
					se.finish(false);
					throw se;
				}
				
				uncompdatasize = 
					(static_cast<uint64_t>(static_cast<uint8_t>(C[payloadsize+4])) << 0)
					|
					(static_cast<uint64_t>(static_cast<uint8_t>(C[payloadsize+5])) << 8)
					|
					(static_cast<uint64_t>(static_cast<uint8_t>(C[payloadsize+6])) << 16)
					|
					(static_cast<uint64_t>(static_cast<uint8_t>(C[payloadsize+7])) << 24);

				if ( uncompdatasize > libmaus::lz::BgzfConstants::getBgzfMaxBlockSize() )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "BamParallelDecodingInputBlock::readBlock(): uncompressed size is too large";
					se.finish(false);
					throw se;									
				}
				
				final = (uncompdatasize == 0) && (stream.peek() < 0);
			}
		};
		
		struct BamParallelDecodingDecompressedBlock
		{
			typedef BamParallelDecodingDecompressedBlock this_type;
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
			
			BamParallelDecodingDecompressedBlock() 
			: 
				D(libmaus::lz::BgzfConstants::getBgzfMaxBlockSize(),false), 
				uncompdatasize(0), 
				P(0),
				final(false)
			{}
			
			uint64_t decompressBlock(
				BamParallelDecodingDecoderContainer & deccont,
				char * in,
				unsigned int const inlen,
				unsigned int const outlen
			)
			{
				BamParallelDecodingDecoderContainer::decoder_type * decoder = deccont.get();				
				decoder->zdecompress(reinterpret_cast<uint8_t *>(in),inlen,D.begin(),outlen);
				deccont.put(decoder);
				
				return outlen;
			}
			
			uint64_t decompressBlock(
				BamParallelDecodingDecoderContainer & deccont,
				BamParallelDecodingInputBlock & inblock
			)
			{
				uncompdatasize = inblock.uncompdatasize;
				final = inblock.final;
				P = D.begin();
				return decompressBlock(deccont,inblock.C.begin(),inblock.payloadsize,uncompdatasize);
			}
		};
					
		struct BamParallelDecodingControl
		{
			typedef BamParallelDecodingControl this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

			typedef libmaus::bambam::BamParallelDecodingInputBlock input_block_type;
			typedef libmaus::bambam::BamParallelDecodingDecompressedBlock decompressed_block_type;

			uint64_t numthreads;
			
			BamParallelDecodingDecoderContainer::unique_ptr_type Pdecoders;
			BamParallelDecodingDecoderContainer & decoders;
			
			libmaus::parallel::LockedFreeList<input_block_type> inputBlockFreeList;			
			libmaus::parallel::LockedFreeList<decompressed_block_type> decompressedBlockFreeList;
			
			libmaus::parallel::PosixSpinLock readLock;

			bool eof;
			
			BamParallelDecodingControl(
				uint64_t rnumthreads
			)
			: 
				numthreads(rnumthreads), 
				Pdecoders(new BamParallelDecodingDecoderContainer(numthreads)), 
				decoders(*Pdecoders),
				inputBlockFreeList(numthreads), 
				decompressedBlockFreeList(numthreads),
				readLock(),
				eof(false)
			{
			
			}
			
			void serialTestDecode(std::istream & in, std::ostream & out)
			{
				bool running = true;
				
				while ( running )
				{
					input_block_type * inblock = inputBlockFreeList.get();
					
					inblock->readBlock(in);
					
					decompressed_block_type * outblock = decompressedBlockFreeList.get();
										
					outblock->decompressBlock(decoders,*inblock);

					running = running && (!(outblock->final));
										
					out.write(outblock->P,outblock->uncompdatasize);
					
					decompressedBlockFreeList.put(outblock);
					inputBlockFreeList.put(inblock);	
				}
			}
			
			static void serialTestDecode1(std::istream & in, std::ostream & out)
			{
				BamParallelDecodingControl BPDC(1);
				BPDC.serialTestDecode(in,out);
			}
		};	
	}
}
#endif
