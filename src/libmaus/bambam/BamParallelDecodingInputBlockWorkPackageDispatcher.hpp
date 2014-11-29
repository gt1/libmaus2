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
#if ! defined(LIBMAUS_BAMBAM_BAMPARALLELDECODINGINPUTBLOCKWORKPACKAGEDISPATCHER_HPP)
#define LIBMAUS_BAMBAM_BAMPARALLELDECODINGINPUTBLOCKWORKPACKAGEDISPATCHER_HPP

#include <libmaus/bambam/BamParallelDecodingInputBlockWorkPackageReturnInterface.hpp>
#include <libmaus/bambam/BamParallelDecodingInputBlockAddPendingInterface.hpp>
#include <libmaus/parallel/SimpleThreadWorkPackageDispatcher.hpp>

namespace libmaus
{
	namespace bambam
	{
		/**
		 * compressed block input dispatcher
		 **/
		struct BamParallelDecodingInputBlockWorkPackageDispatcher : public libmaus::parallel::SimpleThreadWorkPackageDispatcher
		{
			BamParallelDecodingInputBlockWorkPackageReturnInterface & returnInterface;
			BamParallelDecodingInputBlockAddPendingInterface & addPending;
			
			BamParallelDecodingInputBlockWorkPackageDispatcher(
				BamParallelDecodingInputBlockWorkPackageReturnInterface & rreturnInterface,
				BamParallelDecodingInputBlockAddPendingInterface & raddPending
			) : returnInterface(rreturnInterface), addPending(raddPending)
			{
			
			}
		
			virtual void dispatch(
				libmaus::parallel::SimpleThreadWorkPackage * P, 
				libmaus::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & tpi
			)
			{
				// get type cast work package pointer
				BamParallelDecodingInputBlockWorkPackage * BP = dynamic_cast<BamParallelDecodingInputBlockWorkPackage *>(P);
				assert ( BP );
				
				// vector of blocks to pass on for decompression
				std::vector<BamParallelDecodingControlInputInfo::input_block_type *> blockPassVector;
				
				// try to get lock
				if ( BP->inputinfo->readLock.trylock() )
				{
					// free lock at end of scope
					libmaus::parallel::ScopePosixSpinLock llock(BP->inputinfo->readLock,true /* pre locked */);

					// do not run if eof is already set
					bool running = !BP->inputinfo->eof;
					
					while ( running )
					{
						// try to get a free buffer
						BamParallelDecodingControlInputInfo::input_block_type * block = 
							BP->inputinfo->inputBlockFreeList.getIf();
						
						// if we have a buffer then read the next bgzf block
						if ( block )
						{
							// read
							block->readBlock(BP->inputinfo->istr);
							// copy stream id
							block->streamid = BP->inputinfo->streamid;
							// set next block id
							block->blockid = BP->inputinfo->blockid++;
							
							if ( block->final )
							{
								running = false;
								BP->inputinfo->eof = true;
							}

							blockPassVector.push_back(block);
						}
						else
						{
							running = false;
						}
					}
				}

				// return the work package
				returnInterface.putBamParallelDecodingInputBlockWorkPackage(BP);

				// enque decompress requests
				for ( uint64_t i = 0; i < blockPassVector.size(); ++i )
					addPending.putBamParallelDecodingInputBlockAddPending(blockPassVector[i]);
			}		
		};
	}
}
#endif
