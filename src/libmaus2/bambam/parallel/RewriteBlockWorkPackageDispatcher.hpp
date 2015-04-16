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
#if ! defined(LIBMAUS2_BAMBAM_PARALLEL_REWRITEBLOCKWORKPACKAGEDISPATCHER_HPP)
#define LIBMAUS2_BAMBAM_PARALLEL_REWRITEBLOCKWORKPACKAGEDISPATCHER_HPP

#include <libmaus2/bambam/parallel/RewritePackageReturnInterface.hpp>
#include <libmaus2/bambam/parallel/AlignmentBufferReturnInterface.hpp>
#include <libmaus2/bambam/parallel/AlignmentBufferReinsertInterface.hpp>
#include <libmaus2/bambam/parallel/AlignmentRewriteBufferAddPendingInterface.hpp>
#include <libmaus2/bambam/parallel/AlignmentRewriteBufferReinsertForFillingInterface.hpp>
#include <libmaus2/parallel/SimpleThreadWorkPackageDispatcher.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			// dispatcher for block rewriting
			struct RewriteBlockWorkPackageDispatcher : public libmaus2::parallel::SimpleThreadWorkPackageDispatcher
			{
				RewritePackageReturnInterface & packageReturnInterface;
				AlignmentBufferReturnInterface & alignmentBufferReturnInterface;
				AlignmentBufferReinsertInterface & alignmentBufferReinsertInterface;
				AlignmentRewriteBufferAddPendingInterface & alignmentRewriteBufferAddPendingInterface;
				AlignmentRewriteBufferReinsertForFillingInterface & alignmentRewriteBufferReinsertForFillingInterface;
				
				RewriteBlockWorkPackageDispatcher(
					RewritePackageReturnInterface & rpackageReturnInterface,
					AlignmentBufferReturnInterface & ralignmentBufferReturnInterface,
					AlignmentBufferReinsertInterface & ralignmentBufferReinsertInterface,
					AlignmentRewriteBufferAddPendingInterface & ralignmentRewriteBufferAddPendingInterface,
					AlignmentRewriteBufferReinsertForFillingInterface & ralignmentRewriteBufferReinsertForFillingInterface
				) : packageReturnInterface(rpackageReturnInterface), 
				    alignmentBufferReturnInterface(ralignmentBufferReturnInterface),
				    alignmentBufferReinsertInterface(ralignmentBufferReinsertInterface), 
				    alignmentRewriteBufferAddPendingInterface(ralignmentRewriteBufferAddPendingInterface),
				    alignmentRewriteBufferReinsertForFillingInterface(ralignmentRewriteBufferReinsertForFillingInterface)
				{
				}
			
				virtual void dispatch(
					libmaus2::parallel::SimpleThreadWorkPackage * P, 
					libmaus2::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & tpi
				)
				{
					RewriteBlockWorkPackage * BP = dynamic_cast<RewriteBlockWorkPackage *>(P);
					assert ( BP );
					
					libmaus2::bambam::BamAlignment * stallAlgn = 0;
					bool rewriteBlockFull = false;
	
					while ( (!rewriteBlockFull) && (stallAlgn=BP->parseBlock->popStallBuffer()) )
					{
						if ( BP->rewriteBlock->put(reinterpret_cast<char const *>(stallAlgn->D.begin()),stallAlgn->blocksize) )
						{
							BP->parseBlock->returnAlignment(stallAlgn);
						}
						else
						{
							BP->parseBlock->pushFrontStallBuffer(stallAlgn);
							rewriteBlockFull = true;
						}
					}
					
					// if there is still space in the rewrite block
					if ( ! rewriteBlockFull )
					{
						std::deque<libmaus2::bambam::BamAlignment *> algns;
						
						while ( 
							(!rewriteBlockFull)
							&&
							BP->parseBlock->extractNextSameName(algns) 
						)
						{
							assert ( algns.size() );
							
							int64_t i1 = -1;
							int64_t i2 = -1;
							
							for ( uint64_t i = 0; i < algns.size(); ++i )
							{
								uint16_t const flags = algns[i]->getFlags();
								
								if ( 
									(! (flags&libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FPAIRED))
									||
									(flags & (libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FSECONDARY|libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FSUPPLEMENTARY) )
								)
								{
									continue;
								}
								else if ( 
									(flags&libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREAD1)
									&&
									(!(flags&libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREAD2))
								)
								{
									i1 = i;
								}
								else if ( 
									(flags&libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREAD2)
									&&
									(!(flags&libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREAD1))
								)
								{
									i2 = i;
								}
							}
							
							if ( (i1 != -1) && (i2 != -1) )
							{
								libmaus2::bambam::BamAlignment & a1 = *(algns[i1]);
								libmaus2::bambam::BamAlignment & a2 = *(algns[i2]);
							
								a1.filterOutAux(BP->parseBlock->MQMSMCMTfilter);
								a2.filterOutAux(BP->parseBlock->MQMSMCMTfilter);
								
								if ( algns[i1]->blocksize + 6 * (3+sizeof(int32_t)) > algns[i1]->D.size() )
									a1.D.resize(algns[i1]->blocksize + 8 * (3+sizeof(int32_t)));
								if ( algns[i2]->blocksize + 6 * (3+sizeof(int32_t)) > algns[i2]->D.size() )
									a2.D.resize(algns[i2]->blocksize + 8 * (3+sizeof(int32_t)));
								
								libmaus2::bambam::BamAlignment::fixMateInformationPreFiltered(a1,a2);
								libmaus2::bambam::BamAlignment::addMateBaseScorePreFiltered(a1,a2);
								libmaus2::bambam::BamAlignment::addMateCoordinatePreFiltered(a1,a2);
								libmaus2::bambam::BamAlignment::addMateTagPreFiltered(a1,a2,"MT");
							}
						
							while ( algns.size() )
							{
								libmaus2::bambam::BamAlignment * algn = algns.front();
								int64_t const coordinate = algn->getCoordinate();
								
								if ( BP->rewriteBlock->put(reinterpret_cast<char const *>(algn->D.begin()),algn->blocksize,coordinate) )
								{
									BP->parseBlock->returnAlignment(algn);
									algns.pop_front();
								}
								else
								{
									// block is full, stall rest of the alignments
									rewriteBlockFull = true;
									
									for ( uint64_t i = 0; i < algns.size(); ++i )
										BP->parseBlock->pushBackStallBuffer(algns[i]);
										
									algns.resize(0);
								}
							}					
						}
					}
					
					// force block pass if this is the final parse block
					// rewriteBlockFull = rewriteBlockFull || BP->parseBlock->final;
					
					// if rewrite block is now full
					if ( rewriteBlockFull )
					{
						// finalise block by compactifying pointers
						BP->rewriteBlock->reorder();
						// add rewrite block to pending list
						alignmentRewriteBufferAddPendingInterface.putAlignmentRewriteBufferAddPending(BP->rewriteBlock);
						// reinsert parse block
						alignmentBufferReinsertInterface.putReinsertAlignmentBuffer(BP->parseBlock);
					}
					// input buffer is empty
					else
					{
						// reinsert rewrite buffer for more filling
						alignmentRewriteBufferReinsertForFillingInterface.putAlignmentRewriteBufferReinsertForFilling(BP->rewriteBlock);
						// return empty input buffer
						BP->parseBlock->reset();
						alignmentBufferReturnInterface.putReturnAlignmentBuffer(BP->parseBlock);
					}
	
					#if 0
					// add algnbuffer to the free list
					parseBlockFreeList.put(algn);
	
					// see if we can parse the next block now
					checkParsePendingList();
					#endif
					
					// return the work package				
					packageReturnInterface.putReturnRewritePackage(BP);				
				}		
			};
		}
	}
}
#endif
