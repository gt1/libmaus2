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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_FRAGMENTALIGNMENTBUFFERREWRITEWORKPACKAGEDISPATCHER_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_FRAGMENTALIGNMENTBUFFERREWRITEWORKPACKAGEDISPATCHER_HPP

#include <libmaus/bambam/parallel/FragmentAlignmentBufferRewriteWorkPackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferRewriteFragmentCompleteInterface.hpp>
#include <libmaus/bambam/parallel/FragmentAlignmentBufferRewriteWorkPackage.hpp>
#include <libmaus/parallel/SimpleThreadWorkPackageDispatcher.hpp>
#include <libmaus/bambam/BamAuxFilterVector.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{

			/**
			 * block rewrite dispatcher
			 **/
			struct FragmentAlignmentBufferRewriteWorkPackageDispatcher : public libmaus::parallel::SimpleThreadWorkPackageDispatcher
			{
				FragmentAlignmentBufferRewriteWorkPackageReturnInterface & packageReturnInterface;
				FragmentAlignmentBufferRewriteFragmentCompleteInterface & fragmentCompleteInterface;
				libmaus::bambam::BamAuxFilterVector MQfilter;
				bool const fixmates;
				libmaus::bambam::BamAuxFilterVector MCMSMTfilter;
				bool const dupmarksupport;
				libmaus::bambam::BamAuxFilterVector MQMCMSMTfilter;
				std::string const tagtag;
				char const * ctagtag;
				
				FragmentAlignmentBufferRewriteWorkPackageDispatcher(
					FragmentAlignmentBufferRewriteWorkPackageReturnInterface & rpackageReturnInterface,
					FragmentAlignmentBufferRewriteFragmentCompleteInterface & rfragmentCompleteInterface
				) 
				: 
					packageReturnInterface(rpackageReturnInterface), fragmentCompleteInterface(rfragmentCompleteInterface), fixmates(true),
					dupmarksupport(true), tagtag("TA"), ctagtag(tagtag.c_str())
				{
					MQfilter.set("MQ");
					
					MCMSMTfilter.set("MC");
					MCMSMTfilter.set("MS");
					MCMSMTfilter.set("MT");

					MQMCMSMTfilter.set("MQ");
					MQMCMSMTfilter.set("MC");
					MQMCMSMTfilter.set("MS");
					MQMCMSMTfilter.set("MT");
				}
			
				virtual void dispatch(
					libmaus::parallel::SimpleThreadWorkPackage * P, 
					libmaus::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */
				)
				{
					// get type cast work package pointer
					FragmentAlignmentBufferRewriteWorkPackage * BP = dynamic_cast<FragmentAlignmentBufferRewriteWorkPackage *>(P);
					assert ( BP );
					
					// dispatch
					uint64_t * O = BP->FAB->getOffsetStart(BP->j);
					uint64_t const ind = BP->FAB->getOffsetStartIndex(BP->j);
					size_t const num = BP->FAB->getNumAlignments(BP->j);
					FragmentAlignmentBufferFragment * subbuf = (*(BP->FAB))[BP->j];
					size_t looplow  = ind;
					size_t const loopend = ind+num;
					
					libmaus::autoarray::AutoArray<uint8_t> ATA1;
					libmaus::autoarray::AutoArray<uint8_t> ATA2;
					// 2(Tag) + 1(Z) + String + nul = String + 4

					while ( looplow < loopend )
					{
						std::pair<uint8_t *,uint64_t> Pref = BP->algn->at(looplow);
						char const * refname = ::libmaus::bambam::BamAlignmentDecoderBase::getReadName(Pref.first);

						size_t loophigh = looplow+1;
						// increase until we reach the end or find a different read name
						while ( 
							loophigh < loopend &&
							strcmp(
								refname,
								::libmaus::bambam::BamAlignmentDecoderBase::getReadName(BP->algn->textAt(loophigh))
							) == 0
						)
						{
							++loophigh;
						}

						std::pair<int16_t,int16_t> MQP(-1,-1);
						std::pair<int64_t,int64_t> MSP(-1,-1);
						std::pair<int64_t,int64_t> MCP(-1,-1);
						ssize_t firsti = -1;
						ssize_t secondi = -1;
						size_t ATA1len = 0;
						size_t ATA2len = 0;

						if ( fixmates || dupmarksupport )
							for ( size_t i = looplow; i < loophigh; ++i )
							{
								uint8_t const * text = BP->algn->textAt(i);
								uint32_t const flags = ::libmaus::bambam::BamAlignmentDecoderBase::getFlags(text);
								
								if ( 
									(flags & libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FSUPPLEMENTARY) == 0 &&
									(flags & libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FSECONDARY) == 0
								)
								{
									if (
										(flags & libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FREAD1)
										&&
										!(flags & libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FREAD2)
									)
									{
										firsti = i;
									}
									else if (
										(flags & libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FREAD2)
										&&
										!(flags & libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FREAD1)
									)
									{
										secondi = i;
									}
								}
							}
						
						if ( fixmates )
						{
							if ( firsti >= 0 && secondi >= 0 )
							{
								MQP = libmaus::bambam::BamAlignmentEncoderBase::fixMateInformation(
									BP->algn->textAt(firsti),
									BP->algn->textAt(secondi)
								);
							}
						}
						
						if ( dupmarksupport )
						{
							if ( firsti >= 0 && secondi >= 0 )
							{
								std::pair<uint8_t *,uint64_t> Pfirst = BP->algn->at(firsti);
								std::pair<uint8_t *,uint64_t> Psecond = BP->algn->at(secondi);
								
								MSP.first = ::libmaus::bambam::BamAlignmentDecoderBase::getScore(Psecond.first);
								MSP.second = ::libmaus::bambam::BamAlignmentDecoderBase::getScore(Pfirst.first);
								MCP.first = ::libmaus::bambam::BamAlignmentDecoderBase::getCoordinate(Psecond.first);
								MCP.second = ::libmaus::bambam::BamAlignmentDecoderBase::getCoordinate(Pfirst.first);
								
								char const * TA1 = ::libmaus::bambam::BamAlignmentDecoderBase::getAuxString(
									Psecond.first, Psecond.second, ctagtag);
								char const * TA2 = ::libmaus::bambam::BamAlignmentDecoderBase::getAuxString(
									Pfirst.first, Pfirst.second, ctagtag);
									
								if ( TA1 )
								{
									size_t const len = strlen(TA1);
									if ( len + 4 > ATA1.size() )
										ATA1.resize(len+4);
									ATA1[0] = 'M';
									ATA1[1] = 'T';
									ATA1[2] = 'Z';
									std::copy(TA1,TA1+len,ATA1.begin()+3);
									ATA1[len+3] = 0;
									ATA1len = len+4;
								}
								if ( TA2 )
								{
									size_t const len = strlen(TA2);
									if ( len + 4 > ATA2.size() )
										ATA2.resize(len+4);
									ATA2[0] = 'M';
									ATA2[1] = 'T';
									ATA2[2] = 'Z';
									std::copy(TA2,TA2+len,ATA2.begin()+3);
									ATA2[len+3] = 0;
									ATA2len = len+4;
								}
							}
						}

						for ( size_t i = looplow; i < loophigh; ++i )
						{
							std::pair<uint8_t *,uint64_t> P = BP->algn->at(i);

							if ( fixmates && dupmarksupport )
							{							
								P.second = ::libmaus::bambam::BamAlignmentDecoderBase::filterOutAux(P.first,P.second,MQMCMSMTfilter);
								BP->algn->setLengthAt(i,P.second);
							}
							else if ( fixmates )
							{
								P.second = ::libmaus::bambam::BamAlignmentDecoderBase::filterOutAux(P.first,P.second,MQfilter);
								BP->algn->setLengthAt(i,P.second);
							}
							else if ( dupmarksupport )
							{								
								P.second = ::libmaus::bambam::BamAlignmentDecoderBase::filterOutAux(P.first,P.second,MCMSMTfilter);
								BP->algn->setLengthAt(i,P.second);
							}
							
							uint64_t const offset = subbuf->getOffset();
							*(O++) = offset;
							subbuf->pushAlignmentBlock(P.first,P.second);
							
							if ( static_cast<ssize_t>(i) == firsti )
							{
								if ( MQP.first >= 0 )
								{
									uint8_t const T[4] = { 'M', 'Q', 'C', static_cast<uint8_t>(MQP.first) };
									subbuf->push(&T[0],sizeof(T));
									P.second += sizeof(T);
									subbuf->replaceLength(offset,P.second);
								}
								if ( MSP.first >= 0 )
								{
									uint8_t const T[7] = { 
										'M', 'S', 'I', 
										static_cast<uint8_t>((MSP.first >>  0) & 0xFF),
										static_cast<uint8_t>((MSP.first >>  8) & 0xFF),
										static_cast<uint8_t>((MSP.first >> 16) & 0xFF),
										static_cast<uint8_t>((MSP.first >> 24) & 0xFF)
									};
									subbuf->push(&T[0],sizeof(T));
									P.second += sizeof(T);
									subbuf->replaceLength(offset,P.second);
								}
								if ( MCP.first >= 0 )
								{
									uint8_t const T[7] = { 
										'M', 'C', 'I', 
										static_cast<uint8_t>((MCP.first >>  0) & 0xFF),
										static_cast<uint8_t>((MCP.first >>  8) & 0xFF),
										static_cast<uint8_t>((MCP.first >> 16) & 0xFF),
										static_cast<uint8_t>((MCP.first >> 24) & 0xFF)
									};
									subbuf->push(&T[0],sizeof(T));
									P.second += sizeof(T);
									subbuf->replaceLength(offset,P.second);
								}
								if ( ATA1len )
								{
									subbuf->push(ATA1.begin(),ATA1len);
									P.second += ATA1len;
									subbuf->replaceLength(offset,P.second);
								}
							}
							else if ( static_cast<ssize_t>(i) == secondi )
							{
								if ( (MQP.second >= 0) )
								{
									uint8_t const T[4] = { 'M', 'Q', 'C', static_cast<uint8_t>(MQP.second) };
									subbuf->push(&T[0],sizeof(T));
									P.second += sizeof(T);
									subbuf->replaceLength(offset,P.second);							
								}
								if ( MSP.second >= 0 )
								{
									uint8_t const T[7] = { 
										'M', 'S', 'I', 
										static_cast<uint8_t>((MSP.second >>  0) & 0xFF),
										static_cast<uint8_t>((MSP.second >>  8) & 0xFF),
										static_cast<uint8_t>((MSP.second >> 16) & 0xFF),
										static_cast<uint8_t>((MSP.second >> 24) & 0xFF)
									};
									subbuf->push(&T[0],sizeof(T));
									P.second += sizeof(T);
									subbuf->replaceLength(offset,P.second);
								}
								if ( MCP.second >= 0 )
								{
									uint8_t const T[7] = { 
										'M', 'C', 'I', 
										static_cast<uint8_t>((MCP.second >>  0) & 0xFF),
										static_cast<uint8_t>((MCP.second >>  8) & 0xFF),
										static_cast<uint8_t>((MCP.second >> 16) & 0xFF),
										static_cast<uint8_t>((MCP.second >> 24) & 0xFF)
									};
									subbuf->push(&T[0],sizeof(T));
									P.second += sizeof(T);
									subbuf->replaceLength(offset,P.second);
								}
								if ( ATA2len )
								{
									subbuf->push(ATA2.begin(),ATA2len);
									P.second += ATA2len;
									subbuf->replaceLength(offset,P.second);
								}
							}
						}
						
						looplow = loophigh;
					}

					BP->FAB->rewritePointers(BP->j);
					// end of dispatch
	
					fragmentCompleteInterface.fragmentAlignmentBufferRewriteFragmentComplete(BP->algn,BP->FAB,BP->j);

					// return the work package
					packageReturnInterface.returnFragmentAlignmentBufferRewriteWorkPackage(BP);
				}		
			};
		}
	}
}
#endif
