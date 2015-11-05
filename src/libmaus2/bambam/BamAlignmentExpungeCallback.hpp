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
#if ! defined(LIBMAUS2_BAMBAM_BAMALIGNMENTEXPUNGECALLBACK_HPP)
#define LIBMAUS2_BAMBAM_BAMALIGNMENTEXPUNGECALLBACK_HPP

#include <libmaus2/types/types.hpp>

namespace libmaus2
{
	namespace bambam
	{
		/**
		 * callback interface for BamAlignment expung operations
		 **/
		struct BamAlignmentExpungeCallback
		{
			/**
			 * destructor
			 **/
			virtual ~BamAlignmentExpungeCallback() {}

			/**
			 * called whenever an alignment is expunged from a data structure
			 *
			 * @param D alignment data block
			 * @param len length of D in bytes
			 **/
			virtual void expunged(uint8_t const * D, uint32_t const len) = 0;
			/**
			 * called whenever an alignment is expunged from a data structure
			 * and is written in two blocks from a circular buffer with wrap around
			 *
			 * @param Da alignment data block, first part
			 * @param lena length of Da in bytes
			 * @param Db alignment data block, second part
			 * @param lenb length of Db in bytes
			 **/
			virtual void expunged(
				uint8_t const * Da, uint32_t const lena,
				uint8_t const * Db, uint32_t const lenb
				) = 0;
		};
	}
}
#endif
