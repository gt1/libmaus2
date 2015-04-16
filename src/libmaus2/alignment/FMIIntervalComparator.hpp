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
#if ! defined(LIBMAUS_ALIGNMENT_FMIINTERVALCOMPARATOR_HPP)
#define LIBMAUS_ALIGNMENT_FMIINTERVALCOMPARATOR_HPP

#include <libmaus/alignment/FactorMatchInfoPosComparator.hpp>
#include <vector>

namespace libmaus
{
	namespace alignment
	{
		struct FMIIntervalComparator
		{
			std::vector < libmaus::fm::FactorMatchInfo > const & FMI;
			
			FMIIntervalComparator(std::vector < libmaus::fm::FactorMatchInfo > const & rFMI)
			: FMI(rFMI)
			{
			
			}
			
			uint64_t getLengthSum(std::pair<uint64_t,uint64_t> const & A) const
			{
				uint64_t len = 0;
				for ( uint64_t i = A.first; i < A.second; ++i )
					len += FMI[i].len;
				return len;
			}
			
			bool operator()(std::pair<uint64_t,uint64_t> const & A, std::pair<uint64_t,uint64_t> const & B) const
			{
				return getLengthSum(A) > getLengthSum(B);
			}
		};
	}
}
#endif
