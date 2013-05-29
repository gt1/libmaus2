/*
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
*/
#if ! defined(LIBMAUS_BAMBAM_BAMALIGNMENTCOMPARATOR_HPP)
#define LIBMAUS_BAMBAM_BAMALIGNMENTCOMPARATOR_HPP

#include <libmaus/bambam/BamAlignment.hpp>

namespace libmaus
{
	namespace bambam
	{
		/**
		 * bam alignment simple name comparator using pure lexicographic strcmp order
		 **/
		struct BamAlignmentComparator
		{
			/**
			 * compare alignments A and B concerning read names via strcmp and if the names
			 * are equal then A < B if A is marked as read 1
			 *
			 * @param A alignment
			 * @param B alignment
			 * @return A < B via simple read name comparison
			 **/
			bool operator()(
				::libmaus::bambam::BamAlignment::shared_ptr_type const & A,
				::libmaus::bambam::BamAlignment::shared_ptr_type const & B) const
			{
				int r = strcmp(A->getName(),B->getName());
				
				if ( r )
					return r < 0;

				return A->isRead1();
			}
		};
	}
}
#endif
