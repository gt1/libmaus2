/*
    libmaus2
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
#if ! defined(LIBMAUS2_BAMBAM_PARALLEL_DECOMPRESSBLOCKWORKPACKAGEDISPATCHER_HPP)
#define LIBMAUS2_BAMBAM_PARALLEL_DECOMPRESSBLOCKWORKPACKAGEDISPATCHER_HPP

#include <libmaus2/parallel/SimpleThreadWorkPackageDispatcher.hpp>
#include <libmaus2/bambam/parallel/DecompressBlockWorkPackageReturnInterface.hpp>
#include <libmaus2/bambam/parallel/InputBlockReturnInterface.hpp>
#include <libmaus2/bambam/parallel/DecompressedBlockAddPendingInterface.hpp>
#include <libmaus2/bambam/parallel/BgzfInflateZStreamBaseReturnInterface.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			// dispatcher for block decompression
			struct DecompressBlockWorkPackageDispatcher : public libmaus2::parallel::SimpleThreadWorkPackageDispatcher
			{
				DecompressBlockWorkPackageReturnInterface & packageReturnInterface;
				InputBlockReturnInterface & inputBlockReturnInterface;
				DecompressedBlockAddPendingInterface & decompressedBlockPendingInterface;
				BgzfInflateZStreamBaseReturnInterface & decoderReturnInterface;

				DecompressBlockWorkPackageDispatcher(
					DecompressBlockWorkPackageReturnInterface & rpackageReturnInterface,
					InputBlockReturnInterface & rinputBlockReturnInterface,
					DecompressedBlockAddPendingInterface & rdecompressedBlockPendingInterface,
					BgzfInflateZStreamBaseReturnInterface & rdecoderReturnInterface

				) : packageReturnInterface(rpackageReturnInterface), inputBlockReturnInterface(rinputBlockReturnInterface),
				    decompressedBlockPendingInterface(rdecompressedBlockPendingInterface), decoderReturnInterface(rdecoderReturnInterface)
				{

				}

				virtual void dispatch(
					libmaus2::parallel::SimpleThreadWorkPackage * P,
					libmaus2::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & tpi
				)
				{
					DecompressBlockWorkPackage * BP = dynamic_cast<DecompressBlockWorkPackage *>(P);
					assert ( BP );

					// decompress the block
					BP->outputblock->decompressBlock(BP->decoder.get(),BP->inputblock.get());

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

						libmaus2::exception::LibMausException lme;
						lme.getStream() << "DecompressBlockWorkPackageDispatcher: corrupt input data (crc mismatch)\n";
						lme.finish();
						throw lme;
					}

					// set stream and block id
					BP->outputblock->streamid = BP->inputblock->streamid;
					BP->outputblock->blockid = BP->inputblock->blockid;

					// return zstream base (decompressor)
					decoderReturnInterface.putBgzfInflateZStreamBaseReturn(BP->decoder);
					// return input block
					inputBlockReturnInterface.putInputBlockReturn(BP->inputblock);
					// mark output block as pending
					decompressedBlockPendingInterface.putDecompressedBlockAddPending(BP->outputblock);
					// return work meta package
					packageReturnInterface.putDecompressBlockWorkPackage(BP);
				}
			};
		}
	}
}
#endif
