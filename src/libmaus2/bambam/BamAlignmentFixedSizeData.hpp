/*
    libmaus
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS_BAMBAM_BAMALIGNMENTFIXEDSIZEDATA_HPP)
#define LIBMAUS_BAMBAM_BAMALIGNMENTFIXEDSIZEDATA_HPP

#include <libmaus/types/types.hpp>

namespace libmaus
{
	namespace bambam
	{
		#if defined(LIBMAUS_BYTE_ORDER_LITTLE_ENDIAN)
		#pragma pack(push,1)
		/**
		 * quick bam alignment base block access structure for little endian machines
		 **/
		struct BamAlignmentFixedSizeData
		{
			//! reference id
			int32_t  RefID;
			//! position on reference id
		        int32_t  Pos;
		        //! name length
		        uint8_t  NL;
		        //! mapping quality
		        uint8_t  MQ;
		        //! bin
		        uint16_t Bin;
		        //! number of cigar operations
		        uint16_t NC;
		        //! flags
		        uint16_t Flags;
		        // length of sequence
		        int32_t  Lseq;
		        //! mate/next segment reference id
		        int32_t  NextRefID;
		        //! mate/next segment position on reference id
		        int32_t  NextPos;
		        //! template length
		        int32_t  Tlen;
		};
		#pragma pack(pop)
		#endif
	}
}
#endif
