/*
    libmaus2
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
#if ! defined(LIBMAUS2_BAMBAM_PARALLEL_GENERICINPUTMERGEWORKPACKAGEDISPATCHER_HPP)
#define LIBMAUS2_BAMBAM_PARALLEL_GENERICINPUTMERGEWORKPACKAGEDISPATCHER_HPP

#include <libmaus2/bambam/parallel/GenericInputMergeWorkPackageReturnInterface.hpp>
#include <libmaus2/bambam/parallel/GenericInputMergeDecompressedBlockReturnInterface.hpp>
#include <libmaus2/bambam/parallel/GenericInputControlSetMergeStallSlotInterface.hpp>
#include <libmaus2/bambam/parallel/GenericInputControlMergeBlockFinishedInterface.hpp>
#include <libmaus2/bambam/parallel/GenericInputControlMergeRequeue.hpp>
#include <libmaus2/parallel/SimpleThreadWorkPackageDispatcher.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			template<typename _heap_element_type>
			struct GenericInputMergeWorkPackageDispatcher : public libmaus2::parallel::SimpleThreadWorkPackageDispatcher
			{
				typedef _heap_element_type heap_element_type;
				typedef GenericInputMergeWorkPackageDispatcher<heap_element_type> this_type;
				typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
				
				GenericInputMergeWorkPackageReturnInterface<heap_element_type> & packageReturnInterface;
				GenericInputMergeDecompressedBlockReturnInterface & decompressPackageReturnInterface;
				GenericInputControlSetMergeStallSlotInterface & setMergeStallSlotInterface;
				GenericInputControlMergeBlockFinishedInterface & mergeFinishedInterface;
				GenericInputControlMergeRequeue & mergeRequeueInterface;
				
				libmaus2::parallel::LockedCounter LC;
			
				GenericInputMergeWorkPackageDispatcher(
					GenericInputMergeWorkPackageReturnInterface<heap_element_type> & rpackageReturnInterface,
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
			
				void dispatch(libmaus2::parallel::SimpleThreadWorkPackage * P, libmaus2::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */)
				{
					assert ( dynamic_cast<GenericInputMergeWorkPackage<heap_element_type> *>(P) != 0 );
					GenericInputMergeWorkPackage<heap_element_type> * BP = dynamic_cast<GenericInputMergeWorkPackage<heap_element_type> *>(P);
					
					libmaus2::autoarray::AutoArray<GenericInputSingleData::unique_ptr_type> & data = *(BP->data);
					libmaus2::util::FiniteSizeHeap<heap_element_type> & mergeheap = *(BP->mergeheap);
					libmaus2::bambam::parallel::AlignmentBuffer & algn = *(BP->algn);
			
					// loop running
					bool running = !mergeheap.empty();		
			
					if ( ! running )
					{
						// final buffer
						BP->algn->final = true;
						// mark buffer as finished
						mergeFinishedInterface.genericInputControlMergeBlockFinished(BP->algn);
						// reset buffer
						BP->algn = libmaus2::bambam::parallel::AlignmentBuffer::shared_ptr_type();			
					}
							
					while ( running )
					{
						heap_element_type const & HE = mergeheap.top();
			
						libmaus2::bambam::parallel::DecompressedBlock * eblock = HE.block;
						uint8_t const * ublock = reinterpret_cast<uint8_t const *>(eblock->getPrevParsePointer());
						uint8_t const * ublock4 = ublock+sizeof(uint32_t);
						uint32_t const len = libmaus2::bambam::DecoderBase::getLEInteger(ublock,sizeof(uint32_t));
						#if 0
						char const * name = libmaus2::bambam::BamAlignmentDecoderBase::getReadName(ublock4);
						std::cerr << name 
							<< "\t" << libmaus2::bambam::BamAlignmentDecoderBase::getRefID(ublock4) 
							<< "\t" << libmaus2::bambam::BamAlignmentDecoderBase::getPos(ublock4) 
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
							bool const islast = HE.isLast();
							mergeheap.popvoid();
							
							// last alignment in block
							if ( islast )
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
										BP->algn = libmaus2::bambam::parallel::AlignmentBuffer::shared_ptr_type();
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
									uint64_t const streamid = eblock->streamid;
									// get block
									libmaus2::bambam::parallel::DecompressedBlock::shared_ptr_type block = data[streamid]->processActive;
									// return block
									decompressPackageReturnInterface.genericInputMergeDecompressedBlockReturn(block);
			
									{
										// get lock	
										libmaus2::parallel::ScopePosixSpinLock qlock(data[streamid]->processLock);
										// erase block pointer
										data[streamid]->processActive = libmaus2::bambam::parallel::DecompressedBlock::shared_ptr_type();
			
										// if queue is not empty
										if ( data[streamid]->processQueue.size() )
										{
											// get next block
											libmaus2::bambam::parallel::DecompressedBlock::shared_ptr_type nextblock = data[streamid]->processQueue.front();
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
													BP->algn = libmaus2::bambam::parallel::AlignmentBuffer::shared_ptr_type();
												}
											}
											// non-empty block, get next alignment
											else
											{
												// set active block
												data[streamid]->processActive = nextblock;
												// this should be non-empty
												assert ( data[streamid]->processActive->getNumParsePointers() );
												// push
												mergeheap.pushset(data[streamid]->processActive.get());
											}
										}
										// nothing in queue, mark channel as missing and leave loop
										else
										{
											// buffer has space, stall it
											setMergeStallSlotInterface.genericInputControlSetMergeStallSlot(BP->algn);
											// reset buffer
											BP->algn = libmaus2::bambam::parallel::AlignmentBuffer::shared_ptr_type();
										
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
								// push it
								mergeheap.pushset(data[eblock->streamid]->processActive.get());
							}
						}
						else
						{
							// buffer is full
							mergeFinishedInterface.genericInputControlMergeBlockFinished(BP->algn);
							// reque merging
							mergeRequeueInterface.genericInputControlMergeRequeue();
							// reset buffer
							BP->algn = libmaus2::bambam::parallel::AlignmentBuffer::shared_ptr_type();
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
