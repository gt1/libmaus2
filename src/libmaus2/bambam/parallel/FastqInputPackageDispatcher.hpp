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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_FASTQINPUTPACKAGEDISPATCHER_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_FASTQINPUTPACKAGEDISPATCHER_HPP

#include <libmaus2/bambam/parallel/FastqInputPackageReturnInterface.hpp>
#include <libmaus2/bambam/parallel/FastqInputPackageAddPendingInterface.hpp>
#include <libmaus2/parallel/SimpleThreadWorkPackageDispatcher.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{	
			struct FastqInputPackageDispatcher : public libmaus2::parallel::SimpleThreadWorkPackageDispatcher
			{
				typedef FastqInputPackageDispatcher this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
				
				FastqInputPackageReturnInterface & packageReturnInterface;
				FastqInputPackageAddPendingInterface & addPendingInterface;
							
				FastqInputPackageDispatcher(
					FastqInputPackageReturnInterface & rpackageReturnInterface,
					FastqInputPackageAddPendingInterface & raddPendingInterface
				)
				: packageReturnInterface(rpackageReturnInterface), addPendingInterface(raddPendingInterface)
				{
				}
			
				void dispatch(
					libmaus2::parallel::SimpleThreadWorkPackage * P, 
					libmaus2::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */
				)
				{
					assert ( dynamic_cast<FastqInputPackage *>(P) != 0 );
					FastqInputPackage * BP = dynamic_cast<FastqInputPackage *>(P);

					FastQInputDesc & data = *(BP->data);

					typedef FastQInputDescBase::input_block_type input_block_type;
					typedef FastQInputDescBase::free_list_type free_list_type;
					
					libmaus2::parallel::PosixSpinLock & inlock = data.inlock;
					free_list_type & blockFreeList = data.blockFreeList;
					uint64_t const streamid = data.getStreamId();
					libmaus2::autoarray::AutoArray<uint8_t,libmaus2::autoarray::alloc_type_c> & stallArray = data.stallArray;
					uint64_t volatile & stallArraySize = data.stallArraySize;
					std::istream & in = data.in;

					std::vector<input_block_type::shared_ptr_type> fullBlocks;
					
					if ( inlock.trylock() )
					{
						libmaus2::parallel::ScopePosixSpinLock slock(inlock,true /* pre locked */);
						
						input_block_type::shared_ptr_type sblock;
			
						while ( 
							(!data.getEOF()) && (sblock=blockFreeList.getIf()) 
						)
						{
							// reset buffer
							sblock->reset();
							// set stream id
							sblock->meta.streamid = streamid;
							// set block id
							sblock->meta.blockid = data.getBlockId();
							// insert stalled data
							sblock->insert(stallArray.begin(),stallArray.begin()+stallArraySize);
			
							// extend if there is no space
							if ( sblock->pe == sblock->A.end() )
								sblock->extend();
							
							// there should be free space now
							assert ( sblock->pe != sblock->A.end() );
			
							// fill buffer
							libmaus2::bambam::parallel::GenericInputBlockFillResult P = sblock->fill(
								in, false /* finite */,0 /* dataleft */);
			
							if ( in.bad() )
							{
								libmaus2::exception::LibMausException lme;
								lme.getStream() << "Stream error while filling buffer." << std::endl;
								lme.finish();
								throw lme;
							}
			
							data.setEOF(P.eof);
							sblock->meta.eof = P.eof;
							
							// parse bgzf block headers to determine how many full blocks we have				
							libmaus2::bambam::parallel::GenericInputBlockSubBlockInfo & meta = sblock->meta;
							uint64_t f = 0;

							bool foundnewline = false;
							uint8_t * const pc = sblock->pc;
							uint8_t * pe = sblock->pe;
							while ( pe != pc )
							{
								uint8_t const c = *(--pe);
								
								if ( c == '\n' )
								{
									foundnewline = true;
									break;
								}
							}
							
							if ( foundnewline )
							{
								pe += 1;
								
								uint8_t * ls[4] = {0,0,0,0};
								
								while ( pe != pc )
								{
									assert ( pe[-1] == '\n' );
									
									pe -= 1;
									while ( pe != pc )
									{
										uint8_t const c = *(--pe);
										
										if ( c == '\n' )
										{
											pe += 1;
											break;
										}
									}
									
									ls[3] = ls[2];
									ls[2] = ls[1];
									ls[1] = ls[0];
									ls[0] = pe;
									
									if ( ls[3] && ls[0][0] == '@' && ls[2][0] == '+' )
										break;
								}							

								if ( ls[3] && ls[0][0] == '@' && ls[2][0] == '+' )
								{
									uint8_t * le = ls[3];
									while ( *le != '\n' )
										++le;
									assert ( *le == '\n' );
									le += 1;

									uint64_t const bs = le - pc;
									meta.addBlock(std::pair<uint8_t *,uint8_t *>(sblock->pc,sblock->pc+bs));
									f += 1;
									
									sblock->pc += bs;
									
									if ( sblock->pc == sblock->pe && data.getEOF() )
										sblock->meta.eof = true;
								}
							}

							// empty file? inject empty block so we see eof downstream
							if ( data.getEOF() && (sblock->pc == sblock->pe) && (!f) )
							{
								meta.addBlock(std::pair<uint8_t *,uint8_t *>(sblock->pc,sblock->pe));
								f += 1;
							}
											
							// extract rest of data for next block
							stallArraySize = sblock->extractRest(stallArray);
			
							if ( f )
							{
								if ( fullBlocks.size() >= 1 )
								{
									for ( uint64_t i = 0; i < fullBlocks.size(); ++i )
										addPendingInterface.fastqInputPackageAddPending(fullBlocks[i]);
									fullBlocks.resize(0);
								}
								fullBlocks.push_back(sblock);
								data.incrementBlocksProduced();
								data.incrementBlockId();
							}
							else
							{
								// buffer is too small for a single block
								if ( ! data.getEOF() )
								{
									sblock->extend();
									blockFreeList.put(sblock);
								}
								else if ( sblock->pc != sblock->pe )
								{
									assert ( data.getEOF() );
									// throw exception, block is incomplete at EOF
									libmaus2::exception::LibMausException lme;
									lme.getStream() << "Unexpected EOF." << std::endl;
									lme.finish();
									throw lme;
								}
							}
						}			
					}
			
					packageReturnInterface.fastqInputPackageReturn(BP);
			
					for ( uint64_t i = 0; i < fullBlocks.size(); ++i )
						addPendingInterface.fastqInputPackageAddPending(fullBlocks[i]);
				}
			};
		}
	}
}
#endif
