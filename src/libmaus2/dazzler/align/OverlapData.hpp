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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_OVERLAPDATA_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_OVERLAPDATA_HPP

#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/util/LELoad.hpp>
#include <libmaus2/dazzler/align/Overlap.hpp>
#include <libmaus2/dazzler/align/AlignmentFile.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct OverlapData
			{
				typedef OverlapData this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				struct OverlapOffset
				{
					uint64_t offset;
					uint64_t length;

					OverlapOffset()
					{

					}

					OverlapOffset(uint64_t const roffset, uint64_t const rlength)
					: offset(roffset), length(rlength)
					{

					}
				};

				libmaus2::autoarray::AutoArray<uint8_t> Adata;
				libmaus2::autoarray::AutoArray<OverlapOffset> Aoffsets;
				uint64_t overlapsInBuffer;

				OverlapData() : overlapsInBuffer(0) {}

				std::pair<uint8_t const *, uint8_t const *> getData(uint64_t const i) const
				{
					if ( i < overlapsInBuffer )
					{
						return
							std::pair<uint8_t const *, uint8_t const *>(
								Adata.begin() + Aoffsets[i].offset,
								Adata.begin() + Aoffsets[i].offset + Aoffsets[i].length
							);
					}
					else
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "OverlapParser::getData(): index " << i << " is out of range" << std::endl;
						lme.finish();
						throw lme;
					}
				}

				std::pair<uint8_t *, uint8_t *> getData(uint64_t const i)
				{
					if ( i < overlapsInBuffer )
					{
						return
							std::pair<uint8_t *, uint8_t *>(
								Adata.begin() + Aoffsets[i].offset,
								Adata.begin() + Aoffsets[i].offset + Aoffsets[i].length
							);
					}
					else
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "OverlapParser::getData(): index " << i << " is out of range" << std::endl;
						lme.finish();
						throw lme;
					}
				}

				std::string getDataAsString(uint64_t const i) const
				{
					std::pair<uint8_t const *, uint8_t const *> P = getData(i);
					return std::string(P.first,P.second);
				}

				uint64_t size() const
				{
					return overlapsInBuffer;
				}

				void swap(OverlapData & rhs)
				{
					Adata.swap(rhs.Adata);
					Aoffsets.swap(rhs.Aoffsets);
					// Alength.swap(rhs.Alength);
					std::swap(overlapsInBuffer,rhs.overlapsInBuffer);
				}

				static std::ostream & toString(std::ostream & out, uint8_t const * p)
				{
					out << "OverlapData(";
					out << "flags=" << getFlags(p) << ";";
					out << "aread=" << getARead(p) << ";";
					out << "bread=" << getBRead(p) << ";";
					out << "tlen=" << getTLen(p) << ";";
					out << "diffs=" << getDiffs(p) << ";";
					out << "abpos=" << getABPos(p) << ";";
					out << "bbpos=" << getBBPos(p) << ";";
					out << "aepos=" << getAEPos(p) << ";";
					out << "bepos=" << getBEPos(p);
					out << ")";
					return out;
				}

				static int32_t getTLen(uint8_t const * p)
				{
					return libmaus2::util::loadValueLE4(p + 0*sizeof(int32_t));
				}

				static int32_t getDiffs(uint8_t const * p)
				{
					return libmaus2::util::loadValueLE4(p + 1*sizeof(int32_t));
				}

				static int32_t getABPos(uint8_t const * p)
				{
					return libmaus2::util::loadValueLE4(p + 2*sizeof(int32_t));
				}

				static int32_t getBBPos(uint8_t const * p)
				{
					return libmaus2::util::loadValueLE4(p + 3*sizeof(int32_t));
				}

				static int32_t getAEPos(uint8_t const * p)
				{
					return libmaus2::util::loadValueLE4(p + 4*sizeof(int32_t));
				}

				static int32_t getBEPos(uint8_t const * p)
				{
					return libmaus2::util::loadValueLE4(p + 5*sizeof(int32_t));
				}

				static uint32_t getFlags(uint8_t const * p)
				{
					return libmaus2::util::loadValueLE4(p + 6*sizeof(int32_t));
				}

				static void putFlags(uint8_t * p, uint32_t const f)
				{
					p += 6*sizeof(int32_t);

					p[0] = ((f>> 0) & 0xFF);
					p[1] = ((f>> 8) & 0xFF);
					p[2] = ((f>>16) & 0xFF);
					p[3] = ((f>>24) & 0xFF);
				}

				static void putARead(uint8_t * p, uint32_t const f)
				{
					p += 7*sizeof(int32_t);

					p[0] = ((f>> 0) & 0xFF);
					p[1] = ((f>> 8) & 0xFF);
					p[2] = ((f>>16) & 0xFF);
					p[3] = ((f>>24) & 0xFF);
				}

				static void putBRead(uint8_t * p, uint32_t const f)
				{
					p += 8*sizeof(int32_t);

					p[0] = ((f>> 0) & 0xFF);
					p[1] = ((f>> 8) & 0xFF);
					p[2] = ((f>>16) & 0xFF);
					p[3] = ((f>>24) & 0xFF);
				}

				static void addPrimaryFlag(uint8_t * p)
				{
					putFlags(
						p,
						getFlags(p)
						|
						libmaus2::dazzler::align::Overlap::getPrimaryFlag()
					);
				}

				static bool getPrimaryFlag(uint8_t const * p)
				{
					uint32_t const flags = getFlags(p);
					return (flags & libmaus2::dazzler::align::Overlap::getPrimaryFlag()) != 0;
				}

				static bool getInverseFlag(uint8_t const * p)
				{
					uint32_t const flags = getFlags(p);
					return (flags & libmaus2::dazzler::align::Overlap::getInverseFlag()) != 0;
				}

				static bool getACompFlag(uint8_t const * p)
				{
					uint32_t const flags = getFlags(p);
					return (flags & libmaus2::dazzler::align::Overlap::getACompFlag()) != 0;
				}

				static bool getStartFlag(uint8_t const * p)
				{
					uint32_t const flags = getFlags(p);
					return (flags & libmaus2::dazzler::align::Overlap::getStartFlag()) != 0;
				}

				static bool getNextFlag(uint8_t const * p)
				{
					uint32_t const flags = getFlags(p);
					return (flags & libmaus2::dazzler::align::Overlap::getNextFlag()) != 0;
				}

				static bool getBestFlag(uint8_t const * p)
				{
					uint32_t const flags = getFlags(p);
					return (flags & libmaus2::dazzler::align::Overlap::getBestFlag()) != 0;
				}

				static bool getHaploFlag(uint8_t const * p)
				{
					uint32_t const flags = getFlags(p);
					return (flags & libmaus2::dazzler::align::Overlap::getHaploFlag()) != 0;
				}

				static int32_t getARead(uint8_t const * p)
				{
					return libmaus2::util::loadValueLE4(p + 7*sizeof(int32_t));
				}

				static int32_t getBRead(uint8_t const * p)
				{
					return libmaus2::util::loadValueLE4(p + 8*sizeof(int32_t));
				}

				static uint8_t const * getTraceData(uint8_t const * p)
				{
					return p + 10*sizeof(int32_t);
				}

				static uint64_t decodeTraceVector(uint8_t const * p, libmaus2::autoarray::AutoArray<std::pair<uint16_t,uint16_t> > & A, bool const small)
				{
					int32_t const tlen = getTLen(p);
					assert ( (tlen & 1) == 0 );

					int32_t const tlen2 = tlen/2;
					if ( tlen2 > static_cast<int64_t>(A.size()) )
						A.resize(tlen2);

					if ( small )
					{
						uint8_t const * tp = getTraceData(p);

						for ( int32_t i = 0; i < tlen2; ++i )
						{
							A[i].first = tp[0];
							A[i].second = tp[1];
							tp += 2;
						}
					}
					else
					{
						uint8_t const * tp = getTraceData(p);

						for ( int32_t i = 0; i < tlen2; ++i )
						{
							A[i].first  = libmaus2::util::loadValueLE2(tp);
							A[i].second = libmaus2::util::loadValueLE2(tp+sizeof(int16_t));
							tp += 2*sizeof(int16_t);
						}
					}

					return tlen2;
				}

				static uint64_t getTracePoints(
					uint8_t const * p,
					int64_t const tspace,
					uint64_t const traceid,
					libmaus2::autoarray::AutoArray<TracePoint> & V,
					libmaus2::autoarray::AutoArray<std::pair<uint16_t,uint16_t> > & A,
					bool const fullonly,
					uint64_t o = 0
				)
				{
					uint64_t const numt = decodeTraceVector(p,A,libmaus2::dazzler::align::AlignmentFile::tspaceToSmall(tspace));

					// current point on A
					int32_t a_i = ( getABPos(p) / tspace ) * tspace;
					// current point on B
					int32_t b_i = ( getBBPos(p) );

					for ( size_t i = 0; i < numt; ++i )
					{
						// block start point on A
						int32_t const a_i_0 = std::max ( a_i, static_cast<int32_t>(getABPos(p)) );
						// block end point on A
						int32_t const a_i_1 = std::min ( static_cast<int32_t>(a_i + tspace), static_cast<int32_t>(getAEPos(p)) );
						// block end point on B
						int32_t const b_i_1 = b_i + A[i].second;

						if ( (!fullonly) || ((a_i_0%tspace) == 0) )
							V.push(o,TracePoint(a_i_0,b_i,traceid));

						// update start points
						b_i = b_i_1;
						a_i = a_i_1;
					}

					if ( o )
					{
						if ( (!fullonly) || ((a_i%tspace) == 0) )
							V.push(o,TracePoint(a_i,b_i,traceid));
					}
					else
					{
						if ( (!fullonly) || ((getABPos(p)%tspace) == 0) )
							V.push(o,TracePoint(getABPos(p),getBBPos(p),traceid));
					}

					return o;
				}


				static void computeTrace(
					uint8_t const * p,
					libmaus2::autoarray::AutoArray<std::pair<uint16_t,uint16_t> > & A,
					int64_t const tspace,
					uint8_t const * aptr,
					uint8_t const * bptr,
					libmaus2::lcs::AlignmentTraceContainer & ATC,
					libmaus2::lcs::Aligner & aligner
				)
				{
					bool const small = libmaus2::dazzler::align::AlignmentFile::tspaceToSmall(tspace);
					uint64_t const Alen = decodeTraceVector(p,A,small);
					libmaus2::dazzler::align::Overlap::computeTrace(
						A.begin(),
						Alen,
						getABPos(p),
						getAEPos(p),
						getBBPos(p),
						getBEPos(p),
						aptr,
						bptr,
						tspace,
						ATC,
						aligner
					);
				}

				struct TracePartInfo
				{
					int64_t abpos;
					int64_t aepos;
					int64_t bbpos;
					int64_t bepos;

					TracePartInfo() {}
					TracePartInfo(
						int64_t const rabpos,
						int64_t const raepos,
						int64_t const rbbpos,
						int64_t const rbepos
					) : abpos(rabpos), aepos(raepos), bbpos(rbbpos), bepos(rbepos) {}
				};

				template<typename path_iterator>
				static TracePartInfo computeTracePartByPath(
					int64_t const abpos,
					int64_t const aepos,
					int64_t const bbpos,
					int64_t const bepos,
					int64_t const rastart,
					int64_t const raend,
					path_iterator A,
					uint64_t const Alen,
					int64_t const tspace,
					uint8_t const * aptr,
					uint8_t const * bptr,
					libmaus2::lcs::AlignmentTraceContainer & ATC,
					libmaus2::lcs::Aligner & aligner
				)
				{
					assert ( raend >= rastart );

					int64_t astart = rastart;
					int64_t aend = raend;

					// align to trace point boundaries
					if ( astart % tspace != 0 )
						// round to zero
						astart = (astart / tspace) * tspace;
					if ( aend % tspace != 0 )
						// round to infinity
						aend = ((aend + tspace - 1)/tspace)*tspace;

					// clip to [abpos,aepos)
					astart = std::max(astart,abpos);
					aend = std::max(astart,aend);
					aend = std::min(aend,aepos);
					astart = std::min(astart,aend);

					// std::cerr << "[" << astart << "," << aend << ")" << " [" << abpos << "," << aepos << ")" << std::endl;

					Overlap::OffsetInfo startoff = libmaus2::dazzler::align::Overlap::getBforAOffset(tspace,abpos,aepos,bbpos,bepos,astart,A,Alen);
					Overlap::OffsetInfo endoff = libmaus2::dazzler::align::Overlap::getBforAOffset(tspace,abpos,aepos,bbpos,bepos,aend,A,Alen);

					libmaus2::dazzler::align::Overlap::computeTrace(
						A + startoff.offset,
						endoff.offset - startoff.offset,
						startoff.apos,
						endoff.apos,
						startoff.bpos,
						endoff.bpos,
						aptr,
						bptr,
						tspace,
						ATC,
						aligner
					);

					if ( rastart > astart )
					{
						std::pair<uint64_t,uint64_t> P = ATC.advanceA(rastart - astart);
						std::pair<uint64_t,uint64_t> SL = ATC.getStringLengthUsed(ATC.ta,ATC.ta + P.second);

						startoff.apos += SL.first;
						startoff.bpos += SL.second;
						ATC.ta += P.second;
					}
					if ( raend < aend )
					{
						std::reverse(ATC.ta,ATC.te);

						std::pair<uint64_t,uint64_t> P = ATC.advanceA(aend-raend);
						std::pair<uint64_t,uint64_t> SL = ATC.getStringLengthUsed(ATC.ta,ATC.ta + P.second);
						ATC.ta += P.second;

						std::reverse(ATC.ta,ATC.te);

						endoff.apos -= SL.first;
						endoff.bpos -= SL.second;
					}

					return TracePartInfo(startoff.apos,endoff.apos,startoff.bpos,endoff.bpos);
				}

				static TracePartInfo computeTracePart(
					int64_t const rastart,
					int64_t const raend,
					uint8_t const * p,
					libmaus2::autoarray::AutoArray<std::pair<uint16_t,uint16_t> > & A,
					int64_t const tspace,
					uint8_t const * aptr,
					uint8_t const * bptr,
					libmaus2::lcs::AlignmentTraceContainer & ATC,
					libmaus2::lcs::Aligner & aligner
				)
				{
					bool const small = libmaus2::dazzler::align::AlignmentFile::tspaceToSmall(tspace);
					uint64_t const Alen = decodeTraceVector(p,A,small);
					return computeTracePartByPath(
						static_cast<int64_t>(getABPos(p)),
						static_cast<int64_t>(getAEPos(p)),
						static_cast<int64_t>(getBBPos(p)),
						static_cast<int64_t>(getBEPos(p)),
						rastart,raend,A.begin(),Alen,tspace,aptr,bptr,ATC,aligner);
				}
			};
		}
	}
}
#endif
