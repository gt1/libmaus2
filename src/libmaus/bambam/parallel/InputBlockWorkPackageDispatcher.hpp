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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_INPUTBLOCKWORKPACKAGEDISPATCHER_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_INPUTBLOCKWORKPACKAGEDISPATCHER_HPP

#include <libmaus/bambam/parallel/InputBlockWorkPackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/InputBlockAddPendingInterface.hpp>
#include <libmaus/bambam/parallel/RequeReadInterface.hpp>
#include <libmaus/parallel/SimpleThreadWorkPackageDispatcher.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			/**
			 * compressed block input dispatcher
			 **/
			struct InputBlockWorkPackageDispatcher : public libmaus::parallel::SimpleThreadWorkPackageDispatcher
			{
				InputBlockWorkPackageReturnInterface & returnInterface;
				InputBlockAddPendingInterface & addPending;
				RequeReadInterface & requeInterface;
				// ZZZ add request for requeing here
				uint64_t const batchsize;
				
				InputBlockWorkPackageDispatcher(
					InputBlockWorkPackageReturnInterface & rreturnInterface,
					InputBlockAddPendingInterface & raddPending,
					RequeReadInterface & rrequeInterface,
					uint64_t const rbatchsize = 16
				) : returnInterface(rreturnInterface), addPending(raddPending), requeInterface(rrequeInterface), batchsize(rbatchsize)
				{
				
				}
			
				virtual void dispatch(
					libmaus::parallel::SimpleThreadWorkPackage * P, 
					libmaus::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & tpi
				)
				{
					// get type cast work package pointer
					InputBlockWorkPackage * BP = dynamic_cast<InputBlockWorkPackage *>(P);
					assert ( BP );
					
					// vector of blocks to pass on for decompression
					std::deque<ControlInputInfo::input_block_type *> blockPassVector;
					
					// try to get lock
					if ( BP->inputinfo->readLock.trylock() )
					{
						// free lock at end of scope
						libmaus::parallel::ScopePosixSpinLock llock(BP->inputinfo->readLock,true /* pre locked */);
	
						// do not run if eof is already set
						bool running = !BP->inputinfo->getEOF();
						
						while ( running )
						{
							// try to get a free buffer
							ControlInputInfo::input_block_type * block = 
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
									BP->inputinfo->setEOF(true);
								}
	
								blockPassVector.push_back(block);
							}
							else
							{
								running = false;
							}
							
							// enque decompress requests, but make sure we do not send all
							// before leaving the while loop
							if ( blockPassVector.size() == (batchsize<<1) )
							{
								addPending.putInputBlockAddPending(blockPassVector.begin(),blockPassVector.begin()+batchsize);
								for ( uint64_t i = 0; i < batchsize; ++i )
									blockPassVector.pop_front();
							}
						}

						// enque rest of decompress requests
						addPending.putInputBlockAddPending(blockPassVector.begin(),blockPassVector.end());
					}
	
					// return the work package
					returnInterface.putInputBlockWorkPackage(BP);
	
					// request requeing	
					requeInterface.requeRead();
				}		
			};
		}
	}
}
#endif
