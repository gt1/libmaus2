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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_GENERICINPUTCONTROLREADWORKPACKAGEDISPATCHER_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_GENERICINPUTCONTROLREADWORKPACKAGEDISPATCHER_HPP

#include <libmaus/bambam/parallel/GenericInputControlReadWorkPackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/GenericInputControlReadAddPendingInterface.hpp>
#include <libmaus/parallel/SimpleThreadWorkPackageDispatcher.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			struct GenericInputControlReadWorkPackageDispatcher : public libmaus::parallel::SimpleThreadWorkPackageDispatcher
			{
				typedef GenericInputControlReadWorkPackageDispatcher this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
				
				GenericInputControlReadWorkPackageReturnInterface & packageReturnInterface;
				GenericInputControlReadAddPendingInterface & addPendingInterface;
			
				GenericInputControlReadWorkPackageDispatcher(
					GenericInputControlReadWorkPackageReturnInterface & rpackageReturnInterface,
					GenericInputControlReadAddPendingInterface & raddPendingInterface
				)
				: packageReturnInterface(rpackageReturnInterface), addPendingInterface(raddPendingInterface)
				{
				
				}
			
				void dispatch(libmaus::parallel::SimpleThreadWorkPackage * P, libmaus::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */)
				{
					assert ( dynamic_cast<GenericInputControlReadWorkPackage *>(P) != 0 );
					GenericInputControlReadWorkPackage * BP = dynamic_cast<GenericInputControlReadWorkPackage *>(P);
					
					GenericInputSingleData & data = *(BP->data);
					std::vector<GenericInputBase::block_type::shared_ptr_type> fullBlocks;
					
					if ( data.inlock.trylock() )
					{
						libmaus::parallel::ScopePosixSpinLock slock(data.inlock,true /* pre locked */);
						
						GenericInputBase::block_type::shared_ptr_type sblock;
			
						while ( 
							((!data.finite) || data.dataleft) 
							&&
							(!data.getEOF()) && (sblock=data.blockFreeList.getIf()) 
						)
						{
							// reset buffer
							sblock->reset();
							// set stream id
							sblock->meta.streamid = data.getStreamId();
							// set block id
							sblock->meta.blockid = data.getBlockId();
							// insert stalled data
							sblock->insert(data.stallArray.begin(),data.stallArray.begin()+data.stallArraySize);
			
							// extend if there is no space
							if ( sblock->pe == sblock->A.end() )
								sblock->extend();
							
							// there should be free space now
							assert ( sblock->pe != sblock->A.end() );
			
							// fill buffer
							libmaus::bambam::parallel::GenericInputBlockFillResult P = sblock->fill(
								data.in,
								data.finite,
								data.dataleft);
			
							if ( data.in.bad() )
							{
								libmaus::exception::LibMausException lme;
								lme.getStream() << "Stream error while filling buffer." << std::endl;
								lme.finish();
								throw lme;
							}
			
							if ( data.finite )
							{
								assert ( P.gcount <= data.dataleft );
								data.dataleft -= P.gcount;
								
								if ( data.dataleft == 0 )
								{
									P.eof = true;
								}
							}
			
							data.setEOF(P.eof);
							sblock->meta.eof = P.eof;
							
							// parse bgzf block headers to determine how many full blocks we have				
							int64_t bs = -1;
							libmaus::bambam::parallel::GenericInputBlockSubBlockInfo & meta = sblock->meta;
							uint64_t f = 0;
							while ( (bs=libmaus::lz::BgzfInflateHeaderBase::getBlockSize(sblock->pc,sblock->pe)) >= 0 )
							{
								meta.addBlock(std::pair<uint8_t *,uint8_t *>(sblock->pc,sblock->pc+bs));
								sblock->pc += bs;
								f += 1;
							}
			
							#if 0
							// empty file? inject eof block
							if ( data.getEOF() && (sblock->pc == sblock->pe) && (!f) )
							{
								std::ostringstream ostr;
								libmaus::lz::BgzfDeflate<std::ostream> bgzfout(ostr);
								bgzfout.addEOFBlock();
								std::string const s = ostr.str();
								sblock->insert(s.begin(),s.end());
								
								meta.addBlock(std::pair<uint8_t *,uint8_t *>(sblock->pc,sblock->pe));
								f += 1;
							}
							#endif
											
							// empty file?
							if ( data.getEOF() && (sblock->pc == sblock->pe) && (!f) )
							{
								libmaus::exception::LibMausException lme;
								lme.getStream() << "Invalid empty BGZF stream." << std::endl;
								lme.finish();
								throw lme;
							}
			
							// extract rest of data for next block
							data.stallArraySize = sblock->extractRest(data.stallArray);
			
							if ( f )
							{
								fullBlocks.push_back(sblock);
								data.incrementBlockId();
							}
							else
							{
								// buffer is too small for a single block
								if ( ! data.getEOF() )
								{
									sblock->extend();
									data.blockFreeList.put(sblock);
								}
								else if ( sblock->pc != sblock->pe )
								{
									assert ( data.getEOF() );
									// throw exception, block is incomplete at EOF
									libmaus::exception::LibMausException lme;
									lme.getStream() << "Unexpected EOF." << std::endl;
									lme.finish();
									throw lme;
								}
							}
						}			
					}
			
					packageReturnInterface.genericInputControlReadWorkPackageReturn(BP);
			
					for ( uint64_t i = 0; i < fullBlocks.size(); ++i )
					{
						addPendingInterface.genericInputControlReadAddPending(fullBlocks[i]);
					}		
				}
			};
		}
	}
}
#endif
