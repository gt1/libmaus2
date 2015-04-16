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
#if ! defined(LIBMAUS2_BAMBAM_READENDSHEAPPAIRCOMPARATOR_HPP)
#define LIBMAUS2_BAMBAM_READENDSHEAPPAIRCOMPARATOR_HPP

#include <libmaus2/bambam/ReadEnds.hpp>

namespace libmaus2
{
	namespace bambam
	{
		/**
		 * merge heap comparator for ReadEnds objects
		 **/
		struct ReadEndsHeapPairComparator
		{
			//! pair type of merge list index and ReadEnds object
			typedef std::pair<uint64_t,ReadEnds> rpair;

			/**
			 * constructor
			 **/
			ReadEndsHeapPairComparator()
			{
			
			}
			
			/**
			 * compare two heap elements
			 *
			 * @param A first merge list, ReadEnds pair
			 * @param B second merge list, ReadEnds pair
			 * @return A.second>B.second if A.second != B.second, A.first < B.first otherwise
			 **/
			bool operator()(rpair const & A, rpair const & B) const
			{
				if ( A.second != B.second )
					return A.second > B.second;
				else
					return A.first < B.first;
			}
		};
	}
}
#endif
