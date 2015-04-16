/*
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
*/
#if ! defined(LIBMAUS_LZ_BGZFINFLATEBLOCKIDINFO_HPP)
#define LIBMAUS_LZ_BGZFINFLATEBLOCKIDINFO_HPP

#include <libmaus/lz/BgzfInflateBlock.hpp>
#include <libmaus/lz/BgzfThreadOpBase.hpp>

namespace libmaus
{
	namespace lz
	{
		struct BgzfInflateBlockIdInfo
		{
			libmaus::autoarray::AutoArray<libmaus::lz::BgzfInflateBlock::unique_ptr_type> const & inflateB;
					
			BgzfInflateBlockIdInfo(
				libmaus::autoarray::AutoArray<libmaus::lz::BgzfInflateBlock::unique_ptr_type> const & rinflateB
			) : inflateB(rinflateB)
			{
			
			}

			uint64_t operator()(uint64_t const i) const
			{
				return inflateB[i]->blockid;
			}		

			BgzfThreadQueueElement const & operator()(BgzfThreadQueueElement const & i) const
			{
				return i;
			}
		};
	}
}
#endif
