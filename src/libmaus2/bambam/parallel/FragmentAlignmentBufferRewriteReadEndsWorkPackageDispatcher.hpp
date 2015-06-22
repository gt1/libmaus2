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
#if ! defined(LIBMAUS2_BAMBAM_PARALLEL_FRAGMENTALIGNMENTBUFFERREWRITEREADENDSWORKPACKAGEDISPATCHER_HPP)
#define LIBMAUS2_BAMBAM_PARALLEL_FRAGMENTALIGNMENTBUFFERREWRITEREADENDSWORKPACKAGEDISPATCHER_HPP

#include <libmaus2/bambam/parallel/FragmentAlignmentBufferRewriteUpdateInterval.hpp>
#include <libmaus2/bambam/parallel/FragmentAlignmentBufferRewriteReadEndsWorkPackageReturnInterface.hpp>
#include <libmaus2/bambam/parallel/FragmentAlignmentBufferRewriteFragmentCompleteInterface.hpp>
#include <libmaus2/bambam/parallel/FragmentAlignmentBufferRewriteReadEndsWorkPackage.hpp>
#include <libmaus2/bambam/parallel/ReadEndsContainerFreeListInterface.hpp>
#include <libmaus2/parallel/SimpleThreadWorkPackageDispatcher.hpp>
#include <libmaus2/bambam/BamAuxFilterVector.hpp>
#include <libmaus2/bambam/DuplicationMetrics.hpp>
#include <libmaus2/bambam/parallel/AddDuplicationMetricsInterface.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			/**
			 * block rewrite dispatcher
			 **/
			template<bool _create_dup_mark_info>
			struct FragmentAlignmentBufferRewriteReadEndsWorkPackageDispatcher : public libmaus2::parallel::SimpleThreadWorkPackageDispatcher
			{
				static bool const create_dup_mark_info = _create_dup_mark_info;
			
				FragmentAlignmentBufferRewriteReadEndsWorkPackageReturnInterface & packageReturnInterface;
				FragmentAlignmentBufferRewriteFragmentCompleteInterface & fragmentCompleteInterface;
				ReadEndsContainerFreeListInterface & readEndsContainerFreeListInterface;
				AddDuplicationMetricsInterface & addDuplicationMetricsInterface;
				
				libmaus2::bambam::BamAuxFilterVector MQfilter;
				bool const fixmates;
				libmaus2::bambam::BamAuxFilterVector MCMSMTfilter;
				bool const dupmarksupport;
				libmaus2::bambam::BamAuxFilterVector MQMCMSMTfilter;
				std::string const tagtag;
				char const * ctagtag;
				
				FragmentAlignmentBufferRewriteReadEndsWorkPackageDispatcher(
					FragmentAlignmentBufferRewriteReadEndsWorkPackageReturnInterface & rpackageReturnInterface,
					FragmentAlignmentBufferRewriteFragmentCompleteInterface & rfragmentCompleteInterface,
					ReadEndsContainerFreeListInterface & rreadEndsContainerFreeListInterface,
					AddDuplicationMetricsInterface & raddDuplicationMetricsInterface,
					bool const rfixmates,
					bool const rdupmarksupport
				) 
				: 
					packageReturnInterface(rpackageReturnInterface), 
					fragmentCompleteInterface(rfragmentCompleteInterface), 
					readEndsContainerFreeListInterface(rreadEndsContainerFreeListInterface),
					addDuplicationMetricsInterface(raddDuplicationMetricsInterface),
					fixmates(rfixmates),
					dupmarksupport(rdupmarksupport), 
					tagtag("TA"), ctagtag(tagtag.c_str())
				{
					MQfilter.set("MQ");
					
					MCMSMTfilter.set("mc");
					MCMSMTfilter.set("ms");
					MCMSMTfilter.set("mt");

					MQMCMSMTfilter.set("MQ");
					MQMCMSMTfilter.set("mc");
					MQMCMSMTfilter.set("ms");
					MQMCMSMTfilter.set("mt");
				}
			
				virtual void dispatch(
					libmaus2::parallel::SimpleThreadWorkPackage * P, 
					libmaus2::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */
				)
				{
					// get type cast work package pointer
					FragmentAlignmentBufferRewriteReadEndsWorkPackage * BP = dynamic_cast<FragmentAlignmentBufferRewriteReadEndsWorkPackage *>(P);
					assert ( BP );
					
					libmaus2::bambam::ReadEndsContainer::shared_ptr_type pairContainer;
					libmaus2::bambam::ReadEndsContainer::shared_ptr_type fragContainer;

					if ( create_dup_mark_info )
					{
						pairContainer = readEndsContainerFreeListInterface.getPairContainer();
						fragContainer = readEndsContainerFreeListInterface.getFragContainer();
					}
					
					// dispatch
					uint64_t * O = BP->FAB->getOffsetStart(BP->j);
					uint64_t const ind = BP->FAB->getOffsetStartIndex(BP->j);
					size_t const num = BP->FAB->getNumAlignments(BP->j);
					FragmentAlignmentBufferFragment * subbuf = (*(BP->FAB))[BP->j];
					size_t looplow  = ind;
					size_t const loopend = ind+num;
					
					std::map<uint64_t,libmaus2::bambam::DuplicationMetrics> metrics;
					libmaus2::autoarray::AutoArray<uint8_t> ATA1;
					libmaus2::autoarray::AutoArray<uint8_t> ATA2;
					// 2(Tag) + 1(Z) + String + nul = String + 4
					
					while ( looplow < loopend )
					{
						std::pair<uint8_t *,uint64_t> Pref = BP->algn->at(looplow);
						char const * refname = ::libmaus2::bambam::BamAlignmentDecoderBase::getReadName(Pref.first);

						size_t loophigh = looplow+1;
						// increase until we reach the end or find a different read name
						while ( 
							loophigh < loopend &&
							strcmp(
								refname,
								::libmaus2::bambam::BamAlignmentDecoderBase::getReadName(BP->algn->textAt(loophigh))
							) == 0
						)
						{
							++loophigh;
						}
						
						// compute library id
						int64_t libid = -1;
						{
							std::pair<uint8_t *,uint64_t> P = BP->algn->at(looplow);						
							char const * RG = libmaus2::bambam::BamAlignmentDecoderBase::getReadGroup(P.first,P.second);
							libid = BP->header->getLibraryId(RG);
						}

						// compute dup metrics
						for ( uint64_t i = looplow; i < loophigh; ++i )
						{
							uint8_t const * text = BP->algn->textAt(i);
							uint32_t const flags = ::libmaus2::bambam::BamAlignmentDecoderBase::getFlags(text);

							if ( 
								(flags & libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FSUPPLEMENTARY) == 0 &&
								(flags & libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FSECONDARY) == 0
							)
							{
								if ( flags & libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FUNMAP )
								{
									metrics[libid].unmapped++;
								}
								else if ( 
									(!(flags & libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FPAIRED))
									||
									(flags & libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FMUNMAP)
								)
								{
									metrics[libid].unpaired++;
								}
							}
						}

						std::pair<int16_t,int16_t> MQP(-1,-1);
						std::pair<int64_t,int64_t> MSP(-1,-1);
						std::pair<int64_t,int64_t> MCP(-1,-1);
						ssize_t firsti = -1;
						ssize_t secondi = -1;
						int64_t firsto = -1;
						int64_t secondo = -1;
						size_t ATA1len = 0;
						size_t ATA2len = 0;

						// look for first and second primary alignment
						if ( fixmates || dupmarksupport )
							for ( size_t i = looplow; i < loophigh; ++i )
							{
								uint8_t const * text = BP->algn->textAt(i);
								uint32_t const flags = ::libmaus2::bambam::BamAlignmentDecoderBase::getFlags(text);
								
								if ( 
									(flags & libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FSUPPLEMENTARY) == 0 &&
									(flags & libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FSECONDARY) == 0
								)
								{
									if (
										(flags & libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREAD1)
										&&
										!(flags & libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREAD2)
									)
									{
										firsti = i;
									}
									else if (
										(flags & libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREAD2)
										&&
										!(flags & libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREAD1)
									)
									{
										secondi = i;
									}
								}
							}
						
						// run fixmates
						if ( fixmates )
						{
							if ( firsti >= 0 && secondi >= 0 )
							{
								MQP = libmaus2::bambam::BamAlignmentEncoderBase::fixMateInformation(
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
								
								MSP.first = ::libmaus2::bambam::BamAlignmentDecoderBase::getScore(Psecond.first);
								MSP.second = ::libmaus2::bambam::BamAlignmentDecoderBase::getScore(Pfirst.first);
								MCP.first = ::libmaus2::bambam::BamAlignmentDecoderBase::getCoordinate(Psecond.first);
								MCP.second = ::libmaus2::bambam::BamAlignmentDecoderBase::getCoordinate(Pfirst.first);
								
								char const * TA1 = ::libmaus2::bambam::BamAlignmentDecoderBase::getAuxString(
									Psecond.first, Psecond.second, ctagtag);
								char const * TA2 = ::libmaus2::bambam::BamAlignmentDecoderBase::getAuxString(
									Pfirst.first, Pfirst.second, ctagtag);
									
								if ( TA1 )
								{
									size_t const len = strlen(TA1);
									if ( len + 4 > ATA1.size() )
										ATA1.resize(len+4);
									ATA1[0] = 'm';
									ATA1[1] = 't';
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
									ATA2[0] = 'm';
									ATA2[1] = 't';
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
							uint64_t const rank = BP->algn->low + i;

							if ( fixmates && dupmarksupport )
							{							
								P.second = ::libmaus2::bambam::BamAlignmentDecoderBase::filterOutAux(P.first,P.second,MQMCMSMTfilter);
								BP->algn->setLengthAt(i,P.second);
							}
							else if ( fixmates )
							{
								P.second = ::libmaus2::bambam::BamAlignmentDecoderBase::filterOutAux(P.first,P.second,MQfilter);
								BP->algn->setLengthAt(i,P.second);
							}
							else if ( dupmarksupport )
							{								
								P.second = ::libmaus2::bambam::BamAlignmentDecoderBase::filterOutAux(P.first,P.second,MCMSMTfilter);
								BP->algn->setLengthAt(i,P.second);
							}
							
							uint64_t const offset = subbuf->getOffset();
							*(O++) = offset;
							subbuf->pushAlignmentBlock(P.first,P.second);
							
							if ( static_cast<ssize_t>(i) == firsti )
							{
								firsto = offset;
							
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
										'm', 's', 'I', 
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
										'm', 'c', 'I', 
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
								secondo = offset;
								
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
										'm', 's', 'I', 
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
										'm', 'c', 'I', 
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
							
							// add rank (line number in input file)
							uint8_t const T[4+4+8] = {
								'Z','R',
								// array
								'B',
								// number type (unsigned byte)
								'C',
								// length of number in bytes
								8,0,0,0,
								// number (BE)
								static_cast<uint8_t>((rank >> (7*8)) & 0xFF),
								static_cast<uint8_t>((rank >> (6*8)) & 0xFF),
								static_cast<uint8_t>((rank >> (5*8)) & 0xFF),
								static_cast<uint8_t>((rank >> (4*8)) & 0xFF),
								static_cast<uint8_t>((rank >> (3*8)) & 0xFF),
								static_cast<uint8_t>((rank >> (2*8)) & 0xFF),
								static_cast<uint8_t>((rank >> (1*8)) & 0xFF),
								static_cast<uint8_t>((rank >> (0*8)) & 0xFF)
							};
							subbuf->push(&T[0],sizeof(T));
							P.second += sizeof(T);
							subbuf->replaceLength(offset,P.second);
							
							uint8_t const * optr = subbuf->getPointer(offset);
							uint32_t const flags = libmaus2::bambam::BamAlignmentDecoderBase::getFlags(optr + sizeof(uint32_t));
							// is fragment relevant for duplicate marking?
							bool const relfrag = 
								!(
									flags & 
										(
											libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FUNMAP
											|
											libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FSECONDARY
											|
											libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FSUPPLEMENTARY
										)
								);
							if ( relfrag )
							{
								#if 0
								s_fragcnt++;
								#endif

								if ( create_dup_mark_info )
									fragContainer->putFrag(
										optr + sizeof(uint32_t),
										libmaus2::bambam::DecoderBase::getLEInteger(optr,sizeof(uint32_t)),
										*(BP->header)
									);
							}
						}

						uint8_t const * pfirst  = (firsto  >= 0) ? subbuf->getPointer(firsto) : 0;
						uint8_t const * psecond = (secondo >= 0) ? subbuf->getPointer(secondo) : 0;
						
						if ( pfirst )
						{
							uint32_t const flags = libmaus2::bambam::BamAlignmentDecoderBase::getFlags(pfirst + sizeof(uint32_t));
							if ( flags & libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FUNMAP )
								pfirst = 0;
						}
						if ( psecond )
						{
							uint32_t const flags = libmaus2::bambam::BamAlignmentDecoderBase::getFlags(psecond + sizeof(uint32_t));
							if ( flags & libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FUNMAP )
								psecond = 0;						
						}
						
						if ( pfirst && psecond )
						{
							metrics[libid].readpairsexamined++;
						
							int64_t const pfirstrefid = libmaus2::bambam::BamAlignmentDecoderBase::getRefID(pfirst + sizeof(uint32_t));
							int64_t const psecondrefid = libmaus2::bambam::BamAlignmentDecoderBase::getRefID(psecond + sizeof(uint32_t));
							int64_t const pfirstcoord = libmaus2::bambam::BamAlignmentDecoderBase::getCoordinate(pfirst + sizeof(uint32_t));
							int64_t const psecondcoord = libmaus2::bambam::BamAlignmentDecoderBase::getCoordinate(psecond + sizeof(uint32_t));
							
							if ( 
								(psecondrefid > pfirstrefid)
								||
								(
									(psecondrefid == pfirstrefid)
									&& 
									(psecondcoord >= pfirstcoord)
								)
							)
							{
							
							}
							else
							{
								std::swap(pfirst,psecond);
							}
						
							// std::cerr << libmaus2::bambam::BamAlignmentDecoderBase::getReadName(pfirst + sizeof(uint32_t)) << std::endl;
							if ( create_dup_mark_info )
							{
								pairContainer->putPair(
									pfirst + sizeof(uint32_t),
									libmaus2::bambam::DecoderBase::getLEInteger(pfirst,sizeof(uint32_t)),
									psecond + sizeof(uint32_t),
									libmaus2::bambam::DecoderBase::getLEInteger(psecond,sizeof(uint32_t)),
									*(BP->header)
								);
							}
						
							#if 0	
							s_paircnt++;
							#endif
						}
						
						looplow = loophigh;
					}

					addDuplicationMetricsInterface.addDuplicationMetrics(metrics);

					if ( create_dup_mark_info )
					{
						readEndsContainerFreeListInterface.returnPairContainer(pairContainer);
						readEndsContainerFreeListInterface.returnFragContainer(fragContainer);
					}

					BP->FAB->rewritePointers(BP->j);
					// end of dispatch

					fragmentCompleteInterface.fragmentAlignmentBufferRewriteFragmentComplete(BP->algn,BP->FAB,BP->j);

					// return the work package
					packageReturnInterface.returnFragmentAlignmentBufferRewriteReadEndsWorkPackage(BP);					
				}		
			};
		}
	}
}
#endif
