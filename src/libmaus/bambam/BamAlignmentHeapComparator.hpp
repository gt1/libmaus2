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
#if ! defined(LIBMAUS_BAMBAM_BAMALIGNMENTHEAPCOMPARATOR_HPP)
#define LIBMAUS_BAMBAM_BAMALIGNMENTHEAPCOMPARATOR_HPP

#include <libmaus/bambam/BamAlignment.hpp>

namespace libmaus
{
	namespace bambam
	{
		template<typename _comparator_type>
		struct BamAlignmentHeapComparator
		{
			typedef _comparator_type comparator_type;
			comparator_type const & comp;
			::libmaus::bambam::BamAlignment const * A;

			BamAlignmentHeapComparator(comparator_type const & rcomp, ::libmaus::bambam::BamAlignment const * rA) : comp(rcomp), A(rA)
			{
			
			}

			bool operator()(uint64_t const a, uint64_t const b)
			{
				uint8_t const * const da = A[a].D.begin();
				uint8_t const * const db = A[b].D.begin();
				
				bool const b_lt_a = comparator_type::compare(db,da);
				
				if ( b_lt_a )
					return true;

				bool const a_lt_b = comparator_type::compare(da,db);
				
				if ( a_lt_b )
					return false;
			
				return a > b;
			}
		};
	}
}
#endif
