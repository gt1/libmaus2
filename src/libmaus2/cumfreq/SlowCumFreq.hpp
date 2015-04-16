/**
    libmaus2
    Copyright (C) 2012 German Tischler

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

#if ! defined(SLOWCUMFREQ_HPP)
#define SLOWCUMFREQ_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>

namespace libmaus2
{
	namespace cumfreq
	{
		struct SlowCumFreq
		{
			uint64_t s;
			::libmaus2::autoarray::AutoArray<uint64_t> A;
			
			SlowCumFreq(uint64_t rs)
			: s(rs), A(s,false)
			{
				std::fill(A.get(),A.get()+s,0);
			}
			
			uint64_t operator[](uint64_t i) const
			{
				return A[i];
			}
			void inc(uint64_t c)
			{
				for ( uint64_t i = c+1; i < s; ++i )
					A[i] += 1;
			}
		};
	}
}
#endif

