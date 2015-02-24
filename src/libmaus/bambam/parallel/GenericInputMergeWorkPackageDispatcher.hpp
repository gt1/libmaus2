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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_GENERICINPUTMERGEWORKPACKAGEDISPATCHER_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_GENERICINPUTMERGEWORKPACKAGEDISPATCHER_HPP

#include <libmaus/bambam/parallel/GenericInputMergeWorkPackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/GenericInputMergeDecompressedBlockReturnInterface.hpp>
#include <libmaus/bambam/parallel/GenericInputControlSetMergeStallSlotInterface.hpp>
#include <libmaus/bambam/parallel/GenericInputControlMergeBlockFinishedInterface.hpp>
#include <libmaus/bambam/parallel/GenericInputControlMergeRequeue.hpp>
#include <libmaus/parallel/SimpleThreadWorkPackageDispatcher.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			struct GenericInputMergeWorkPackageDispatcher : public libmaus::parallel::SimpleThreadWorkPackageDispatcher
			{
				typedef GenericInputMergeWorkPackageDispatcher this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
				
				GenericInputMergeWorkPackageReturnInterface & packageReturnInterface;
				GenericInputMergeDecompressedBlockReturnInterface & decompressPackageReturnInterface;
				GenericInputControlSetMergeStallSlotInterface & setMergeStallSlotInterface;
				GenericInputControlMergeBlockFinishedInterface & mergeFinishedInterface;
				GenericInputControlMergeRequeue & mergeRequeueInterface;
				
				libmaus::parallel::LockedCounter LC;
			
				GenericInputMergeWorkPackageDispatcher(
					GenericInputMergeWorkPackageReturnInterface & rpackageReturnInterface,
					GenericInputMergeDecompressedBlockReturnInterface & rdecompressPackageReturnInterface,
					GenericInputControlSetMergeStallSlotInterface & rsetMergeStallSlotInterface,
					GenericInputControlMergeBlockFinishedInterface & rmergeFinishedInterface,
					GenericInputControlMergeRequeue & rmergeRequeueInterface
				)
				: packageReturnInterface(rpackageReturnInterface),
				  decompressPackageReturnInterface(rdecompressPackageReturnInterface),
				  setMergeStallSlotInterface(rsetMergeStallSlotInterface),
				  mergeFinishedInterface(rmergeFinishedInterface),
				  mergeRequeueInterface(rmergeRequeueInterface),
				  LC(0)
				{
				
				}
			
				void dispatch(libmaus::parallel::SimpleThreadWorkPackage * P, libmaus::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */)
				{
					assert ( dynamic_cast<GenericInputMergeWorkPackage *>(P) != 0 );
					GenericInputMergeWorkPackage * BP = dynamic_cast<GenericInputMergeWorkPackage *>(P);
					
					libmaus::autoarray::AutoArray<GenericInputSingleData::unique_ptr_type> & data = *(BP->data);
					libmaus::util::FiniteSizeHeap<GenericInputControlMergeHeapEntry> & mergeheap = *(BP->mergeheap);
					libmaus::bambam::parallel::AlignmentBuffer & algn = *(BP->algn);
			
					GenericInputControlMergeHeapEntry E;
					// loop running
					bool running = !mergeheap.empty();		
			
					if ( ! running )
					{
						// final buffer
						BP->algn->final = true;
						// mark buffer as finished
						mergeFinishedInterface.genericInputControlMergeBlockFinished(BP->algn);
						// reset buffer
						BP->algn = libmaus::bambam::parallel::AlignmentBuffer::shared_ptr_type();			
					}
							
					while ( running )
					{
						mergeheap.pop(E);
			
						libmaus::bambam::parallel::DecompressedBlock * eblock = E.block;
						uint8_t const * ublock = reinterpret_cast<uint8_t const *>(eblock->getPrevParsePointer());
						uint8_t const * ublock4 = ublock+sizeof(uint32_t);
						uint32_t const len = libmaus::bambam::DecoderBase::getLEInteger(ublock,sizeof(uint32_t));
						#if 0
						char const * name = libmaus::bambam::BamAlignmentDecoderBase::getReadName(ublock4);
						std::cerr << name 
							<< "\t" << libmaus::bambam::BamAlignmentDecoderBase::getRefID(ublock4) 
							<< "\t" << libmaus::bambam::BamAlignmentDecoderBase::getPos(ublock4) 
							<< std::endl;
						#endif
			
						bool ok;
						
						// buffer is not empty, see if we can put more
						if ( algn.fillUnpacked() )
						{
							ok = algn.put(reinterpret_cast<char const *>(ublock4),len);
						}
						// buffer is empty, but we always put at least one entry
						// even if this implies we need to extend the buffer
						else
						{
							algn.putExtend(reinterpret_cast<char const *>(ublock4),len);
							ok = true;
						}
			
						if ( ok )
							LC += 1;
			
						if ( ok )
						{			
							// last alignment in block
							if ( E.isLast() )
							{
								// last block?
								if ( eblock->final )
								{
									if ( mergeheap.empty() )
									{
										// final buffer
										BP->algn->final = true;
										// mark buffer as finished
										mergeFinishedInterface.genericInputControlMergeBlockFinished(BP->algn);
										// reset buffer
										BP->algn = libmaus::bambam::parallel::AlignmentBuffer::shared_ptr_type();
										// leave merge loop
										running = false;
									}
									else
									{
										// intentionally left empty
									}
								}
								else
								{
									// get stream id
									uint64_t const streamid = E.block->streamid;
									// get block
									libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type block = data[streamid]->processActive;
									// return block
									decompressPackageReturnInterface.genericInputMergeDecompressedBlockReturn(block);
			
									{
										// get lock	
										libmaus::parallel::ScopePosixSpinLock qlock(data[streamid]->processLock);
										// erase block pointer
										data[streamid]->processActive = libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type();
			
										// if queue is not empty
										if ( data[streamid]->processQueue.size() )
										{
											// get next block
											libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type nextblock = data[streamid]->processQueue.front();
											// remove it from the queue
											data[streamid]->processQueue.pop_front();
											
											// this should be either non-empty or the last block
											assert ( nextblock->getNumParsePointers() || nextblock->final );
			
											// last block, return it			
											if ( !(nextblock->getNumParsePointers())  )
											{
												assert ( nextblock->final );
												
												// return block
												decompressPackageReturnInterface.genericInputMergeDecompressedBlockReturn(nextblock);
												// stop running if this was the last stream ending
												running = !(mergeheap.empty());
												
												if ( ! running )
												{
													// final buffer
													BP->algn->final = true;
													// mark buffer as finished
													mergeFinishedInterface.genericInputControlMergeBlockFinished(BP->algn);
													// reset buffer
													BP->algn = libmaus::bambam::parallel::AlignmentBuffer::shared_ptr_type();
												}
											}
											// non-empty block, get next alignment
											else
											{
												// set active block
												data[streamid]->processActive = nextblock;
												// this should be non-empty
												assert ( data[streamid]->processActive->getNumParsePointers() );
												// construct heap entry
												GenericInputControlMergeHeapEntry GICMHE(data[streamid]->processActive.get());
												// push it
												mergeheap.push(GICMHE);
											}
										}
										// nothing in queue, mark channel as missing and leave loop
										else
										{
											// buffer has space, stall it
											setMergeStallSlotInterface.genericInputControlSetMergeStallSlot(BP->algn);
											// reset buffer
											BP->algn = libmaus::bambam::parallel::AlignmentBuffer::shared_ptr_type();
										
											// missing
											data[streamid]->processMissing = true;
											// quit loop
											running = false;
										}
									}
								}
							}
							// not last element in block
							else
							{
								// construct heap entry
								GenericInputControlMergeHeapEntry GICMHE(data[E.block->streamid]->processActive.get());
								// push it
								mergeheap.push(GICMHE);			
							}
						}
						else
						{
							// put E back on heap
							mergeheap.push(E);
							// buffer is full
							mergeFinishedInterface.genericInputControlMergeBlockFinished(BP->algn);
							// reque merging
							mergeRequeueInterface.genericInputControlMergeRequeue();
							// reset buffer
							BP->algn = libmaus::bambam::parallel::AlignmentBuffer::shared_ptr_type();
							// quit loop
							running = false;
						}
					}
							
					packageReturnInterface.genericInputMergeWorkPackageReturn(BP);
				}
			};
		}
	}
}
#endif
