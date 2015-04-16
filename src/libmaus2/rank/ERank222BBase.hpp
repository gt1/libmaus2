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
#if ! defined(LIBMAUS_RANK_ERANK222BBASE)
#define LIBMAUS_RANK_ERANK222BBASE

#include <libmaus2/types/types.hpp>

namespace libmaus2
{
	namespace rank
	{
		struct ERank222BBase
		{
			protected:
			// super block size 2^16 bits
			static unsigned int const sbbitwidth = 16;
			// mini block size 2^6 = 64 bits
			static unsigned int const mbbitwidth = 6;
			
			// derived numbers
			
			// actual block sizes
			static uint64_t const sbsize = 1u << sbbitwidth;
			static uint64_t const mbsize = 1u << mbbitwidth;
			
			// superblock mask
			static uint64_t const sbmask = (1u<<sbbitwidth)-1;
			// miniblock mask
			static uint64_t const mbmask = (1u<<mbbitwidth)-1;
			// superblock/miniblock mask
			static uint64_t const sbmbmask = (1ull << (sbbitwidth-mbbitwidth))-1;
			// superblock/miniblock shift
			static unsigned int const sbmbshift = sbbitwidth-mbbitwidth;

			// division rounding up
			static inline uint64_t divUp(uint64_t a, uint64_t b) { return (a+b-1)/b; }
		};
	}
}
#endif
