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
#if ! defined(CHECKOVERLAPRESULIDHEAPCOMPARATOR_HPP)
#define CHECKOVERLAPRESULIDHEAPCOMPARATOR_HPP

#include <libmaus/lcs/CheckOverlapResult.hpp>

namespace libmaus
{
	namespace lcs
	{
		struct CheckOverlapResultIdHeapComparator
		{
			CheckOverlapResultIdComparator subcomp;
		
			bool operator()(CheckOverlapResult const & A, CheckOverlapResult const & B) const
			{
				return ! subcomp(A,B);
			}

			bool operator()(std::pair<uint64_t,CheckOverlapResult::shared_ptr_type> const & A, std::pair<uint64_t,CheckOverlapResult::shared_ptr_type> const & B) const
			{
				return ! subcomp(*(A.second),*(B.second));
			}
		};
	}
}
#endif
