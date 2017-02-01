/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_LASTOBAMCONVERTERBASE_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_LASTOBAMCONVERTERBASE_HPP

#include <libmaus2/bambam/BamAlignment.hpp>
#include <libmaus2/bambam/parallel/FragmentAlignmentBufferFragment.hpp>
#include <libmaus2/dazzler/align/AlignmentFile.hpp>
#include <libmaus2/dazzler/align/OverlapData.hpp>
#include <libmaus2/lcs/AlignerFactory.hpp>
#include <libmaus2/lcs/DalignerLocalAlignment.hpp>
#include <libmaus2/lcs/EditDistanceTraceContainer.hpp>
#include <libmaus2/dazzler/align/RefMapEntryVector.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct LASToBamConverterBase
			{
				typedef LASToBamConverterBase this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				libmaus2::autoarray::AutoArray < std::pair<uint16_t,uint16_t> > Apath;

				#define USE_DALIGNER

				#if defined(LIBMAUS2_HAVE_DALIGNER) && defined(USE_DALIGNER)
				libmaus2::lcs::DalignerLocalAlignment DLA;
				#endif

				libmaus2::lcs::EditDistanceTraceContainer EATC;
				libmaus2::lcs::EditDistanceTraceContainer & ATC;
				libmaus2::lcs::Aligner::unique_ptr_type Pal;
				libmaus2::lcs::Aligner & ND;
				::libmaus2::fastx::UCharBuffer sbuffer;
				::libmaus2::fastx::UCharBuffer tbuffer;
				::libmaus2::bambam::MdStringComputationContext context;
				::libmaus2::bambam::BamSeqEncodeTable const seqenc;

				int64_t const tspace;
				bool const small;

				bool const calmdnm;

				libmaus2::autoarray::AutoArray<std::pair<libmaus2::lcs::AlignmentTraceContainer::step_type,uint64_t> > Aopblocks;
				libmaus2::autoarray::AutoArray<libmaus2::bambam::cigar_operation> Acigop;
				libmaus2::autoarray::AutoArray<uint8_t> ASQ;

				enum supplementary_seq_strategy_t {
					supplementary_seq_strategy_soft,
					supplementary_seq_strategy_hard,
					supplementary_seq_strategy_none
				};

				supplementary_seq_strategy_t const supplementary_seq_strategy;

				std::string const rgid;

				libmaus2::dazzler::align::RefMapEntryVector const & refmap;

				static libmaus2::lcs::Aligner::unique_ptr_type constructAligner()
				{
					std::set<libmaus2::lcs::AlignerFactory::aligner_type> const S = libmaus2::lcs::AlignerFactory::getSupportedAligners();

					if ( S.find(libmaus2::lcs::AlignerFactory::libmaus2_lcs_AlignerFactory_Daligner_NP) != S.end() )
					{
						libmaus2::lcs::Aligner::unique_ptr_type T(libmaus2::lcs::AlignerFactory::construct(libmaus2::lcs::AlignerFactory::libmaus2_lcs_AlignerFactory_Daligner_NP));
						return UNIQUE_PTR_MOVE(T);
					}
					else if ( S.find(libmaus2::lcs::AlignerFactory::libmaus2_lcs_AlignerFactory_y256_8) != S.end() )
					{
						libmaus2::lcs::Aligner::unique_ptr_type T(libmaus2::lcs::AlignerFactory::construct(libmaus2::lcs::AlignerFactory::libmaus2_lcs_AlignerFactory_y256_8));
						return UNIQUE_PTR_MOVE(T);
					}
					else if ( S.find(libmaus2::lcs::AlignerFactory::libmaus2_lcs_AlignerFactory_x128_8) != S.end() )
					{
						libmaus2::lcs::Aligner::unique_ptr_type T(libmaus2::lcs::AlignerFactory::construct(libmaus2::lcs::AlignerFactory::libmaus2_lcs_AlignerFactory_x128_8));
						return UNIQUE_PTR_MOVE(T);
					}
					else if ( S.find(libmaus2::lcs::AlignerFactory::libmaus2_lcs_AlignerFactory_NP) != S.end() )
					{
						libmaus2::lcs::Aligner::unique_ptr_type T(libmaus2::lcs::AlignerFactory::construct(libmaus2::lcs::AlignerFactory::libmaus2_lcs_AlignerFactory_NP));
						return UNIQUE_PTR_MOVE(T);
					}
					else if ( S.size() )
					{
						libmaus2::lcs::Aligner::unique_ptr_type T(libmaus2::lcs::AlignerFactory::construct(*(S.begin())));
						return UNIQUE_PTR_MOVE(T);
					}
					else
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "LASToBAMConverter::constructAligner: no aligners found" << std::endl;
						lme.finish();
						throw lme;
					}
				}

				LASToBamConverterBase(
					int64_t const rtspace,
					bool const rcalmdnm,
					supplementary_seq_strategy_t const rsupplementaryStrategy,
					std::string const & rrgid,
					libmaus2::dazzler::align::RefMapEntryVector const & rrefmap
				)
				:
				  Apath(),
				  #if defined(LIBMAUS2_HAVE_DALIGNER) && defined(USE_DALIGNER)
				  DLA(),
				  EATC(),
				  ATC(DLA),
				  #else
				  EATC(),
				  ATC(EATC),
				  #endif
				  Pal(constructAligner()),
				  ND(*Pal),
				  tspace(rtspace),
				  small(libmaus2::dazzler::align::AlignmentFile::tspaceToSmall(tspace)),
				  calmdnm(rcalmdnm),
				  supplementary_seq_strategy(rsupplementaryStrategy),
				  rgid(rrgid),
				  refmap(rrefmap)
				{

				}

				void computeTrace(
					// the overlap
					uint8_t const * OVL,
					// ref
					char const * aptr,
					// ref length
					uint64_t const reflen,
					// read
					char const * bptr,
					//
					uint64_t const readlen
				)
				{
					int64_t const abpos = libmaus2::dazzler::align::OverlapData::getABPos(OVL);
					int64_t const aepos = libmaus2::dazzler::align::OverlapData::getAEPos(OVL);
					int64_t const bbpos = libmaus2::dazzler::align::OverlapData::getBBPos(OVL);
					int64_t const bepos = libmaus2::dazzler::align::OverlapData::getBEPos(OVL);
					uint64_t const tracelen = libmaus2::dazzler::align::OverlapData::decodeTraceVector(OVL,Apath,small);

					#if defined(LIBMAUS2_HAVE_DALIGNER) && defined(USE_DALIGNER)
					/* LocalEditDistanceResult const LEDR = */ DLA.computeDenseTracePreMapped(
						reinterpret_cast<uint8_t const *>(aptr),reflen,
						reinterpret_cast<uint8_t const *>(bptr),readlen,
						tspace,
						Apath.begin(),tracelen,
						libmaus2::dazzler::align::OverlapData::getDiffs(OVL),
						abpos,
						bbpos,
						aepos,
						bepos);
					#else
					// compute dense alignment trace
					libmaus2::dazzler::align::Overlap::computeTracePreMapped(Apath.begin(),tracelen,abpos,aepos,bbpos,bepos,reinterpret_cast<uint8_t const *>(aptr),reinterpret_cast<uint8_t const *>(bptr),tspace,ATC,ND);
					#endif
				}

				struct AuxTagAddRequest
				{
					virtual ~AuxTagAddRequest() {}
					virtual void operator()(::libmaus2::fastx::UCharBuffer & tbuffer) const = 0;
				};

				struct AuxTagIntegerAddRequest : public AuxTagAddRequest
				{
					char const * tag;
					int32_t v;

					AuxTagIntegerAddRequest() : tag(0), v(-1) {}
					AuxTagIntegerAddRequest(char const * rtag, int32_t const rv) : tag(rtag), v(rv) {}

					virtual void operator()(::libmaus2::fastx::UCharBuffer & tbuffer) const
					{
						libmaus2::bambam::BamAlignmentEncoderBase::putAuxNumber(tbuffer,tag,'i',v);
					}
				};

				struct AuxTagStringAddRequest : public AuxTagAddRequest
				{
					char const * tag;
					char const * v;

					AuxTagStringAddRequest() : tag(0), v(0) {}
					AuxTagStringAddRequest(char const * rtag, char const * rv) : tag(rtag), v(rv) {}

					virtual void operator()(::libmaus2::fastx::UCharBuffer & tbuffer) const
					{
						libmaus2::bambam::BamAlignmentEncoderBase::putAuxString(tbuffer,tag,v);
					}
				};

				struct AuxTagCopyAddRequest : public AuxTagAddRequest
				{
					uint8_t const * d;
					uint64_t l;

					AuxTagCopyAddRequest() : d(0), l(0) {}
					AuxTagCopyAddRequest(
						uint8_t const * rd,
						uint64_t const rl
					) : d(rd), l(rl) {}

					virtual void operator()(::libmaus2::fastx::UCharBuffer & tbuffer) const
					{
						tbuffer.put(d,l);
					}
				};

				void convert(
					// the overlap
					uint8_t const * OVL,
					// ref
					char const * aptr,
					// length of ref seq
					uint64_t const aseqlen,
					// read
					char const * bptr,
					// read length
					uint64_t const readlen,
					// readname
					char const * readname,
					// buffer for storing bam record,
					libmaus2::bambam::parallel::FragmentAlignmentBufferFragment & FABR,
					// is this a secondary alignment?
					bool const secondary,
					// is this a supplementary alignment?
					bool const supplementary,
					// header
					libmaus2::bambam::BamHeader const & bamheader,
					// aux tag add requests
					AuxTagAddRequest const ** aux_a,
					// aux tag add requests end
					AuxTagAddRequest const ** aux_e
				)
				{
					bool const primary =
						(!secondary) && (!supplementary);

					int64_t const aread = libmaus2::dazzler::align::OverlapData::getARead(OVL);
					// int64_t const bread = libmaus2::dazzler::align::OverlapData::getBRead(OVL);
					bool const bIsInverse = libmaus2::dazzler::align::OverlapData::getFlags(OVL) & 1;

					int64_t const abpos = libmaus2::dazzler::align::OverlapData::getABPos(OVL);
					#if defined(LASTOBAM_ALIGNMENT_PRINT_DEBUG)
					int64_t const aepos = libmaus2::dazzler::align::OverlapData::getAEPos(OVL);
					#endif
					int64_t const bbpos = libmaus2::dazzler::align::OverlapData::getBBPos(OVL);
					int64_t const bepos = libmaus2::dazzler::align::OverlapData::getBEPos(OVL);

					int32_t const blen   = (bepos - bbpos);
					int32_t const bclipleft = bbpos;
					int32_t const bclipright = readlen - blen - bclipleft;

					computeTrace(OVL,aptr,aseqlen,bptr,readlen);

					// compute cigar operation blocks based on alignment trace
					size_t const nopblocks = ATC.getOpBlocks(Aopblocks);
					// compute final number of cigar operations based on clipping information
					size_t const cigopblocks = nopblocks + (bclipleft?1:0) + (bclipright?1:0);

					// reallocate cigar operation vector if necessary
					if ( cigopblocks > Acigop.size() )
						Acigop.resize(cigopblocks);
					uint64_t cigp = 0;

					int64_t as = 0;
					if ( bclipleft )
					{
						if ( primary )
							Acigop[cigp++] = libmaus2::bambam::cigar_operation(libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CSOFT_CLIP,bclipleft);
						else
						{
							switch ( supplementary_seq_strategy )
							{
								case supplementary_seq_strategy_soft:
									Acigop[cigp++] = libmaus2::bambam::cigar_operation(libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CSOFT_CLIP,bclipleft);
									break;
								case supplementary_seq_strategy_hard:
								case supplementary_seq_strategy_none:
									Acigop[cigp++] = libmaus2::bambam::cigar_operation(libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CHARD_CLIP,bclipleft);
									break;
							}
						}
					}
					for ( uint64_t i = 0; i < nopblocks; ++i )
					{
						switch ( Aopblocks[i].first )
						{
							case libmaus2::lcs::AlignmentTraceContainer::STEP_MATCH:
								Acigop[cigp++] = libmaus2::bambam::cigar_operation(libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CEQUAL,Aopblocks[i].second);
								as += Aopblocks[i].second;
								break;
							case libmaus2::lcs::AlignmentTraceContainer::STEP_MISMATCH:
								Acigop[cigp++] = libmaus2::bambam::cigar_operation(libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CDIFF,Aopblocks[i].second);
								as -= static_cast<int64_t>(Aopblocks[i].second);
								break;
							case libmaus2::lcs::AlignmentTraceContainer::STEP_INS:
								Acigop[cigp++] = libmaus2::bambam::cigar_operation(libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CINS,Aopblocks[i].second);
								as -= static_cast<int64_t>(Aopblocks[i].second);
								break;
							case libmaus2::lcs::AlignmentTraceContainer::STEP_DEL:
								Acigop[cigp++] = libmaus2::bambam::cigar_operation(libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CDEL,Aopblocks[i].second);
								as -= static_cast<int64_t>(Aopblocks[i].second);
								break;
							default:
								break;
						}
					}
					if ( bclipright )
					{
						if ( primary )
							Acigop[cigp++] = libmaus2::bambam::cigar_operation(libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CSOFT_CLIP,bclipright);
						else
						{
							switch ( supplementary_seq_strategy )
							{
								case supplementary_seq_strategy_soft:
									Acigop[cigp++] = libmaus2::bambam::cigar_operation(libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CSOFT_CLIP,bclipright);
									break;
								case supplementary_seq_strategy_hard:
								case supplementary_seq_strategy_none:
									Acigop[cigp++] = libmaus2::bambam::cigar_operation(libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CHARD_CLIP,bclipright);
									break;
							}
						}
					}

					// reallocate quality value buffer if necessary
					if ( readlen > ASQ.size() )
					{
						ASQ.resize(readlen);
						std::fill(ASQ.begin(),ASQ.end(),255);
					}

					// length of stored sequence
					uint64_t storeseqlen;

					if ( primary )
					{
						storeseqlen = readlen;
					}
					else
					{
						switch ( supplementary_seq_strategy )
						{
							// hard clip
							case supplementary_seq_strategy_hard:
								storeseqlen = readlen - (bclipleft+bclipright);
								break;
							// do not store
							case supplementary_seq_strategy_none:
								storeseqlen = 0;
								break;
							// soft clip
							case supplementary_seq_strategy_soft:
							default:
								storeseqlen = readlen;
								break;
						}
					}

					char const * readnamee = readname;

					while ( *readnamee && !isspace(*readnamee) )
						++readnamee;

					libmaus2::bambam::BamAlignmentEncoderBase::encodeAlignmentPreMapped(
						tbuffer,
						// seqenc,
						readname,
						readnamee-readname,
						refmap.at(aread).refid, // ref id
						refmap.at(aread).offset + abpos, // pos
						255, // mapq (none given)
						(bIsInverse ? libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREVERSE : 0)
						|
						(secondary ? libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FSECONDARY : 0)
						|
						(supplementary ? libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FSUPPLEMENTARY : 0)
						,
						Acigop.begin(),
						cigp,
						-1, // next ref id
						-1, // next pos
						0, // template length (not given)
						primary ? bptr : (bptr + ((supplementary_seq_strategy==supplementary_seq_strategy_hard) ? bclipleft : 0)),
						storeseqlen,
						ASQ.begin(), /* quality (not given) */
						0, /* quality offset */
						true, /* reset buffer */
						false /* encode rc */
					);

					libmaus2::bambam::libmaus2_bambam_alignment_validity val = libmaus2::bambam::BamAlignmentDecoderBase::valid(tbuffer.begin(),tbuffer.end() - tbuffer.begin());

					if ( val != libmaus2::bambam::libmaus2_bambam_alignment_validity_ok )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "LASToBAMConverter: constructed alignment is invalid: " << val << std::endl;
						::libmaus2::bambam::BamFormatAuxiliary auxdata;
						libmaus2::bambam::BamAlignmentDecoderBase::formatAlignment(lme.getStream(),tbuffer.begin(),tbuffer.end() - tbuffer.begin(),bamheader,auxdata);
						lme.getStream() << std::endl;
						lme.finish();
						throw lme;
					}

					if ( calmdnm )
					{
						try
						{
							if ( primary || (supplementary_seq_strategy != supplementary_seq_strategy_none) )
							{
								libmaus2::bambam::BamAlignmentDecoderBase::calculateMdMapped(
									tbuffer.begin(),tbuffer.end()-(tbuffer.begin()),
									context,aptr + abpos,false
								);
							}
							else
							{
								sbuffer.reset();
								libmaus2::bambam::BamAlignmentEncoderBase::encodeSeqPreMapped(sbuffer,bptr+bclipleft,readlen-(bclipleft+bclipright));

								libmaus2::bambam::BamAlignmentDecoderBase::calculateMdMapped(
									tbuffer.begin(),tbuffer.end()-(tbuffer.begin()),
									context,aptr + abpos,
									sbuffer.begin(),
									readlen-(bclipleft+bclipright),
									false
								);
							}
							libmaus2::bambam::BamAlignmentEncoderBase::putAuxString(tbuffer,"MD",context.md.get());
							libmaus2::bambam::BamAlignmentEncoderBase::putAuxNumber(tbuffer,"NM",'i',context.nm);
							libmaus2::bambam::BamAlignmentEncoderBase::putAuxNumber(tbuffer,"AS",'i',as);
						}
						catch(std::exception const & ex)
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "LASToBAMConverter: calmdnm exception for read " << readname << "\n" << ex.what() << std::endl;
							::libmaus2::bambam::BamFormatAuxiliary auxdata;
							libmaus2::bambam::BamAlignmentDecoderBase::formatAlignment(lme.getStream(),tbuffer.begin(),tbuffer.end() - tbuffer.begin(),bamheader,auxdata);
							lme.finish();
							throw lme;
						}
					}

					if ( rgid.size() )
						libmaus2::bambam::BamAlignmentEncoderBase::putAuxString(tbuffer,"RG",rgid.c_str());

					for ( AuxTagAddRequest const ** aux_c = aux_a; aux_c != aux_e; ++aux_c )
						(**aux_c)(tbuffer);

					libmaus2::math::IntegerInterval<int64_t> const covered = libmaus2::bambam::BamAlignmentDecoderBase::getCoveredReadInterval(tbuffer.begin());

					bool const c_ok = (covered.from == bbpos) && (covered.to+1 == bepos);

					if ( ! c_ok )
					{
						std::ostringstream ostr;
						ostr << "covered " << covered << " expected " << bbpos << "," << bepos << std::endl;
						ostr << "front clipping " << libmaus2::bambam::BamAlignmentDecoderBase::getFrontClipping(tbuffer.begin()) << std::endl;
						ostr << "back clipping " << libmaus2::bambam::BamAlignmentDecoderBase::getBackClipping(tbuffer.begin()) << std::endl;
						ostr << "lseq " << libmaus2::bambam::BamAlignmentDecoderBase::getLseq(tbuffer.begin()) << std::endl;
						ostr << "from " << covered.from << std::endl;
						ostr << "to " << covered.to << std::endl;

						::libmaus2::bambam::BamFormatAuxiliary auxdata;
						libmaus2::bambam::BamAlignmentDecoderBase::formatAlignment(ostr,tbuffer.begin(),tbuffer.end() - tbuffer.begin(),bamheader,auxdata);
						ostr.put('\n');

						std::string const s = ostr.str();
						char const * c = s.c_str();
						std::cerr.write(c,s.size());

						assert ( c_ok );
					}

					FABR.pushAlignmentBlock(tbuffer.begin(),tbuffer.end() - tbuffer.begin());

					#if defined(LASTOBAM_ALIGNMENT_PRINT_DEBUG)
					// print alignment if requested
					if ( printAlignments )
					{
						libmaus2::lcs::AlignmentStatistics const AS = ATC.getAlignmentStatistics();

						if ( AS.getErrorRate() <= eratelimit )
						{
							std::cout << OVL << std::endl;
							std::cout << AS << std::endl;
							ATC.printAlignmentLines(std::cout,aptr + abpos,aepos-abpos,bptr + bbpos,bepos-bbpos,80);
						}
					}
					#endif

					#if defined(LASTOBAM_PRINT_SAM)
					::libmaus2::bambam::BamFormatAuxiliary auxdata;
					libmaus2::bambam::BamAlignmentDecoderBase::formatAlignment(std::cerr,tbuffer.begin(),tbuffer.end() - tbuffer.begin(),bamheader,auxdata);
					std::cerr << std::endl;
					#endif

				}

				void convertUnmapped(
					// read
					char const * bptr,
					// read length
					uint64_t const readlen,
					// readname
					char const * readname,
					// buffer for storing bam record,
					libmaus2::bambam::parallel::FragmentAlignmentBufferFragment & FABR,
					// header
					libmaus2::bambam::BamHeader const & bamheader,
					// aux tag add requests
					AuxTagAddRequest const ** aux_a,
					// aux tag add requests end
					AuxTagAddRequest const ** aux_e
				)
				{
					// reallocate quality value buffer if necessary
					if ( readlen > ASQ.size() )
					{
						ASQ.resize(readlen);
						std::fill(ASQ.begin(),ASQ.end(),255);
					}

					char const * readnamee = readname;
					while ( *readnamee && !isspace(*readnamee) )
						++readnamee;

					libmaus2::bambam::BamAlignmentEncoderBase::encodeAlignmentPreMapped(
						tbuffer,
						readname,
						readnamee-readname,
						-1, // ref id
						-1, // pos
						255, // mapq (none given)
						libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FUNMAP,
						Acigop.begin(),
						0,
						-1, // next ref id
						-1, // next pos
						0, // template length (not given)
						bptr,
						readlen,
						ASQ.begin(), /* quality (not given) */
						0, /* quality offset */
						true, /* reset buffer */
						false /* encode rc */
					);

					libmaus2::bambam::libmaus2_bambam_alignment_validity val = libmaus2::bambam::BamAlignmentDecoderBase::valid(tbuffer.begin(),tbuffer.end() - tbuffer.begin());

					if ( val != libmaus2::bambam::libmaus2_bambam_alignment_validity_ok )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "LASToBAMConverter: constructed alignment is invalid: " << val << std::endl;
						::libmaus2::bambam::BamFormatAuxiliary auxdata;
						libmaus2::bambam::BamAlignmentDecoderBase::formatAlignment(lme.getStream(),tbuffer.begin(),tbuffer.end() - tbuffer.begin(),bamheader,auxdata);
						lme.getStream() << std::endl;
						lme.finish();
						throw lme;
					}

					if ( rgid.size() )
						libmaus2::bambam::BamAlignmentEncoderBase::putAuxString(tbuffer,"RG",rgid.c_str());

					for ( AuxTagAddRequest const ** aux_c = aux_a; aux_c != aux_e; ++aux_c )
						(**aux_c)(tbuffer);

					FABR.pushAlignmentBlock(tbuffer.begin(),tbuffer.end() - tbuffer.begin());

					#if defined(LASTOBAM_PRINT_SAM)
					::libmaus2::bambam::BamFormatAuxiliary auxdata;
					libmaus2::bambam::BamAlignmentDecoderBase::formatAlignment(std::cerr,tbuffer.begin(),tbuffer.end() - tbuffer.begin(),bamheader,auxdata);
					std::cerr << std::endl;
					#endif

				}
			};
		}
	}
}
#endif
