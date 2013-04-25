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
#if ! defined(LIBMAUS_BAMBAM_BAMALIGNMENTDECODER_HPP)
#define LIBMAUS_BAMBAM_BAMALIGNMENTDECODER_HPP

#include <libmaus/bambam/BamAlignment.hpp>
#include <libmaus/bambam/BamHeader.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct BamAlignmentDecoder
		{
			bool putbackbuffer;
		
			BamAlignmentDecoder() : putbackbuffer(false) {}
			virtual ~BamAlignmentDecoder() {}
			
			virtual bool readAlignmentInternal(bool const = false) = 0;
			virtual libmaus::bambam::BamAlignment const & getAlignment() const = 0;
			virtual libmaus::bambam::BamHeader const & getHeader() const = 0;

			void putback()
			{
				putbackbuffer = true;
			}
			
			bool readAlignment(bool const delayPutRank = false)
			{
				if ( putbackbuffer )
				{
					putbackbuffer = false;
					return true;
				}
				
				return readAlignmentInternal(delayPutRank);
			}

			libmaus::bambam::BamAlignment::unique_ptr_type ualignment() const
			{
				return UNIQUE_PTR_MOVE(getAlignment().uclone());
			}
			
			libmaus::bambam::BamAlignment::shared_ptr_type salignment() const
			{
				return getAlignment().sclone();
			}

		};
	}
}
#endif
