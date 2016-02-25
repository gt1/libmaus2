/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#if ! defined(LIBMAUS2_FASTX_COORDINATECACHE_HPP)
#define LIBMAUS2_FASTX_COORDINATECACHE_HPP

#include <libmaus2/fastx/DNAIndexMetaDataBigBand.hpp>
#include <libmaus2/rank/DNARank.hpp>

namespace libmaus2
{
	namespace fastx
	{
		struct CoordinateCache
		{
			unsigned int const blockshift;
			uint64_t const n;
			libmaus2::fastx::DNAIndexMetaDataBigBand const & index;
			uint64_t const blocksize;
			uint64_t const numblocks;
			libmaus2::autoarray::AutoArray<int64_t> C;

			CoordinateCache(libmaus2::rank::DNARank const & rrank, libmaus2::fastx::DNAIndexMetaDataBigBand const & rindex, unsigned int const rblockshift = 16)
			: blockshift(rblockshift), n(rrank.size()), index(rindex), blocksize(1ull << blockshift), numblocks((n + blocksize-1)/blocksize), C(numblocks)
			{
				for ( uint64_t b = 0; b < numblocks; ++b )
				{
					uint64_t const low = b * blocksize;
					uint64_t const high = std::min(n,low+blocksize);
					assert ( high > low );

					std::pair<uint64_t,uint64_t> const coordlow = index.mapCoordinates(low);
					std::pair<uint64_t,uint64_t> const coordhigh = index.mapCoordinates(high-1);

					if ( coordlow.first == coordhigh.first )
					{
						C[b] = coordlow.first;
					}
					else
					{
						C[b] = -1;
					}
				}
			}

			std::pair<uint64_t,uint64_t> operator[](uint64_t const i) const
			{
				int64_t const l = C[i >> blockshift];

				if ( l < 0 )
					return index.mapCoordinates(i);
				else
					return std::pair<uint64_t,uint64_t>(l,i - index.L[l]);
			}
		};
	}
}
#endif
