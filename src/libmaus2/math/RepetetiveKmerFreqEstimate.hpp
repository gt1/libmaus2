/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#if ! defined(LIBMAUS2_MATH_REPETETIVEKMERFREQESTIMATE_HPP)
#define LIBMAUS2_MATH_REPETETIVEKMERFREQESTIMATE_HPP

#include <libmaus2/math/binom.hpp>

namespace libmaus2
{
	namespace math
	{
		struct RepetetiveKmerFreqEstimate
		{
			static std::pair<uint64_t,double> estimateNumOcc(
				uint64_t const replen,
				uint64_t const period,
				uint64_t const k,
				double const thres = 0.99,
				double const e = 0.15
			)
			{
				double const p = 1.0 - 2.0*e; // correct base probability (two reads)
				double const pk = std::pow(p,k);
				uint64_t const numinst = replen / period;
			
				uint64_t i = 0;
					
				while ( libmaus2::math::Binom::binomRowUpper(pk,i,numinst) >= thres )
					++i;
						
				if ( i )
					i -= 1;
					
				return std::pair<uint64_t,double>(i,libmaus2::math::Binom::binomRowUpper(pk,i,numinst));
			}
		};
	}
}
#endif
