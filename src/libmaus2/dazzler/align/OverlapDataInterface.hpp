/*
    libmaus2
    Copyright (C) 2017 German Tischler

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
#if !defined(LIBMAUS2_DAZZLER_ALIGN_OVERLAPDATAINTERFACE_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_OVERLAPDATAINTERFACE_HPP

#include <libmaus2/dazzler/align/OverlapData.hpp>
#include <libmaus2/dazzler/align/OverlapInfo.hpp>
#include <libmaus2/dazzler/align/Overlap.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct OverlapDataInterface
			{
				uint8_t const * p;

				OverlapDataInterface() : p(0) {}
				OverlapDataInterface(uint8_t const * rp) : p(rp) {}

				libmaus2::dazzler::align::OverlapInfo getInfo() const
				{
					return libmaus2::dazzler::align::OverlapInfo(
						(aread()<<1),
						(bread()<<1) | isInverse(),
						abpos(),aepos(),
						bbpos(),bepos()
					);
				}

				int64_t aread() const { return OverlapData::getARead(p); }
				int64_t bread() const { return OverlapData::getBRead(p); }
				int64_t abpos() const { return OverlapData::getABPos(p); }
				int64_t aepos() const { return OverlapData::getAEPos(p); }
				int64_t bbpos() const { return OverlapData::getBBPos(p); }
				int64_t bepos() const { return OverlapData::getBEPos(p); }
				int64_t tlen() const  { return OverlapData::getTLen(p); }
				int64_t diffs() const  { return OverlapData::getDiffs(p); }
				int64_t flags() const  { return OverlapData::getFlags(p); }
				int64_t isInverse() const  { return OverlapData::getInverseFlag(p); }
				bool getHaploFlag() const { return OverlapData::getHaploFlag(p); }
				bool getTrueFlag() const { return OverlapData::getTrueFlag(p); }
				uint64_t decodeTraceVector(libmaus2::autoarray::AutoArray<std::pair<uint16_t,uint16_t> > & A, int64_t const tspace) const
				{ return OverlapData::decodeTraceVector(p,A,libmaus2::dazzler::align::AlignmentFile::tspaceToSmall(tspace)); }
				double getErrorRate() const
				{ return (aepos() > abpos()) ? (static_cast<double>(diffs()) / static_cast<double>(aepos()-abpos())) : 0.0; }

				void getOverlap(
					libmaus2::dazzler::align::Overlap & OVL,
					libmaus2::autoarray::AutoArray<std::pair<uint16_t,uint16_t> > & A,
					int64_t const tspace
				) const
				{
					uint64_t const tracelen = decodeTraceVector(A,tspace);
					OVL.flags = flags();
					OVL.aread = aread();
					OVL.bread = bread();
					OVL.path.abpos = abpos();
					OVL.path.aepos = aepos();
					OVL.path.bbpos = bbpos();
					OVL.path.bepos = bepos();
					OVL.path.diffs = diffs();
					OVL.path.tlen = tracelen * 2;
					OVL.path.path.resize(tracelen);
					std::copy(A.begin(),A.begin()+tracelen,OVL.path.path.begin());
				}

				libmaus2::dazzler::align::Overlap getOverlap(libmaus2::autoarray::AutoArray<std::pair<uint16_t,uint16_t> > & A, int64_t const tspace) const
				{
					libmaus2::dazzler::align::Overlap OVL;
					getOverlap(OVL,A,tspace);
					return OVL;
				}

				libmaus2::dazzler::align::Overlap getOverlap(int64_t const tspace) const
				{
					libmaus2::dazzler::align::Overlap OVL;
					libmaus2::autoarray::AutoArray<std::pair<uint16_t,uint16_t> > A;
					getOverlap(OVL,A,tspace);
					return OVL;
				}

				void computeTrace(
					libmaus2::autoarray::AutoArray<std::pair<uint16_t,uint16_t> > & A,
					int64_t const tspace,
					uint8_t const * aptr,
					uint8_t const * bptr,
					libmaus2::lcs::AlignmentTraceContainer & ATC,
					libmaus2::lcs::Aligner & aligner
				) const
				{
					OverlapData::computeTrace(p,A,tspace,aptr,bptr,ATC,aligner);
				}
			};

			struct OverlapDataInterfaceFullComparator
			{
				uint8_t const * p;

				OverlapDataInterfaceFullComparator() : p(0) {}
				OverlapDataInterfaceFullComparator(uint8_t const * rp) : p(rp) {}

				static bool compare(
					uint8_t const * pa,
					uint8_t const * pb
				)
				{

					OverlapDataInterface const OA(pa);
					OverlapDataInterface const OB(pb);

					{
						int64_t const a = OA.aread();
						int64_t const b = OB.aread();

						if ( a != b )
							return a < b;
					}

					{
						int64_t const a = OA.bread();
						int64_t const b = OB.bread();

						if ( a != b )
							return a < b;
					}

					{
						int64_t const a = OA.isInverse();
						int64_t const b = OB.isInverse();

						if ( a != b )
							return a < b;
					}

					{
						int64_t const a = OA.abpos();
						int64_t const b = OB.abpos();

						if ( a != b )
							return a < b;
					}

					{
						int64_t const a = OA.aepos();
						int64_t const b = OB.aepos();

						if ( a != b )
							return a < b;
					}

					{
						int64_t const a = OA.bbpos();
						int64_t const b = OB.bbpos();

						if ( a != b )
							return a < b;
					}

					{
						int64_t const a = OA.bepos();
						int64_t const b = OB.bepos();

						if ( a != b )
							return a < b;
					}

					return false;
				}

				bool operator()(
					OverlapData::OverlapOffset const & A,
					OverlapData::OverlapOffset const & B
				) const
				{
					uint8_t const * pa = p + A.offset;
					uint8_t const * pb = p + B.offset;

					return compare(pa,pb);
				}
			};

			struct OverlapDataInterfaceBAComparator
			{
				uint8_t const * p;

				OverlapDataInterfaceBAComparator() : p(0) {}
				OverlapDataInterfaceBAComparator(uint8_t const * rp) : p(rp) {}

				static bool compare(
					uint8_t const * pa,
					uint8_t const * pb
				)
				{
					OverlapDataInterface const OA(pa);
					OverlapDataInterface const OB(pb);

					{
						int64_t const a = OA.bread();
						int64_t const b = OB.bread();

						if ( a != b )
							return a < b;
					}

					{
						int64_t const a = OA.aread();
						int64_t const b = OB.aread();

						if ( a != b )
							return a < b;
					}

					{
						int64_t const a = OA.isInverse();
						int64_t const b = OB.isInverse();

						if ( a != b )
							return a < b;
					}

					{
						int64_t const a = OA.abpos();
						int64_t const b = OB.abpos();

						if ( a != b )
							return a < b;
					}

					{
						int64_t const a = OA.aepos();
						int64_t const b = OB.aepos();

						if ( a != b )
							return a < b;
					}

					{
						int64_t const a = OA.bbpos();
						int64_t const b = OB.bbpos();

						if ( a != b )
							return a < b;
					}

					{
						int64_t const a = OA.bepos();
						int64_t const b = OB.bepos();

						if ( a != b )
							return a < b;
					}

					return false;
				}

				bool operator()(
					OverlapData::OverlapOffset const & A,
					OverlapData::OverlapOffset const & B
				) const
				{
					uint8_t const * pa = p + A.offset;
					uint8_t const * pb = p + B.offset;

					return compare(pa,pb);
				}
			};

			std::ostream & operator<<(std::ostream & out, OverlapDataInterface const & O);
		}
	}
}
#endif
