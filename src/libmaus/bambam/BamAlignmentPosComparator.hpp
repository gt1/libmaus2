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
				
				#if 1
				return posa < posb;
				#else
				if ( posa != posb )
					return posa < posb;
				
				char const * namea = ::libmaus::bambam::BamAlignmentDecoderBase::getReadName(da);
				char const * nameb = ::libmaus::bambam::BamAlignmentDecoderBase::getReadName(db);

				int const r = ::bambam::BamAlignmentNameComparator::strcmpnum(namea,nameb);
				bool res;
				
				if ( r < 0 )
				{
					res = true;
				}
				else if ( r == 0 )
				{
					// read 1 before read 2
					res = ::libmaus::bambam::BamAlignmentDecoderBase::getFlags(da) & ::libmaus::bambam::BamFlagBase::SUDS_BAMBAM_FREAD1;
				}
				else
				{
					res = false;
				}
				
				return res;
				
				#endif
			}
			
			bool operator()(uint64_t const a, uint64_t const b) const
			{
				return compare(data + a + sizeof(uint32_t),data + b + sizeof(uint32_t));
			}
		};
	}
}
#endif
