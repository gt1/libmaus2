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
		struct BamAlignmentPosComparator
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
			static bool compare(uint8_t const * da, uint8_t const * db)
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
			static int compareInt(uint8_t const * da, uint8_t const * db)
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
			 * compare alignments at offsets a and b relative to the data block pointer passed to this object
			 * at construction time
			 *
			 * @param a offset of first alignment
			 * @param b offset of second alignment
			 * @return true iff a < b concerning mapping coordinates
			 **/
			bool operator()(uint64_t const a, uint64_t const b) const
			{
				return compare(data + a + sizeof(uint32_t),data + b + sizeof(uint32_t));
			}

			/**
			 * compare alignments at offsets a and b relative to the data block pointer passed to this object
			 * at construction time
			 *
			 * @param a offset of first alignment
			 * @param b offset of second alignment
			 * @return -1 if a<b, 0 if a=b, 1 if a>b concerning mapping coordinates
			 **/
			int compareInt(uint64_t const a, uint64_t const b) const
			{
				return compareInt(data + a + sizeof(uint32_t),data + b + sizeof(uint32_t));
			}
		};
	}
}
#endif
