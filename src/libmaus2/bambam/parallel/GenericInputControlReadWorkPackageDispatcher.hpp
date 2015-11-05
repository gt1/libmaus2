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
#if ! defined(LIBMAUS2_BAMBAM_PARALLEL_GENERICINPUTCONTROLREADWORKPACKAGEDISPATCHER_HPP)
#define LIBMAUS2_BAMBAM_PARALLEL_GENERICINPUTCONTROLREADWORKPACKAGEDISPATCHER_HPP

#include <libmaus2/bambam/parallel/GenericInputControlReadWorkPackageReturnInterface.hpp>
#include <libmaus2/bambam/parallel/GenericInputControlReadAddPendingInterface.hpp>
#include <libmaus2/parallel/SimpleThreadWorkPackageDispatcher.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			struct GenericInputControlReadWorkPackageDispatcher : public libmaus2::parallel::SimpleThreadWorkPackageDispatcher
			{
				typedef GenericInputControlReadWorkPackageDispatcher this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				GenericInputControlReadWorkPackageReturnInterface & packageReturnInterface;
				GenericInputControlReadAddPendingInterface & addPendingInterface;

				enum parser_type_enum
				{
					parse_bam = 0,
					parse_sam = 1
				};

				parser_type_enum const parser_type;

				GenericInputControlReadWorkPackageDispatcher(
					GenericInputControlReadWorkPackageReturnInterface & rpackageReturnInterface,
					GenericInputControlReadAddPendingInterface & raddPendingInterface,
					parser_type_enum const rparser_type = parse_bam
				)
				: packageReturnInterface(rpackageReturnInterface), addPendingInterface(raddPendingInterface), parser_type(rparser_type)
				{
				}

				void dispatch(libmaus2::parallel::SimpleThreadWorkPackage * P, libmaus2::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */)
				{
					assert ( dynamic_cast<GenericInputControlReadWorkPackage *>(P) != 0 );
					GenericInputControlReadWorkPackage * BP = dynamic_cast<GenericInputControlReadWorkPackage *>(P);

					GenericInputSingleDataReadBase & data = *(BP->data);
					std::vector<GenericInputBase::generic_input_shared_block_ptr_type> fullBlocks;

					if ( data.inlock.trylock() )
					{
						libmaus2::parallel::ScopePosixSpinLock slock(data.inlock,true /* pre locked */);

						GenericInputBase::generic_input_shared_block_ptr_type sblock;

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
							libmaus2::bambam::parallel::GenericInputBlockFillResult P = sblock->fill(
								data.in,
								data.finite,
								data.dataleft);

							if ( data.in.bad() )
							{
								libmaus2::exception::LibMausException lme;
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
							libmaus2::bambam::parallel::GenericInputBlockSubBlockInfo & meta = sblock->meta;
							uint64_t f = 0;

							switch ( parser_type )
							{
								case parse_bam:
								{
									int64_t bs = -1;
									while ( (bs=libmaus2::lz::BgzfInflateHeaderBase::getBlockSize(sblock->pc,sblock->pe)) >= 0 )
									{
										meta.addBlock(std::pair<uint8_t *,uint8_t *>(sblock->pc,sblock->pc+bs));
										sblock->pc += bs;
										f += 1;
									}
									break;
								}
								case parse_sam:
								{
									uint8_t * const pc = sblock->pc;
									uint8_t * pe = sblock->pe;
									bool foundnl = false;

									while ( pe != pc )
									{
										uint8_t const c = *(--pe);

										if ( c == '\n' )
										{
											foundnl = true;
											break;
										}
									}

									if ( foundnl )
									{
										assert ( *pe == '\n' );
										pe += 1;
										ptrdiff_t const bs = pe-pc;
										meta.addBlock(std::pair<uint8_t *,uint8_t *>(sblock->pc,sblock->pc+bs));
										sblock->pc += bs;
										f += 1;
									}

									break;
								}
								default:
								{
									libmaus2::exception::LibMausException lme;
									lme.getStream() << "GenericInputControlReadWorkPackageDispatcher: unknown parsing type" << std::endl;
									lme.finish();
									throw lme;
								}
							}

							#if 0
							// empty file? inject eof block
							if ( data.getEOF() && (sblock->pc == sblock->pe) && (!f) )
							{
								std::ostringstream ostr;
								libmaus2::lz::BgzfDeflate<std::ostream> bgzfout(ostr);
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
								switch ( parser_type )
								{
									case parse_bam:
									{
										libmaus2::exception::LibMausException lme;
										lme.getStream() << "libmaus2::bambam::parallel::GenericInputControlReadWorkPackageDispatcher: Invalid empty stream is invalid bgzf/BAM" << std::endl;
										lme.finish();
										throw lme;
									}
									case parse_sam:
									{
										// absolutely empty sam file
										meta.addBlock(std::pair<uint8_t *,uint8_t *>(sblock->pc,sblock->pe));
										f += 1;
										break;
									}
								}
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
									libmaus2::exception::LibMausException lme;
									lme.getStream() << "libmaus2::bambam::parallel::GenericInputControlReadWorkPackageDispatcher: Unexpected EOF." << std::endl;
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
