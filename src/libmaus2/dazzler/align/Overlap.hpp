/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_OVERLAP_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_OVERLAP_HPP

#include <libmaus2/dazzler/align/Path.hpp>
#include <libmaus2/lcs/Aligner.hpp>
#include <libmaus2/math/IntegerInterval.hpp>
#include <libmaus2/dazzler/align/TraceBlock.hpp>
#include <libmaus2/dazzler/align/TracePoint.hpp>
#include <libmaus2/fastx/acgtnMap.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct Overlap : public libmaus2::dazzler::db::InputBase, public libmaus2::dazzler::db::OutputBase
			{
				Path path;
				uint32_t flags;
				int32_t aread;
				int32_t bread;

				bool operator<(Overlap const & O) const
				{
					if ( aread != O.aread )
						return aread < O.aread;
					else if ( bread != O.bread )
						return bread < O.bread;
					else if ( isInverse() != O.isInverse() )
						return !isInverse();
					else if ( path.abpos != O.path.abpos )
						return path.abpos < O.path.abpos;
					#if 0
					else if ( path.aepos != O.path.aepos )
						return path.aepos < O.path.aepos;
					else if ( path.bbpos != O.path.bbpos )
						return path.bbpos < O.path.bbpos;
					else if ( path.bepos != O.path.bepos )
						return path.bepos < O.path.bepos;
					#endif
					else
						return false;
				}

				// 40

				uint64_t deserialise(std::istream & in)
				{
					uint64_t s = 0;

					s += path.deserialise(in);

					uint64_t offset = 0;
					flags = getUnsignedLittleEndianInteger4(in,offset);
					aread = getLittleEndianInteger4(in,offset);
					bread = getLittleEndianInteger4(in,offset);

					getLittleEndianInteger4(in,offset); // padding

					s += offset;

					// std::cerr << "flags=" << flags << " aread=" << aread << " bread=" << bread << std::endl;

					return s;
				}

				uint64_t serialise(std::ostream & out)
				{
					uint64_t s = 0;
					s += path.serialise(out);
					uint64_t offset = 0;
					putUnsignedLittleEndianInteger4(out,flags,offset);
					putLittleEndianInteger4(out,aread,offset);
					putLittleEndianInteger4(out,bread,offset);
					putLittleEndianInteger4(out,0,offset); // padding
					s += offset;
					return s;
				}

				uint64_t serialiseWithPath(std::ostream & out, bool const small) const
				{
					uint64_t s = 0;
					s += path.serialise(out);
					uint64_t offset = 0;
					putUnsignedLittleEndianInteger4(out,flags,offset);
					putLittleEndianInteger4(out,aread,offset);
					putLittleEndianInteger4(out,bread,offset);
					putLittleEndianInteger4(out,0,offset); // padding
					s += offset;
					s += path.serialisePath(out,small);
					return s;
				}

				bool operator==(Overlap const & O) const
				{
					return
						compareMeta(O) && path == O.path;
				}

				bool compareMeta(Overlap const & O) const
				{
					return
						path.comparePathMeta(O.path) &&
						flags == O.flags &&
						aread == O.aread &&
						bread == O.bread;
				}

				bool compareMetaLower(Overlap const & O) const
				{
					return
						path.comparePathMetaLower(O.path) &&
						flags == O.flags &&
						aread == O.aread &&
						bread == O.bread;
				}

				Overlap()
				{

				}

				Overlap(std::istream & in)
				{
					deserialise(in);
				}

				Overlap(std::istream & in, uint64_t & s)
				{
					s += deserialise(in);
				}

				bool isInverse() const
				{
					return (flags & getInverseFlag()) != 0;
				}

				static uint64_t getInverseFlag()
				{
					return 1;
				}

				static uint64_t getPrimaryFlag()
				{
					return (1ull << 31);
				}

				static uint64_t getACompFlag()
				{
					return 0x2;
				}

				static uint64_t getStartFlag()
				{
					return 0x4;
				}

				static uint64_t getNextFlag()
				{
					return 0x8;
				}

				static uint64_t getBestFlag()
				{
					return 0x10;
				}

				bool isAComp() const
				{
					return (flags & getACompFlag()) != 0;
				}

				bool isStart() const
				{
					return (flags & getStartFlag()) != 0;
				}

				bool isBest() const
				{
					return (flags & getBestFlag()) != 0;
				}

				bool isPrimary() const
				{
					return (flags & getPrimaryFlag()) != 0;
				}

				void setPrimary()
				{
					flags |= getPrimaryFlag();
				}

				uint64_t getNumErrors() const
				{
					return path.getNumErrors();
				}

				static Overlap computeOverlap(
					int64_t const flags,
					int64_t const aread,
					int64_t const bread,
					int64_t const abpos,
					int64_t const aepos,
					int64_t const bbpos,
					int64_t const bepos,
					int64_t const tspace,
					libmaus2::lcs::AlignmentTraceContainer const & ATC
				)
				{
					Overlap OVL;
					OVL.flags = flags;
					OVL.aread = aread;
					OVL.bread = bread;
					OVL.path = computePath(abpos,aepos,bbpos,bepos,tspace,ATC);
					return OVL;
				}

				static Path computePath(
					int64_t const abpos,
					int64_t const aepos,
					int64_t const bbpos,
					int64_t const bepos,
					int64_t const tspace,
					libmaus2::lcs::AlignmentTraceContainer const & ATC
				)
				{
					Path path;

					path.abpos = abpos;
					path.bbpos = bbpos;
					path.aepos = aepos;
					path.bepos = bepos;
					path.diffs = 0;

					// current point on A
					int64_t a_i = ( path.abpos / tspace ) * tspace;

					libmaus2::lcs::AlignmentTraceContainer::step_type const * tc = ATC.ta;

					int64_t bsum = 0;

					while ( a_i < aepos )
					{
						int64_t a_c = std::max(abpos,a_i);
						// block end point on A
						int64_t const a_i_1 = std::min ( static_cast<int64_t>(a_i + tspace), static_cast<int64_t>(path.aepos) );

						int64_t bforw = 0;
						int64_t err = 0;
						while ( a_c < a_i_1 )
						{
							assert ( tc < ATC.te );
							switch ( *(tc++) )
							{
								case libmaus2::lcs::AlignmentTraceContainer::STEP_MATCH:
									++a_c;
									++bforw;
									break;
								case libmaus2::lcs::AlignmentTraceContainer::STEP_MISMATCH:
									++a_c;
									++bforw;
									++err;
									break;
								case libmaus2::lcs::AlignmentTraceContainer::STEP_DEL:
									++a_c;
									++err;
									break;
								case libmaus2::lcs::AlignmentTraceContainer::STEP_INS:
									++bforw;
									++err;
									break;
								case libmaus2::lcs::AlignmentTraceContainer::STEP_RESET:
									break;
							}
						}

						// consume rest of operations if we reached end of alignment on A read
						while ( a_c == static_cast<int64_t>(path.aepos) && tc != ATC.te )
						{
							switch ( *(tc++) )
							{
								case libmaus2::lcs::AlignmentTraceContainer::STEP_MATCH:
									++a_c;
									++bforw;
									break;
								case libmaus2::lcs::AlignmentTraceContainer::STEP_MISMATCH:
									++a_c;
									++bforw;
									++err;
									break;
								case libmaus2::lcs::AlignmentTraceContainer::STEP_DEL:
									++a_c;
									++err;
									break;
								case libmaus2::lcs::AlignmentTraceContainer::STEP_INS:
									++bforw;
									++err;
									break;
								case libmaus2::lcs::AlignmentTraceContainer::STEP_RESET:
									break;
							}
						}

						path.diffs += err;
						path.path.push_back(Path::tracepoint(err,bforw));
						bsum += bforw;

						a_i = a_i_1;
					}

					path.tlen = path.path.size() << 1;

					bool const ok = bsum == (bepos - bbpos);
					if ( ! ok )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "Overlap::computePath: bsum=" << bsum << " != " << (bepos - bbpos) << std::endl;

						std::pair<uint64_t,uint64_t> SL = ATC.getStringLengthUsed(ATC.ta,ATC.te);
						lme.getStream() << "SL = " << SL.first << "," << SL.second << std::endl;
						lme.getStream() << "aepos-abpos=" << aepos-abpos << std::endl;
						lme.getStream() << "bepos-bbpos=" << bepos-bbpos << std::endl;
						lme.getStream() << "ATC.te - ATC.ta=" << (ATC.te-ATC.ta) << std::endl;
						lme.getStream() << "ATC.te - tc=" << (ATC.te-tc) << std::endl;

						lme.finish();
						throw lme;
					}

					return path;
				}

				static Path computePath(std::vector<TraceBlock> const & V)
				{
					Path path;

					if ( ! V.size() )
					{
						path.abpos = 0;
						path.bbpos = 0;
						path.aepos = 0;
						path.bepos = 0;
						path.diffs = 0;
						path.tlen = 0;
					}
					else
					{
						path.abpos = V.front().A.first;
						path.aepos = V.back().A.second;
						path.bbpos = V.front().B.first;
						path.bepos = V.back().B.second;
						path.diffs = 0;
						path.tlen = 2*V.size();

						for ( uint64_t i = 0; i < V.size(); ++i )
						{
							path.path.push_back(Path::tracepoint(V[i].err,V[i].B.second-V[i].B.first));
							path.diffs += V[i].err;
						}
					}

					return path;
				}

				template<typename path_iterator>
				static void computeTrace(
					path_iterator path,
					size_t pathlen,
					int32_t const abpos,
					int32_t const aepos,
					int32_t const bbpos,
					int32_t const /* bepos */,
					uint8_t const * aptr,
					uint8_t const * bptr,
					int64_t const tspace,
					libmaus2::lcs::AlignmentTraceContainer & ATC,
					libmaus2::lcs::Aligner & aligner
				)
				{
					// current point on A
					int32_t a_i = ( abpos / tspace ) * tspace;
					// current point on B
					int32_t b_i = ( bbpos );

					// reset trace container
					ATC.reset();

					for ( size_t i = 0; i < pathlen; ++i )
					{
						// block end point on A
						int32_t const a_i_1 = std::min ( static_cast<int32_t>(a_i + tspace), static_cast<int32_t>(aepos) );
						// block end point on B
						int32_t const b_i_1 = b_i + path[i].second;

						// block on A
						uint8_t const * asubsub_b = aptr + std::max(a_i,abpos);
						uint8_t const * asubsub_e = asubsub_b + a_i_1-std::max(a_i,abpos);

						// block on B
						uint8_t const * bsubsub_b = bptr + b_i;
						uint8_t const * bsubsub_e = bsubsub_b + (b_i_1-b_i);

						aligner.align(asubsub_b,(asubsub_e-asubsub_b),bsubsub_b,bsubsub_e-bsubsub_b);

						// add trace to full alignment
						ATC.push(aligner.getTraceContainer());

						// update start points
						b_i = b_i_1;
						a_i = a_i_1;
					}
				}

				template<typename path_iterator>
				static std::pair<int32_t,int32_t> computeDiagBand(
					path_iterator path,
					size_t pathlen,
					int32_t const abpos,
					int32_t const aepos,
					int32_t const bbpos,
					int32_t const /* bepos */,
					int32_t const tspace
				)
				{
					// current point on A
					int32_t a_i = ( abpos / tspace ) * tspace;
					// current point on B
					int32_t b_i = bbpos;

					int32_t mindiag = abpos - bbpos;
					int32_t maxdiag = mindiag;

					for ( size_t i = 0; i < pathlen; ++i )
					{
						// actual block start in a
						int32_t const a_i_0 = std::max(a_i,abpos);

						// block end point on A
						int32_t const a_i_1 = std::min ( static_cast<int32_t>(a_i + tspace), static_cast<int32_t>(aepos) );
						// block end point on B
						int32_t const b_i_1 = b_i + path[i].second;

						int32_t const blockstartdiag = a_i_0 - b_i;
						// bounds, not actual values
						int32_t const blockmindiag = blockstartdiag - path[i].second;
						int32_t const blockmaxdiag = blockstartdiag + tspace;

						mindiag = std::min(mindiag,blockmindiag);
						maxdiag = std::max(maxdiag,blockmaxdiag);

						// update start points
						b_i = b_i_1;
						a_i = a_i_1;
					}

					return std::pair<int32_t,int32_t>(mindiag,maxdiag);
				}

				template<typename path_iterator>
				static std::pair<int32_t,int32_t> computeDiagBandTracePoints(
					path_iterator path,
					size_t pathlen,
					int32_t const abpos,
					int32_t const aepos,
					int32_t const bbpos,
					int32_t const bepos,
					int32_t const tspace
				)
				{
					// current point on A
					int32_t a_i = ( abpos / tspace ) * tspace;
					// current point on B
					int32_t b_i = bbpos;

					int32_t mindiag = abpos - bbpos;
					int32_t maxdiag = mindiag;

					for ( size_t i = 0; i < pathlen; ++i )
					{
						// actual block start in a
						int32_t const a_i_0 = std::max(a_i,abpos);

						int32_t const blockstartdiag = a_i_0 - b_i;
						assert ( (i!=0) || (blockstartdiag==abpos-bbpos) );
						mindiag = std::min(mindiag,blockstartdiag);
						maxdiag = std::max(maxdiag,blockstartdiag);

						int32_t const bforw = path[i].second;

						// block end point on A
						int32_t const a_i_1 = std::min ( static_cast<int32_t>(a_i + tspace), static_cast<int32_t>(aepos) );
						// block end point on B
						int32_t const b_i_1 = b_i + bforw;

						// update start points
						b_i = b_i_1;
						a_i = a_i_1;
					}

					int32_t const blockenddiag = aepos-bepos;
					mindiag = std::min(mindiag,blockenddiag);
					maxdiag = std::max(maxdiag,blockenddiag);

					return std::pair<int32_t,int32_t>(mindiag,maxdiag);
				}

				struct TracePointId
				{
					int32_t apos;
					int32_t bpos;
					int32_t id;
					int32_t subid;

					bool operator<(TracePointId const & O) const
					{
						if ( apos != O.apos )
							return apos < O.apos;
						else if ( bpos != O.bpos )
							return bpos < O.bpos;
						else
							return id < O.id;
					}
				};

				template<typename path_iterator>
				static uint64_t pushTracePoints(
					path_iterator path,
					size_t pathlen,
					int32_t const abpos,
					int32_t const aepos,
					int32_t const bbpos,
					int32_t const /* bepos */,
					int32_t const tspace,
					libmaus2::autoarray::AutoArray< TracePointId > & TP,
					uint64_t o,
					int32_t const id
				)
				{
					TP.ensureSize(o+(pathlen+1));

					// current point on A
					int32_t a_i = ( abpos / tspace ) * tspace;
					// current point on B
					int32_t b_i = bbpos;

					for ( size_t i = 0; i < pathlen; ++i )
					{
						// actual block start in a
						int32_t const a_i_0 = std::max(a_i,abpos);

						TracePointId TPI;
						TPI.apos = a_i_0;
						TPI.bpos = b_i;
						TPI.id = id;
						TPI.subid = i;
						TP[o++] = TPI;

						int32_t const bforw = path[i].second;

						// block end point on A
						int32_t const a_i_1 = std::min ( static_cast<int32_t>(a_i + tspace), static_cast<int32_t>(aepos) );
						// block end point on B
						int32_t const b_i_1 = b_i + bforw;

						// update start points
						b_i = b_i_1;
						a_i = a_i_1;
					}

					TracePointId TPI;
					TPI.apos = a_i;
					TPI.bpos = b_i;
					TPI.id = id;
					TPI.subid = pathlen;
					TP[o++] = TPI;

					return o;
				}

				template<typename path_iterator>
				static void computeTracePreMapped(
					path_iterator path,
					size_t pathlen,
					int32_t const abpos,
					int32_t const aepos,
					int32_t const bbpos,
					int32_t const
					#if ! defined(NDEBUG)
						bepos
					#endif
						,
					uint8_t const * aptr,
					uint8_t const * bptr,
					int64_t const tspace,
					libmaus2::lcs::AlignmentTraceContainer & ATC,
					libmaus2::lcs::Aligner & aligner
				)
				{
					// current point on A
					int32_t a_i = ( abpos / tspace ) * tspace;
					// current point on B
					int32_t b_i = ( bbpos );

					// reset trace container
					ATC.reset();

					int64_t bsum = 0;
					for ( size_t i = 0; i < pathlen; ++i )
					{
						// update sum
						bsum += path[i].second;

						// block end point on A
						int32_t const a_i_1 = std::min ( static_cast<int32_t>(a_i + tspace), static_cast<int32_t>(aepos) );
						// block end point on B
						int32_t const b_i_1 = b_i + path[i].second;

						// block on A
						uint8_t const * asubsub_b = aptr + std::max(a_i,abpos);
						uint8_t const * asubsub_e = asubsub_b + a_i_1-std::max(a_i,abpos);

						// block on B
						uint8_t const * bsubsub_b = bptr + b_i;
						uint8_t const * bsubsub_e = bsubsub_b + (b_i_1-b_i);

						aligner.alignPreMapped(asubsub_b,(asubsub_e-asubsub_b),bsubsub_b,bsubsub_e-bsubsub_b);

						// add trace to full alignment
						ATC.push(aligner.getTraceContainer());

						#if 0
						libmaus2::lcs::AlignmentStatistics astats = aligner.getTraceContainer().getAlignmentStatistics();
						assert ( astats.matches + astats.mismatches + astats.deletions  == (a_i_1 - std::max(a_i,abpos)) );
						assert ( astats.matches + astats.mismatches + astats.insertions == (b_i_1 - b_i) );
						#endif

						// update start points
						b_i = b_i_1;
						a_i = a_i_1;
					}

					#if ! defined(NDEBUG)
					assert ( bsum == (bepos-bbpos) );
					#endif

					#if 0
					libmaus2::lcs::AlignmentStatistics astats = ATC.getAlignmentStatistics();
					assert ( astats.matches + astats.mismatches + astats.deletions  == (aepos-abpos) );
					assert ( astats.matches + astats.mismatches + astats.insertions == (bepos-bbpos) );
					#endif
				}

				static void computeTrace(
					Path const & path,
					uint8_t const * aptr,
					uint8_t const * bptr,
					int64_t const tspace,
					libmaus2::lcs::AlignmentTraceContainer & ATC,
					libmaus2::lcs::Aligner & aligner
				)
				{
					// current point on A
					int32_t a_i = ( path.abpos / tspace ) * tspace;
					// current point on B
					int32_t b_i = ( path.bbpos );

					// reset trace container
					ATC.reset();

					for ( size_t i = 0; i < path.path.size(); ++i )
					{
						// block end point on A
						int32_t const a_i_1 = std::min ( static_cast<int32_t>(a_i + tspace), static_cast<int32_t>(path.aepos) );
						// block end point on B
						int32_t const b_i_1 = b_i + path.path[i].second;

						// block on A
						uint8_t const * asubsub_b = aptr + std::max(a_i,path.abpos);
						uint8_t const * asubsub_e = asubsub_b + a_i_1-std::max(a_i,path.abpos);

						// block on B
						uint8_t const * bsubsub_b = bptr + b_i;
						uint8_t const * bsubsub_e = bsubsub_b + (b_i_1-b_i);

						aligner.align(asubsub_b,(asubsub_e-asubsub_b),bsubsub_b,bsubsub_e-bsubsub_b);

						// add trace to full alignment
						ATC.push(aligner.getTraceContainer());

						// update start points
						b_i = b_i_1;
						a_i = a_i_1;
					}
				}

				static void computeTrace(
					std::vector<TraceBlock> const & TBV,
					uint8_t const * aptr,
					uint8_t const * bptr,
					libmaus2::lcs::AlignmentTraceContainer & ATC,
					libmaus2::lcs::Aligner & aligner
				)
				{
					// reset trace container
					ATC.reset();

					for ( size_t i = 0; i < TBV.size(); ++i )
					{
						int32_t const a_i_0 = TBV[i].A.first;
						int32_t const a_i_1 = TBV[i].A.second;
						int32_t const b_i_0 = TBV[i].B.first;
						int32_t const b_i_1 = TBV[i].B.second;

						// block on A
						uint8_t const * asubsub_b = aptr + a_i_0;
						uint8_t const * asubsub_e = aptr + a_i_1;

						// block on B
						uint8_t const * bsubsub_b = bptr + b_i_0;
						uint8_t const * bsubsub_e = bptr + b_i_1;

						aligner.align(asubsub_b,(asubsub_e-asubsub_b),bsubsub_b,bsubsub_e-bsubsub_b);

						// add trace to full alignment
						ATC.push(aligner.getTraceContainer());
					}
				}

				/**
				 * compute alignment trace
				 *
				 * @param aptr base sequence of aread
				 * @param bptr base sequence of bread if isInverse() returns false, reverse complement of bread if isInverse() returns true
				 * @param tspace trace point spacing
				 * @param ATC trace container for storing trace
				 * @param aligner aligner
				 **/
				void computeTrace(
					uint8_t const * aptr,
					uint8_t const * bptr,
					int64_t const tspace,
					libmaus2::lcs::AlignmentTraceContainer & ATC,
					libmaus2::lcs::Aligner & aligner
				) const
				{
					computeTrace(path,aptr,bptr,tspace,ATC,aligner);
				}

				/**
				 * get error block histogram
				 *
				 * @param tspace trace point spacing
				 **/
				void fillErrorHistogram(
					int64_t const tspace,
					std::map< uint64_t, std::map<uint64_t,uint64_t> > & H,
					int64_t const rlen
				) const
				{
					// current point on A
					int32_t a_i = ( path.abpos / tspace ) * tspace;
					// current point on B
					int32_t b_i = ( path.bbpos );

					for ( size_t i = 0; i < path.path.size(); ++i )
					{
						// block start on A
						int32_t const a_i_0 = std::max( a_i, path.abpos );
						// block end point on A
						int32_t const a_i_1 = std::min ( static_cast<int32_t>(a_i + tspace), static_cast<int32_t>(path.aepos) );
						// block end point on B
						int32_t const b_i_1 = b_i + path.path[i].second;

						if (
							(
								(a_i_1 - a_i_0) == tspace
								&&
								(a_i_0 % tspace == 0)
								&&
								(a_i_1 % tspace == 0)
							)
							||
							(
								a_i_1 == rlen
							)
						)
						{
							#if 0
							if ( a_i_0 == path.abpos )
								std::cerr << "start block" << std::endl;
							if ( a_i_1 == rlen )
								std::cerr << "end block" << std::endl;
							#endif

							H [ a_i / tspace ][path.path[i].first]++;
						}

						// update start points
						b_i = b_i_1;
						a_i = a_i_1;
					}
				}

				/**
				 * get bases in full blocks and number of errors in these blocks
				 *
				 * @param tspace trace point spacing
				 **/
				std::pair<uint64_t,uint64_t> fullBlockErrors(int64_t const tspace) const
				{
					// current point on A
					int32_t a_i = ( path.abpos / tspace ) * tspace;
					// current point on B
					int32_t b_i = ( path.bbpos );

					uint64_t errors = 0;
					uint64_t length = 0;

					for ( size_t i = 0; i < path.path.size(); ++i )
					{
						// block end point on A
						int32_t const a_i_1 = std::min ( static_cast<int32_t>(a_i + tspace), static_cast<int32_t>(path.aepos) );
						// block end point on B
						int32_t const b_i_1 = b_i + path.path[i].second;

						if (
							(a_i_1 - a_i) == tspace
							&&
							(a_i % tspace == 0)
							&&
							(a_i_1 % tspace == 0)
						)
						{
							errors += path.path[i].first;
							length += tspace;
						}

						// update start points
						b_i = b_i_1;
						a_i = a_i_1;
					}

					return std::pair<uint64_t,uint64_t>(length,errors);
				}

				Overlap getSwapped(
					int64_t const tspace,
					uint8_t const * aptr,
					int64_t const alen,
					uint8_t const * bptr,
					int64_t const blen,
					libmaus2::autoarray::AutoArray<uint8_t> & Binv,
					libmaus2::lcs::AlignmentTraceContainer & ATC,
					libmaus2::lcs::Aligner & aligner
				) const
				{
					if ( ! isInverse() )
					{
						computeTrace(path,aptr,bptr,tspace,ATC,aligner);
						ATC.swapRoles();
						// std::reverse(ATC.ta,ATC.te);
						Overlap OVL;
						OVL.flags = flags;
						OVL.aread = bread;
						OVL.bread = aread;
						OVL.path = computePath(
							path.bbpos,
							path.bepos,
							path.abpos,
							path.aepos,
							tspace,ATC);
						return OVL;
					}
					else
					{
						if ( static_cast<int64_t>(Binv.size()) < blen )
							Binv.resize(blen);
						std::copy(bptr,bptr+blen,Binv.begin());
						std::reverse(Binv.begin(),Binv.begin()+blen);
						for ( int64_t i = 0; i < blen; ++i )
							Binv[i] = libmaus2::fastx::invertUnmapped(Binv[i]);

						computeTrace(path,aptr,Binv.begin(),tspace,ATC,aligner);
						ATC.swapRoles();
						std::reverse(ATC.ta,ATC.te);
						Overlap OVL;
						OVL.flags = flags;
						OVL.aread = bread;
						OVL.bread = aread;
						OVL.path = computePath(
							blen - path.bepos,
							blen - path.bbpos,
							alen - path.aepos,
							alen - path.abpos,
							tspace,ATC);
						return OVL;

					}
				}

				bool bConsistent() const
				{
					int64_t bsum = 0;
					for ( uint64_t i = 0; i < path.path.size(); ++i )
						bsum += path.path[i].second;
					return bsum == (path.bepos-path.bbpos);
				}

				Overlap getSwappedPreMapped(
					int64_t const tspace,
					uint8_t const * aptr,
					int64_t const alen,
					uint8_t const * bptr,
					int64_t const blen,
					libmaus2::lcs::AlignmentTraceContainer & ATC,
					libmaus2::lcs::Aligner & aligner
				) const
				{
					if ( ! isInverse() )
					{
						computeTracePreMapped(
							path.path.begin(),
							path.path.size(),
							path.abpos,path.aepos,path.bbpos,path.bepos,aptr,bptr,tspace,
							ATC,aligner);

						ATC.swapRoles();
						// std::reverse(ATC.ta,ATC.te);
						Overlap OVL;
						OVL.flags = flags;
						OVL.aread = bread;
						OVL.bread = aread;
						OVL.path = computePath(
							path.bbpos,
							path.bepos,
							path.abpos,
							path.aepos,
							tspace,ATC);
						return OVL;
					}
					else
					{
						computeTracePreMapped(
							path.path.begin(),
							path.path.size(),
							path.abpos,path.aepos,path.bbpos,path.bepos,aptr,bptr,tspace,
							ATC,aligner);

						ATC.swapRoles();
						std::reverse(ATC.ta,ATC.te);
						Overlap OVL;
						OVL.flags = flags;
						OVL.aread = bread;
						OVL.bread = aread;
						OVL.path = computePath(
							blen - path.bepos,
							blen - path.bbpos,
							alen - path.aepos,
							alen - path.abpos,
							tspace,ATC);
						return OVL;

					}
				}

				struct GetSwappedPreMappedInvAux
				{
					typedef GetSwappedPreMappedInvAux this_type;
					typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

					libmaus2::autoarray::AutoArray<uint8_t> T;
					libmaus2::autoarray::AutoArray<Path::tracepoint> TP;
					uint64_t apadlen;
					int64_t aread;

					GetSwappedPreMappedInvAux() : apadlen(0), aread(-1) {}

					void reset() { aread = -1; }

					void computePaddedRC(
						int const tspace,
						uint8_t const * aptr,
						int64_t const alen,
						int64_t const raread
					)
					{
						if ( raread != aread )
						{
							// padding to make length of a multiple of tspace
							uint64_t const apadbytes = (tspace - (alen % tspace)) % tspace;
							// padded length for a
							apadlen = alen + apadbytes;
							// allocate memory
							T.ensureSize(apadlen + 2);

							// end of allocated area
							uint8_t * t = T.begin() + (apadlen + 2);

							// terminator in back
							*(--t) = 4;
							// add reverse complement
							uint8_t const * s = aptr;
							uint8_t const * const se = s + alen;
							while ( s != se )
								*(--t) = (*(s++)) ^ 3;
							// fill pad bytes and front terminator
							while ( t != T.begin() )
								*(--t) = 4;

							#if 0
							// check
							uint8_t const * uc = T.begin() + 1 + apadbytes;
							for ( int64_t i = 0; i < alen; ++i )
								assert ( *(uc++) == (aptr[alen-i-1] ^ 3) );
							#endif

							aread = raread;
						}
					}

					void computeReversePathVector(Path const & path)
					{
						// compute reverse trace vector
						TP.ensureSize(path.path.size());
						Path::tracepoint * tp = TP.begin() + path.path.size();
						for ( uint64_t i = 0; i < path.path.size(); ++i )
							*(--tp) = path.path[i];
						assert ( tp == TP.begin() );
					}
				};

				/*
				 * aread assumed short
				 */
				Overlap getSwappedPreMappedInv(
					int64_t const tspace,
					uint8_t const * aptr,
					int64_t const alen,
					uint8_t const * bptr,
					int64_t const blen,
					libmaus2::lcs::AlignmentTraceContainer & ATC,
					libmaus2::lcs::Aligner & aligner,
					GetSwappedPreMappedInvAux & aux
				) const
				{
					if ( ! isInverse() )
					{
						computeTracePreMapped(
							path.path.begin(),
							path.path.size(),
							path.abpos,path.aepos,path.bbpos,path.bepos,aptr,bptr,tspace,
							ATC,aligner);

						ATC.swapRoles();
						// std::reverse(ATC.ta,ATC.te);
						Overlap OVL;
						OVL.flags = flags;
						OVL.aread = bread;
						OVL.bread = aread;
						OVL.path = computePath(
							path.bbpos,
							path.bepos,
							path.abpos,
							path.aepos,
							tspace,ATC);
						return OVL;
					}
					else
					{
						aux.computePaddedRC(tspace,aptr,alen,aread);
						aux.computeReversePathVector(path);

						// compute trace
						computeTracePreMapped(
							aux.TP.begin(),
							path.path.size(),
							aux.apadlen - path.aepos,
							aux.apadlen - path.abpos,
							blen - path.bepos,
							blen - path.bbpos,
							aux.T.begin() + 1 /* terminator */,
							bptr,
							tspace,
							ATC,aligner
						);

						ATC.swapRoles();

						Overlap OVL;
						OVL.flags = flags;
						OVL.aread = bread;
						OVL.bread = aread;
						OVL.path = computePath
						(
							blen - path.bepos,
							blen - path.bbpos,
							alen - path.aepos,
							alen - path.abpos,
							tspace,ATC
						);
						return OVL;
					}
				}

				Path getSwappedPath(int64_t const tspace) const
				{
					return computePath(getSwappedTraceBlocks(tspace));
				}

				Path getSwappedPathInverse(int64_t const tspace, int64_t const alen, int64_t const blen) const
				{
					return computePath(getSwappedTraceBlocksInverse(tspace,alen,blen));
				}

				std::vector<TraceBlock> getSwappedTraceBlocks(int64_t const tspace) const
				{
					std::vector<TraceBlock> V = getTraceBlocks(tspace);

					for ( uint64_t i = 0; i < V.size(); ++i )
						V[i].swap();

					return V;
				}

				std::vector<TraceBlock> getSwappedTraceBlocksInverse(int64_t const tspace, int64_t const alen, int64_t const blen) const
				{
					std::vector<TraceBlock> V = getTraceBlocks(tspace);

					for ( uint64_t i = 0; i < V.size(); ++i )
						V[i].swap(alen,blen);
					std::reverse(V.begin(),V.end());

					return V;
				}

				std::vector<TraceBlock> getTraceBlocks(int64_t const tspace) const
				{
					// current point on A
					int32_t a_i = ( path.abpos / tspace ) * tspace;
					// current point on B
					int32_t b_i = ( path.bbpos );

					std::vector < TraceBlock > V;

					for ( size_t i = 0; i < path.path.size(); ++i )
					{
						// block start point on A
						int32_t const a_i_0 = std::max ( a_i, static_cast<int32_t>(path.abpos) );
						// block end point on A
						int32_t const a_i_1 = std::min ( static_cast<int32_t>(a_i + tspace), static_cast<int32_t>(path.aepos) );
						// block end point on B
						int32_t const b_i_1 = b_i + path.path[i].second;

						V.push_back(
							TraceBlock(
								std::pair<int64_t,int64_t>(a_i_0,a_i_1),
								std::pair<int64_t,int64_t>(b_i,b_i_1),
								path.path[i].first
							)
						);

						// update start points
						b_i = b_i_1;
						a_i = a_i_1;
					}

					return V;
				}

				std::vector<TracePoint> getTracePoints(int64_t const tspace, uint64_t const traceid) const
				{
					std::vector<TracePoint> V;
					getTracePoints(tspace,traceid,V);
					return V;
				}

				void getTracePoints(int64_t const tspace, uint64_t const traceid, std::vector<TracePoint> & V) const
				{
					// current point on A
					int32_t a_i = ( path.abpos / tspace ) * tspace;
					// current point on B
					int32_t b_i = ( path.bbpos );

					for ( size_t i = 0; i < path.path.size(); ++i )
					{
						// block start point on A
						int32_t const a_i_0 = std::max ( a_i, static_cast<int32_t>(path.abpos) );
						// block end point on A
						int32_t const a_i_1 = std::min ( static_cast<int32_t>(a_i + tspace), static_cast<int32_t>(path.aepos) );
						// block end point on B
						int32_t const b_i_1 = b_i + path.path[i].second;

						V.push_back(TracePoint(a_i_0,b_i,traceid));

						// update start points
						b_i = b_i_1;
						a_i = a_i_1;
					}

					if ( V.size() )
						V.push_back(TracePoint(a_i,b_i,traceid));
					else
						V.push_back(TracePoint(path.abpos,path.bbpos,traceid));
				}

				std::vector<TracePoint> getSwappedTracePoints(int64_t const tspace, uint64_t const traceid) const
				{
					std::vector<TracePoint> V;
					getSwappedTracePoints(tspace,traceid,V);
					return V;
				}

				void getSwappedTracePoints(int64_t const tspace, uint64_t const traceid, std::vector<TracePoint> & V) const
				{
					// current point on A
					int32_t a_i = ( path.abpos / tspace ) * tspace;
					// current point on B
					int32_t b_i = ( path.bbpos );

					for ( size_t i = 0; i < path.path.size(); ++i )
					{
						// block start point on A
						int32_t const a_i_0 = std::max ( a_i, static_cast<int32_t>(path.abpos) );
						// block end point on A
						int32_t const a_i_1 = std::min ( static_cast<int32_t>(a_i + tspace), static_cast<int32_t>(path.aepos) );
						// block end point on B
						int32_t const b_i_1 = b_i + path.path[i].second;

						V.push_back(TracePoint(a_i_0,b_i,traceid));

						// update start points
						b_i = b_i_1;
						a_i = a_i_1;
					}

					if ( V.size() )
						V.push_back(TracePoint(a_i,b_i,traceid));
					else
						V.push_back(TracePoint(path.abpos,path.bbpos,traceid));

					for ( uint64_t i = 0; i < V.size(); ++i )
						V[i].swap();
				}

				std::vector<uint64_t> getFullBlocks(int64_t const tspace) const
				{
					std::vector<TraceBlock> const TB = getTraceBlocks(tspace);
					std::vector<uint64_t> B;
					for ( uint64_t i = 0; i < TB.size(); ++i )
						if (
							TB[i].A.second - TB[i].A.first == tspace
						)
						{
							assert ( TB[i].A.first % tspace == 0 );

							B.push_back(TB[i].A.first / tspace);
						}
					return B;
				}

				static bool haveOverlappingTraceBlock(std::vector<TraceBlock> const & VA, std::vector<TraceBlock> const & VB)
				{
					uint64_t b_low = 0;

					for ( uint64_t i = 0; i < VA.size(); ++i )
					{
						while ( (b_low < VB.size()) && (VB[b_low].A.second <= VA[i].A.first) )
							++b_low;

						assert ( (b_low == VB.size()) || (VB[b_low].A.second > VA[i].A.first) );

						uint64_t b_high = b_low;
						while ( b_high < VB.size() && (VB[b_high].A.first < VA[i].A.second) )
							++b_high;

						assert ( (b_high == VB.size()) || (VB[b_high].A.first >= VA[i].A.second) );

						if ( b_high-b_low )
						{
							for ( uint64_t j = b_low; j < b_high; ++j )
							{
								assert ( VA[i].overlapsA(VB[j]) );
								if ( VA[i].overlapsB(VB[j]) )
									return true;
							}
						}
					}

					return false;

				}

				static bool haveOverlappingTraceBlock(Overlap const & A, Overlap const & B, int64_t const tspace)
				{
					return haveOverlappingTraceBlock(A.getTraceBlocks(tspace),B.getTraceBlocks(tspace));
				}

				/**
				 * fill number of spanning reads for each sparse trace point on read
				 *
				 * @param tspace trace point spacing
				 **/
				void fillSpanHistogram(
					int64_t const tspace,
					std::map< uint64_t, uint64_t > & H,
					uint64_t const ethres,
					uint64_t const cthres,
					uint64_t const rlen
				) const
				{
					std::vector<uint64_t> M(path.path.size(),std::numeric_limits<uint64_t>::max());

					// id of lowest block in alignment
					uint64_t const lowblockid = (path.abpos / tspace);
					// number of blocks
					uint64_t const numblocks = (rlen+tspace-1)/tspace;
					// current point on A
					int32_t a_i = lowblockid * tspace;
					// current point on B
					int32_t b_i = ( path.bbpos );

					for ( size_t i = 0; i < path.path.size(); ++i )
					{
						// start point on A
						int32_t const a_i_0 = std::max ( a_i, path.abpos );
						// block end point on A
						int32_t const a_i_1 = std::min ( static_cast<int32_t>(a_i + tspace), static_cast<int32_t>(path.aepos) );
						// block end point on B
						int32_t const b_i_1 = b_i + path.path[i].second;

						if (
							(
								(a_i_1 - a_i_0) == tspace
								&&
								(a_i_0 % tspace == 0)
								&&
								(a_i_1 % tspace == 0)
								&&
								path.path[i].first <= ethres
							)
							||
							(
								(a_i_1 == path.aepos)
								&&
								(path.path[i].first <= ethres)
							)
						)
						{
							M . at ( i ) = path.path[i].first;
							// assert ( a_i / tspace == i );
						}

						// update start points
						b_i = b_i_1;
						a_i = a_i_1;
					}

					// number of valid blocks on the right
					uint64_t numleft = 0;
					uint64_t numright = 0;
					for ( uint64_t i = 0; i < M.size(); ++i )
						if ( M[i] != std::numeric_limits<uint64_t>::max() )
							numright++;

					// mark all blocks as spanned if we find at least cthres ok blocks left and right of it (at any distance)
					for ( uint64_t i = 0; i < M.size(); ++i )
					{
						bool const below = M[i] != std::numeric_limits<uint64_t>::max();

						if ( below )
							numright -= 1;

						if (
							(numleft >= cthres && numright >= cthres)
						)
							H [ lowblockid + i ] += 1;

						if ( below )
							numleft += 1;
					}

					// mark first cthres blocks as spanned if all first cthres blocks are ok
					if ( (path.abpos == 0) && M.size() >= 2*cthres )
					{
						uint64_t numok = 0;
						for ( uint64_t i = 0; i < 2*cthres; ++i )
							if ( M.at(i) != std::numeric_limits<uint64_t>::max() )
								++numok;
						if ( numok == 2*cthres )
						{
							for ( uint64_t i = 0; i < cthres; ++i )
								H [ i ] += 1;
						}
					}
					// mark last cthres blocks as spanned if all last 2*cthres blocks are ok
					if ( (path.aepos == static_cast<int64_t>(rlen)) && M.size() >= 2*cthres )
					{
						uint64_t numok = 0;
						for ( uint64_t i = 0; i < 2*cthres; ++i )
							if ( M.at(M.size()-i-1) != std::numeric_limits<uint64_t>::max() )
								++numok;
						if ( numok == 2*cthres )
						{
							for ( uint64_t i = 0; i < cthres; ++i )
								H [ numblocks - i - 1 ] += 1;
						}
					}
				}
			};

			struct OverlapComparator
			{
				bool operator()(Overlap const & lhs, Overlap const & rhs) const
				{
					if ( lhs.aread != rhs.aread )
						return lhs.aread < rhs.aread;
					else if ( lhs.bread != rhs.bread )
						return lhs.bread < rhs.bread;
					else if ( lhs.isInverse() != rhs.isInverse() )
						return !lhs.isInverse();
					else if ( lhs.path.abpos != rhs.path.abpos )
						return lhs.path.abpos < rhs.path.abpos;
					else
						return false;
				}

			};

			struct OverlapFullComparator
			{
				bool operator()(Overlap const & lhs, Overlap const & rhs) const
				{
					if ( lhs.aread != rhs.aread )
						return lhs.aread < rhs.aread;
					else if ( lhs.bread != rhs.bread )
						return lhs.bread < rhs.bread;
					else if ( lhs.isInverse() != rhs.isInverse() )
						return !lhs.isInverse();
					else if ( lhs.path.abpos != rhs.path.abpos )
						return lhs.path.abpos < rhs.path.abpos;
					else if ( lhs.path.aepos != rhs.path.aepos )
						return lhs.path.aepos < rhs.path.aepos;
					else if ( lhs.path.bbpos != rhs.path.bbpos )
						return lhs.path.bbpos < rhs.path.bbpos;
					else if ( lhs.path.bepos != rhs.path.bepos )
						return lhs.path.bepos < rhs.path.bepos;
					else
						return false;
				}
			};

			struct OverlapComparatorBReadARead
			{
				bool operator()(Overlap const & lhs, Overlap const & rhs) const
				{
					if ( lhs.bread != rhs.bread )
						return lhs.bread < rhs.bread;
					else if ( lhs.aread != rhs.aread )
						return lhs.aread < rhs.aread;
					else if ( lhs.isInverse() != rhs.isInverse() )
						return !lhs.isInverse();
					else if ( lhs.path.abpos != rhs.path.abpos )
						return lhs.path.abpos < rhs.path.abpos;
					else
						return false;
				}

			};

			struct OverlapComparatorAIdBId
			{
				bool operator()(Overlap const & lhs, Overlap const & rhs) const
				{
					if ( lhs.aread != rhs.aread )
						return lhs.aread < rhs.aread;
					else if ( lhs.bread != rhs.bread )
						return lhs.bread < rhs.bread;
					else
						return false;
				}

			};

			struct OverlapComparatorBIdAId
			{
				bool operator()(Overlap const & lhs, Overlap const & rhs) const
				{
					if ( lhs.bread != rhs.bread )
						return lhs.bread < rhs.bread;
					else if ( lhs.aread != rhs.aread )
						return lhs.aread < rhs.aread;
					else
						return false;
				}

			};

			std::ostream & operator<<(std::ostream & out, Overlap const & P);
		}
	}
}
#endif
