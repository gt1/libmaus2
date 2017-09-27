/*
    libmaus2
    Copyright (C) 2009-2013 German Tischler
    Copyright (C) 2011-2013 Genome Research Limited

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
#if ! defined(LIBMAUS2_BAMBAM_BAMALIGNMENTPOSCOMPARATOR_HPP)
#define LIBMAUS2_BAMBAM_BAMALIGNMENTPOSCOMPARATOR_HPP

#include <libmaus2/bambam/BamAlignmentDecoderBase.hpp>
#include <libmaus2/bambam/BamAlignmentNameComparator.hpp>

namespace libmaus2
{
	namespace bambam
	{
		/**
		 * class for comparing BAM alignment blocks by coordinates
		 **/
		struct BamAlignmentPosComparator : public DecoderBase
		{
			//! data block pointer
			uint8_t const * data;

			/**
			 * construct
			 *
			 * @param rdata data block pointer
			 **/
			BamAlignmentPosComparator(uint8_t const * rdata) : data(rdata) {}

			/**
			 * compare alignment blocks at da and db
			 *
			 * @param da pointer to first alignment block
			 * @param db pointer to second alignment block
			 * @return true iff da < db concerning mapping coordinates
			 **/
			static bool compare(uint8_t const * da, uint64_t const /* la */, uint8_t const * db, uint64_t const /* lb */)
			{
				int32_t const refa = ::libmaus2::bambam::BamAlignmentDecoderBase::getRefID(da);
				int32_t const refb = ::libmaus2::bambam::BamAlignmentDecoderBase::getRefID(db);

				if ( refa != refb )
					return  static_cast<uint32_t>(refa) < static_cast<uint32_t>(refb);

				int32_t const posa = ::libmaus2::bambam::BamAlignmentDecoderBase::getPos(da);
				int32_t const posb = ::libmaus2::bambam::BamAlignmentDecoderBase::getPos(db);

				return posa < posb;
			}

			/**
			 * compare alignment blocks at da and db
			 *
			 * @param da pointer to first alignment block
			 * @param db pointer to second alignment block
			 * @return -1 if da<db, 0 if da=db, 1 if da>db concerning mapping coordinates
			 **/
			static int compareInt(uint8_t const * da, uint64_t const /* la */, uint8_t const * db, uint64_t const /* lb */)
			{
				int32_t const refa = ::libmaus2::bambam::BamAlignmentDecoderBase::getRefID(da);
				int32_t const refb = ::libmaus2::bambam::BamAlignmentDecoderBase::getRefID(db);

				if ( refa != refb )
				{
					if ( static_cast<uint32_t>(refa) < static_cast<uint32_t>(refb) )
						return -1;
					else
						return 1;
				}

				int32_t const posa = ::libmaus2::bambam::BamAlignmentDecoderBase::getPos(da);
				int32_t const posb = ::libmaus2::bambam::BamAlignmentDecoderBase::getPos(db);

				if ( posa < posb )
					return -1;
				else if ( posa > posb )
					return 1;
				else
					return 0;
			}

			/**
			 * compare alignments at offsets a and b in the data block
			 *
			 * @param a first offset into data block
			 * @param b second offset into data block
			 * @return true iff alignment at a < alignment at b (alignments at offset a and b, not offsets and b)
			 **/
			bool operator()(uint64_t const a, uint64_t const b) const
			{
				uint8_t const * da = data + a;
				uint64_t const la = static_cast<int32_t>(getLEInteger(da,4));
				uint8_t const * db = data + b;
				uint64_t const lb = static_cast<int32_t>(getLEInteger(db,4));
				return compare(da+sizeof(uint32_t),la,db+sizeof(uint32_t),lb);
			}

			/**
			 * compare alignments at offsets a and b in the data block
			 *
			 * @param a first offset into data block
			 * @param b second offset into data block
			 * @return -1 for a < b, 0 for a == b, 1 for a > b (alignments at offset a and b, not offsets and b)
			 **/
			int compareInt(uint64_t const a, uint64_t const b) const
			{
				uint8_t const * da = data + a;
				uint64_t const la = static_cast<int32_t>(getLEInteger(da,4));
				uint8_t const * db = data + b;
				uint64_t const lb = static_cast<int32_t>(getLEInteger(db,4));
				return compareInt(da+sizeof(uint32_t),la,db+sizeof(uint32_t),lb);
			}
		};
	}
}
#endif
