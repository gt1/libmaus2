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
#if ! defined(LIBMAUS2_BAMBAM_MERGEQUEUEELEMENT_HPP)
#define LIBMAUS2_BAMBAM_MERGEQUEUEELEMENT_HPP

#include <libmaus2/bambam/BamAlignment.hpp>
#include <cstring>

namespace libmaus2
{
	namespace bambam
	{
		/**
		 * merge queue element for collating BAM decoder
		 **/
		struct MergeQueueElement
		{
			//! alignment
			::libmaus2::bambam::BamAlignment::shared_ptr_type algn;
			//! input list index
			uint64_t index;

			/**
			 * constructor for invalid/null object
			 **/
			MergeQueueElement()
			{
			}

			/**
			 * construct from alignment and input list index
			 *
			 * @param ralgn alignment
			 * @param rindex index of input list
			 **/
			MergeQueueElement(::libmaus2::bambam::BamAlignment::shared_ptr_type ralgn, uint64_t const rindex)
			: algn(ralgn), index(rindex) {}

			/**
			 * compare two alignments by name
			 *
			 * @param A first alignment
			 * @param B second alignment
			 * @return true iff A.name > B.name or A.name = B.name and A is read 2
			 **/
			static bool compare(
				::libmaus2::bambam::BamAlignment::shared_ptr_type const & A,
				::libmaus2::bambam::BamAlignment::shared_ptr_type const & B)
			{
				int r = strcmp(A->getName(),B->getName());

				if ( r )
					return r > 0;

				return A->isRead2();
			}

			/**
			 * comparison operator calling compare method
			 *
			 * @param o other merge queue element
			 * @return compare(algn,o.algn)
			 **/
			bool operator<(MergeQueueElement const & o) const
			{
				return compare(algn,o.algn);
			}
		};
	}
}
#endif
