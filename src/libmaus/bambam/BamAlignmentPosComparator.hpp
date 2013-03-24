/**
    libmaus
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
**/
#if ! defined(LIBMAUS_BAMBAM_BAMALIGNMENTPOSCOMPARATOR_HPP)
#define LIBMAUS_BAMBAM_BAMALIGNMENTPOSCOMPARATOR_HPP

#include <libmaus/bambam/BamAlignmentDecoderBase.hpp>
#include <libmaus/bambam/BamAlignmentNameComparator.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct BamAlignmentPosComparator
		{
			uint8_t const * data;
			
			BamAlignmentPosComparator(uint8_t const * rdata) : data(rdata) {}
			
			static bool compare(uint8_t const * da, uint8_t const * db)
			{
				int32_t const refa = ::libmaus::bambam::BamAlignmentDecoderBase::getRefID(da);
				int32_t const refb = ::libmaus::bambam::BamAlignmentDecoderBase::getRefID(db);
				
				if ( refa != refb )
					return  static_cast<uint32_t>(refa) < static_cast<uint32_t>(refb);

				int32_t const posa = ::libmaus::bambam::BamAlignmentDecoderBase::getPos(da);
				int32_t const posb = ::libmaus::bambam::BamAlignmentDecoderBase::getPos(db);
				
				return posa < posb;
			}

			static int compareInt(uint8_t const * da, uint8_t const * db)
			{
				int32_t const refa = ::libmaus::bambam::BamAlignmentDecoderBase::getRefID(da);
				int32_t const refb = ::libmaus::bambam::BamAlignmentDecoderBase::getRefID(db);
				
				if ( refa != refb )
				{
					if ( static_cast<uint32_t>(refa) < static_cast<uint32_t>(refb) )
						return -1;
					else
						return 1;
				}

				int32_t const posa = ::libmaus::bambam::BamAlignmentDecoderBase::getPos(da);
				int32_t const posb = ::libmaus::bambam::BamAlignmentDecoderBase::getPos(db);
				
				if ( posa < posb )
					return -1;
				else if ( posa > posb )
					return 1;
				else
					return 0;
			}
			
			bool operator()(uint64_t const a, uint64_t const b) const
			{
				return compare(data + a + sizeof(uint32_t),data + b + sizeof(uint32_t));
			}
			
			int compareInt(uint64_t const a, uint64_t const b) const
			{
				return compareInt(data + a + sizeof(uint32_t),data + b + sizeof(uint32_t));
			}
		};
	}
}
#endif
