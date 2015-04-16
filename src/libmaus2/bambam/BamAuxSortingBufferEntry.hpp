/*
    libmaus2
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
#if ! defined(LIBMAUS2_BAMBAM_BAMAUXSORTINGBUFFERENTRY_HPP)
#define LIBMAUS2_BAMBAM_BAMAUXSORTINGBUFFERENTRY_HPP

#include <utility>
#include <libmaus2/types/types.hpp>

namespace libmaus2
{
	namespace bambam
	{
		//! entry class for sorting BAM aux tag blocks
		struct BamAuxSortingBufferEntry
		{
			//! tag id
			std::pair<uint8_t,uint8_t> c;
			//! block offset
			uint64_t offset;
			//! block length
			uint64_t length;
			
			/**
			 * construct for empty/invalid entry
			 **/
			BamAuxSortingBufferEntry() : c(0,0), offset(0), length(0) {}
			/**
			 * constructor by block parameters
			 *
			 * @param ca first character of id
			 * @param cb second charactoer of id
			 * @param roffset block offset (in bytes)
			 * @param rlength block lengt (in bytes)
			 **/
			BamAuxSortingBufferEntry(uint8_t const ca, uint8_t const cb, uint64_t const roffset, uint64_t const rlength) : c(ca,cb), offset(roffset), length(rlength) {}
			
			/**
			 * comparator
			 *
			 * @param o other entry
			 * @return true iff *this < o lexicographically
			 **/
			bool operator<(BamAuxSortingBufferEntry const & o) const
			{
				if ( c != o.c )
					return c < o.c;
				else if ( offset < o.offset )
					return offset < o.offset;
				else
					return length < o.length;
			}
		};
	}
}
#endif
