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
#if ! defined(LIBMAUS2_BAMBAM_BAMALIGNMENTREG2BIN_HPP)
#define LIBMAUS2_BAMBAM_BAMALIGNMENTREG2BIN_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>

namespace libmaus2
{
	namespace bambam
	{
		struct BamAlignmentReg2Bin
		{
			/* reg2bin as defined in CSI specs, somewhat reformatted (see https://github.com/samtools/hts-specs/blob/master/CSIv1.tex) */
			static inline int reg2bin(int64_t const beg, int64_t end, int const min_shift = 14, int const depth = 5)
			{
				int l = depth;
				int s = min_shift;
				int t = ((1<<depth*3) - 1) / 7;

				end -= 1;

				for ( ; l > 0; --l, s += 3, t -= 1<<l*3 )
				{
					if (beg>>s == end>>s)
						return t + (beg>>s);
				}

				return 0;
			}
			
			/* calculate the list of bins that may overlap with region [beg,end) (zero-based) */
			static inline uint64_t reg2bins(int64_t beg, int64_t end, libmaus2::autoarray::AutoArray<int> & Abins, int const min_shift = 14, int const depth = 5)
			{
				uint64_t o = 0;

				if ( end > beg )
				{
					int l = depth;
					int s = min_shift;
					int t = ((1<<depth*3) - 1) / 7;

					end -= 1;

					for ( ; l > 0; --l, s += 3, t -= 1<<l*3 )
					{
						int64_t const from = t + ( beg >> s );
						int64_t const to = t + ( end >> s );
						
						for ( int64_t i = from; i <= to; ++i )
							Abins.push(o,i);
					}
				}

				return o;
			}
		};
	}
}
#endif
