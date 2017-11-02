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
				uint64_t decodeTraceVector(libmaus2::autoarray::AutoArray<std::pair<uint16_t,uint16_t> > & A, int64_t const tspace)
				{ return OverlapData::decodeTraceVector(p,A,libmaus2::dazzler::align::AlignmentFile::tspaceToSmall(tspace)); }
                                double getErrorRate() const
                                { return (aepos() > abpos()) ? (static_cast<double>(diffs()) / static_cast<double>(aepos()-abpos())) : 0.0; }

			};

			std::ostream & operator<<(std::ostream & out, OverlapDataInterface const & O);
		}
	}
}
#endif
