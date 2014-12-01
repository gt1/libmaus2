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
#if ! defined(LIBMAUS_BAMBAM_BAMPARALLELDECODINGPARSEBLOCKWORKPACKAGEDISPATCHER_HPP)
#define LIBMAUS_BAMBAM_BAMPARALLELDECODINGPARSEBLOCKWORKPACKAGEDISPATCHER_HPP

#include <libmaus/bambam/BamParallelDecodingDecompressedBlockAddPendingInterface.hpp>
#include <libmaus/bambam/BamParallelDecodingDecompressedBlockReturnInterface.hpp>
#include <libmaus/bambam/BamParallelDecodingParsedBlockAddPendingInterface.hpp>
#include <libmaus/bambam/BamParallelDecodingParsedBlockStallInterface.hpp>
#include <libmaus/bambam/BamParallelDecodingParsePackageReturnInterface.hpp>
#include <libmaus/parallel/SimpleThreadWorkPackageDispatcher.hpp>

namespace libmaus
{
	namespace bambam
	{
		// dispatcher for block parsing
		struct BamParallelDecodingParseBlockWorkPackageDispatcher : public libmaus::parallel::SimpleThreadWorkPackageDispatcher
		{
			BamParallelDecodingDecompressedBlockAddPendingInterface & addDecompressedPendingInterface;
			BamParallelDecodingDecompressedBlockReturnInterface     & returnDecompressedInterface;
			BamParallelDecodingParsedBlockAddPendingInterface       & addParsedPendingInterface;
			BamParallelDecodingParsedBlockStallInterface            & parseStallInterface;
			BamParallelDecodingParsePackageReturnInterface          & packageReturnInterface;

			BamParallelDecodingParseBlockWorkPackageDispatcher(
				BamParallelDecodingDecompressedBlockAddPendingInterface & raddDecompressedPendingInterface,
				BamParallelDecodingDecompressedBlockReturnInterface     & rreturnDecompressedInterface,
				BamParallelDecodingParsedBlockAddPendingInterface       & raddParsedPendingInterface,
				BamParallelDecodingParsedBlockStallInterface            & rparseStallInterface,
				BamParallelDecodingParsePackageReturnInterface          & rpackageReturnInterface
			) : addDecompressedPendingInterface(raddDecompressedPendingInterface),
			    returnDecompressedInterface(rreturnDecompressedInterface),
			    addParsedPendingInterface(raddParsedPendingInterface),
			    parseStallInterface(rparseStallInterface),
			    packageReturnInterface(rpackageReturnInterface)
			{
			
			}
		
			virtual void dispatch(
				libmaus::parallel::SimpleThreadWorkPackage * P, 
				libmaus::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & tpi
			)
			{
				BamParallelDecodingParseBlockWorkPackage * BP = dynamic_cast<BamParallelDecodingParseBlockWorkPackage *>(P);
				assert ( BP );
				
				// tpi.addLogStringWithThreadId("BamParallelDecodingParseBlockWorkPackageDispatcher::dispatch() block id " + libmaus::util::NumberSerialisation::formatNumber(BP->decompressedblock->blockid,0));
				
				// can we parse all information in the decompressed input block?
				if ( BP->parseInfo->parseBlock(*(BP->decompressedblock),*(BP->parseBlock)) )
				{
					// tpi.addLogStringWithThreadId("BamParallelDecodingParseBlockWorkPackageDispatcher::dispatch() parseBlock true");
					
					// if this is the last input block
					if ( BP->decompressedblock->final )
					{
						// post process block (reorder pointers to original input order)
						BP->parseBlock->reorder();
						BP->parseBlock->final = true;
						BP->parseBlock->low   = BP->parseInfo->parseacc;
						BP->parseInfo->parseacc += BP->parseBlock->fill();
						addParsedPendingInterface.putBamParallelDecodingParsedBlockAddPending(BP->parseBlock);						
					}
					// otherwise parse block might not be full yet, stall it
					else
					{
						// std::cerr << "stalling on " << BP->decompressedblock->blockid << std::endl;
						parseStallInterface.putBamParallelDecodingParsedBlockStall(BP->parseBlock);
					}
					
					// return decompressed input block (implies we are ready for the next one)
					returnDecompressedInterface.putBamParallelDecodingDecompressedBlockReturn(BP->decompressedblock);
				}
				else
				{
					// tpi.addLogStringWithThreadId("BamParallelDecodingParseBlockWorkPackageDispatcher::dispatch() parseBlock false");
					
					// post process block (reorder pointers to original input order)
					BP->parseBlock->reorder();
					// put back last name
					BP->parseInfo->putBackLastName(*(BP->parseBlock));
					// not the last one
					BP->parseBlock->final = false;
					// set low
					BP->parseBlock->low   = BP->parseInfo->parseacc;
					// increase number of reads seen
					BP->parseInfo->parseacc += BP->parseBlock->fill();
					
					// parse block is full, add it to pending list
					addParsedPendingInterface.putBamParallelDecodingParsedBlockAddPending(BP->parseBlock);
					// decompressed input block still has more data, mark it as pending again
					addDecompressedPendingInterface.putBamParallelDecodingDecompressedBlockAddPending(BP->decompressedblock);
				}

				// return the work package				
				packageReturnInterface.putBamParallelDecodingReturnParsePackage(BP);				
			}		
		};
	}
}
#endif
