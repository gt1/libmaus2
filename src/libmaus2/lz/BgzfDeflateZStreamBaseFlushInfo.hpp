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
#if ! defined(LIBMAUS2_LZ_BGZFDEFLATEZSTREAMBASEFLUSHINFO_HPP)
#define LIBMAUS2_LZ_BGZFDEFLATEZSTREAMBASEFLUSHINFO_HPP

#include <libmaus2/types/types.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct BgzfDeflateZStreamBaseFlushInfo
		{
			unsigned int blocks;

			uint32_t block_a_u;
			uint32_t block_a_c;

			uint32_t block_b_u;
			uint32_t block_b_c;

			uint8_t * moveto;
			uint8_t * movefrom;
			uint64_t movesize;

			BgzfDeflateZStreamBaseFlushInfo()
			: blocks(0), block_a_u(0), block_a_c(0), block_b_u(0), block_b_c(0), moveto(0), movefrom(0), movesize(0) {}
			BgzfDeflateZStreamBaseFlushInfo(BgzfDeflateZStreamBaseFlushInfo const & o)
			: blocks(o.blocks), block_a_u(o.block_a_u), block_a_c(o.block_a_c), block_b_u(o.block_b_u), block_b_c(o.block_b_c), moveto(o.moveto), movefrom(o.movefrom), movesize(o.movesize)  {}

			BgzfDeflateZStreamBaseFlushInfo(uint32_t const r_block_a_u, uint32_t const r_block_a_c)
			: blocks(1), block_a_u(r_block_a_u), block_a_c(r_block_a_c), block_b_u(0), block_b_c(0), moveto(0), movefrom(0), movesize(0) {}

			BgzfDeflateZStreamBaseFlushInfo(uint32_t const r_block_a_u, uint32_t const r_block_a_c, uint8_t * rmoveto, uint8_t * rmovefrom, uint64_t rmovesize)
			: blocks(1), block_a_u(r_block_a_u), block_a_c(r_block_a_c), block_b_u(0), block_b_c(0), moveto(rmoveto), movefrom(rmovefrom), movesize(rmovesize) {}

			BgzfDeflateZStreamBaseFlushInfo(uint32_t const r_block_a_u, uint32_t const r_block_a_c, uint32_t const r_block_b_u, uint32_t const r_block_b_c)
			: blocks(2), block_a_u(r_block_a_u), block_a_c(r_block_a_c), block_b_u(r_block_b_u), block_b_c(r_block_b_c), moveto(0), movefrom(0), movesize(0) {}

			BgzfDeflateZStreamBaseFlushInfo & operator=(BgzfDeflateZStreamBaseFlushInfo const & o)
			{
				blocks = o.blocks;
				block_a_u = o.block_a_u;
				block_a_c = o.block_a_c;
				block_b_u = o.block_b_u;
				block_b_c = o.block_b_c;
				moveto = o.moveto;
				movefrom = o.movefrom;
				movesize = o.movesize;
				return *this;
			}

			uint64_t getCompressedSize() const
			{
				if ( blocks == 0 )
					return 0;
				else if ( blocks == 1 )
					return block_a_c;
				else
					return block_a_c + block_b_c;
			}

			uint8_t * moveUncompressedRest()
			{
				if ( movesize )
					::std::memmove(moveto,movefrom,movesize);

				return moveto + movesize;
			}
		};
	}
}
#endif
