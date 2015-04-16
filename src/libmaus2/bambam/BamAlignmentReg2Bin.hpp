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
#if ! defined(LIBMAUS_BAMBAM_BAMALIGNMENTREG2BIN_HPP)
#define LIBMAUS_BAMBAM_BAMALIGNMENTREG2BIN_HPP

#include <libmaus/types/types.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct BamAlignmentReg2Bin
		{
			/**
			 * reg2bin as defined in sam file format spec 
			 *
			 * @param beg alignment start (inclusive)
			 * @param end alignment end (exclusive)
			 * @return bin for alignment interval
			 **/
			static inline int reg2bin(uint32_t beg, uint32_t end)
			{
				--end;
				if (beg>>14 == end>>14) return ((1ul<<15)-1ul)/7ul + (beg>>14);
				if (beg>>17 == end>>17) return ((1ul<<12)-1ul)/7ul + (beg>>17);
				if (beg>>20 == end>>20) return ((1ul<<9)-1ul)/7ul  + (beg>>20);
				if (beg>>23 == end>>23) return ((1ul<<6)-1ul)/7ul + (beg>>23);
				if (beg>>26 == end>>26) return ((1ul<<3)-1ul)/7ul + (beg>>26);
				return 0;
			}		
		};
	}
}
#endif
