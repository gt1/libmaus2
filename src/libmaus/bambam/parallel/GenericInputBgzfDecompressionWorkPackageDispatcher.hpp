/*
    libmaus
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_GENERICINPUTBGZFDECOMPRESSIONWORKPACKAGEDISPATCHER_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_GENERICINPUTBGZFDECOMPRESSIONWORKPACKAGEDISPATCHER_HPP

#include <libmaus/bambam/parallel/GenericInputBgzfDecompressionWorkPackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/GenericInputBgzfDecompressionWorkPackageMemInputBlockReturnInterface.hpp>
#include <libmaus/bambam/parallel/GenericInputBgzfDecompressionWorkPackageDecompressedBlockReturnInterface.hpp>
#include <libmaus/bambam/parallel/GenericInputBgzfDecompressionWorkSubBlockDecompressionFinishedInterface.hpp>
#include <libmaus/parallel/LockedFreeList.hpp>
#include <libmaus/lz/BgzfInflateZStreamBase.hpp>
#include <libmaus/parallel/SimpleThreadWorkPackageDispatcher.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			struct GenericInputBgzfDecompressionWorkPackageDispatcher : public libmaus::parallel::SimpleThreadWorkPackageDispatcher
			{
				typedef GenericInputBgzfDecompressionWorkPackageDispatcher this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
				
				GenericInputBgzfDecompressionWorkPackageReturnInterface & packageReturnInterface;
				GenericInputBgzfDecompressionWorkPackageMemInputBlockReturnInterface & genericInputBgzfDecompressionWorkPackageMemInputBlockReturn;
				GenericInputBgzfDecompressionWorkPackageDecompressedBlockReturnInterface & genericInputBgzfDecompressionWorkPackageDecompressedBlockReturn;
				GenericInputBgzfDecompressionWorkSubBlockDecompressionFinishedInterface & genericInputBgzfDecompressionWorkSubBlockDecompressionFinished;
			
				libmaus::parallel::LockedFreeList<
					libmaus::lz::BgzfInflateZStreamBase,
					libmaus::lz::BgzfInflateZStreamBaseAllocator,
					libmaus::lz::BgzfInflateZStreamBaseTypeInfo
				> & deccont;
			
			
				GenericInputBgzfDecompressionWorkPackageDispatcher(
					GenericInputBgzfDecompressionWorkPackageReturnInterface & rpackageReturnInterface,
					GenericInputBgzfDecompressionWorkPackageMemInputBlockReturnInterface & rgenericInputBgzfDecompressionWorkPackageMemInputBlockReturn,
					GenericInputBgzfDecompressionWorkPackageDecompressedBlockReturnInterface & rgenericInputBgzfDecompressionWorkPackageDecompressedBlockReturn,
					GenericInputBgzfDecompressionWorkSubBlockDecompressionFinishedInterface & rgenericInputBgzfDecompressionWorkSubBlockDecompressionFinished,
					libmaus::parallel::LockedFreeList<
						libmaus::lz::BgzfInflateZStreamBase,
						libmaus::lz::BgzfInflateZStreamBaseAllocator,
						libmaus::lz::BgzfInflateZStreamBaseTypeInfo
					> & rdeccont
				)
				: packageReturnInterface(rpackageReturnInterface),
				  genericInputBgzfDecompressionWorkPackageMemInputBlockReturn(rgenericInputBgzfDecompressionWorkPackageMemInputBlockReturn),
				  genericInputBgzfDecompressionWorkPackageDecompressedBlockReturn(rgenericInputBgzfDecompressionWorkPackageDecompressedBlockReturn),
				  genericInputBgzfDecompressionWorkSubBlockDecompressionFinished(rgenericInputBgzfDecompressionWorkSubBlockDecompressionFinished),
				  deccont(rdeccont)
				{
				
				}
			
				void dispatch(libmaus::parallel::SimpleThreadWorkPackage * P, libmaus::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */)
				{
					assert ( dynamic_cast<GenericInputBgzfDecompressionWorkPackage *>(P) != 0 );
					GenericInputBgzfDecompressionWorkPackage * BP = dynamic_cast<GenericInputBgzfDecompressionWorkPackage *>(P);
			
					GenericInputControlSubBlockPending & data = BP->data;
					
					GenericInputBase::block_type::shared_ptr_type block = data.block;
					uint64_t const subblockid = data.subblockid;
			
					// parse header data
					data.mib->readBlock(
						block->meta.blocks[subblockid].first,
						block->meta.eof && ((subblockid+1) == block->meta.blocks.size())
					);
					// decompress block
					data.db->decompressBlock(deccont,*(data.mib));
					// set block id
					data.db->blockid = data.mib->blockid;
					// set stream id
					data.db->streamid = data.mib->streamid;
					// compute crc32
					uint32_t const crc = data.db->computeCrc();
							
					if ( crc != data.mib->crc )
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "CRC mismatch." << std::endl;
						lme.finish();
						throw lme;
					}
			
					// return decompressed block		
					genericInputBgzfDecompressionWorkPackageDecompressedBlockReturn.genericInputBgzfDecompressionWorkPackageDecompressedBlockReturn(block->meta.streamid,data.db);		
					// return compressed block meta info
					genericInputBgzfDecompressionWorkPackageMemInputBlockReturn.genericInputBgzfDecompressionWorkPackageMemInputBlockReturn(block->meta.streamid,data.mib);
					// return input block
					genericInputBgzfDecompressionWorkSubBlockDecompressionFinished.genericInputBgzfDecompressionWorkSubBlockDecompressionFinished(block,subblockid);	
					// return work package
					packageReturnInterface.genericInputBgzfDecompressionWorkPackageReturn(BP);
				}
			};
		}
	}
}
#endif
