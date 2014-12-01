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
#if ! defined(LIBMAUS_BAMBAM_BAMPARALLELDECODINGDECOMPRESSBLOCKWORKPACKAGEDISPATCHER_HPP)
#define LIBMAUS_BAMBAM_BAMPARALLELDECODINGDECOMPRESSBLOCKWORKPACKAGEDISPATCHER_HPP

#include <libmaus/parallel/SimpleThreadWorkPackageDispatcher.hpp>
#include <libmaus/bambam/BamParallelDecodingDecompressBlockWorkPackageReturnInterface.hpp>
#include <libmaus/bambam/BamParallelDecodingInputBlockReturnInterface.hpp>
#include <libmaus/bambam/BamParallelDecodingDecompressedBlockAddPendingInterface.hpp>
#include <libmaus/bambam/BamParallelDecodingBgzfInflateZStreamBaseReturnInterface.hpp>

namespace libmaus
{
	namespace bambam
	{
		// dispatcher for block decompression
		struct BamParallelDecodingDecompressBlockWorkPackageDispatcher : public libmaus::parallel::SimpleThreadWorkPackageDispatcher
		{
			BamParallelDecodingDecompressBlockWorkPackageReturnInterface & packageReturnInterface;
			BamParallelDecodingInputBlockReturnInterface & inputBlockReturnInterface;
			BamParallelDecodingDecompressedBlockAddPendingInterface & decompressedBlockPendingInterface;
			BamParallelDecodingBgzfInflateZStreamBaseReturnInterface & decoderReturnInterface;

			BamParallelDecodingDecompressBlockWorkPackageDispatcher(
				BamParallelDecodingDecompressBlockWorkPackageReturnInterface & rpackageReturnInterface,
				BamParallelDecodingInputBlockReturnInterface & rinputBlockReturnInterface,
				BamParallelDecodingDecompressedBlockAddPendingInterface & rdecompressedBlockPendingInterface,
				BamParallelDecodingBgzfInflateZStreamBaseReturnInterface & rdecoderReturnInterface

			) : packageReturnInterface(rpackageReturnInterface), inputBlockReturnInterface(rinputBlockReturnInterface),
			    decompressedBlockPendingInterface(rdecompressedBlockPendingInterface), decoderReturnInterface(rdecoderReturnInterface)
			{
			
			}
		
			virtual void dispatch(
				libmaus::parallel::SimpleThreadWorkPackage * P, 
				libmaus::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & tpi
			)
			{
				BamParallelDecodingDecompressBlockWorkPackage * BP = dynamic_cast<BamParallelDecodingDecompressBlockWorkPackage *>(P);
				assert ( BP );
				
				// decompress the block
				BP->outputblock->decompressBlock(BP->decoder,BP->inputblock);

				// compute crc of uncompressed data
				uint32_t const crc = BP->outputblock->computeCrc();

				// check crc
				if ( crc != BP->inputblock->crc )
				{
					tpi.getGlobalLock().lock();
					std::cerr << "crc failed for block " << BP->inputblock->blockid 
						<< " expecting " << std::hex << BP->inputblock->crc << std::dec
						<< " got " << std::hex << crc << std::dec << std::endl;
					tpi.getGlobalLock().unlock();
					
					libmaus::exception::LibMausException lme;
					lme.getStream() << "BamParallelDecodingDecompressBlockWorkPackageDispatcher: corrupt input data (crc mismatch)\n";
					lme.finish();
					throw lme;
				}
				
				// set stream and block id
				BP->outputblock->streamid = BP->inputblock->streamid;
				BP->outputblock->blockid = BP->inputblock->blockid;

				// return zstream base (decompressor)
				decoderReturnInterface.putBamParallelDecodingBgzfInflateZStreamBaseReturn(BP->decoder);
				// return input block
				inputBlockReturnInterface.putBamParallelDecodingInputBlockReturn(BP->inputblock);
				// mark output block as pending
				decompressedBlockPendingInterface.putBamParallelDecodingDecompressedBlockAddPending(BP->outputblock);
				// return work meta package
				packageReturnInterface.putBamParallelDecodingDecompressBlockWorkPackage(BP);
			}		
		};
	}
}
#endif
