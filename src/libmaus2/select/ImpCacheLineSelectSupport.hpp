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

#if ! defined(IMPCACHELINESELECT_HPP)
#define IMPCACHELINESELECT_HPP

#include <libmaus2/rank/ImpCacheLineRank.hpp>
#include <libmaus2/select/ESelectBase.hpp>

namespace libmaus2
{
	namespace select
	{
		struct ImpCacheLineSelectSupport : public ::libmaus2::select::ESelectBase<1>
		{
			typedef ::libmaus2::select::ESelectBase<1> base_type;
			typedef ImpCacheLineSelectSupport this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			::libmaus2::rank::ImpCacheLineRank const & ICLR;
			unsigned int const slog;
			uint64_t const s;
			uint64_t const n1;
			::libmaus2::autoarray::AutoArray<uint64_t> const sdict;
			
			::libmaus2::autoarray::AutoArray<uint64_t> constructSdict() const
			{
				::libmaus2::autoarray::AutoArray<uint64_t> sdict((n1 + s-1)/s);
			
				assert ( ICLR.A.size() % 8 == 0 );
				uint64_t const numblocks = ICLR.A.size()/8;
				uint64_t const * p = ICLR.A.begin();
				uint64_t const smask = s-1;
				
				uint64_t q = 0;
				for ( uint64_t b = 0; b < numblocks; ++b )
				{
					p += 2;

					for ( uint64_t j = 0; j < 6; ++j )
					{
						uint64_t v = *(p++);

						for ( uint64_t mask = 1ull<<63; mask; mask >>= 1 )
							if ( v & mask )
							{
								if ( (q & smask) == 0 )
									sdict[q>>slog] = b;
								++q;
							}
					}
				}
			
				return sdict;
			}
			
			ImpCacheLineSelectSupport(::libmaus2::rank::ImpCacheLineRank const & rICLR, unsigned int const rslog)
			: ICLR(rICLR), slog(rslog), s(1ull << slog),
			  n1(ICLR.n ? ICLR.rank1(ICLR.n-1) : 0), sdict(constructSdict())
			{
			}
			
			uint64_t next1(uint64_t r) const
			{
				assert ( ICLR[r] );
				while ( ! ICLR[++r] ) {}
				assert ( ICLR[r] );
				return r;
			}
			
			uint64_t next1NotAdjacent(uint64_t r) const
			{
				assert ( ICLR[r] );
				r += 2;
				while ( ! ICLR[r] )
					r++;
				return r;
			}
			
			uint64_t select1(uint64_t const r) const
			{
				// find lower bound block by lookup
				uint64_t b = sdict [ r >> slog ];
				// blocks are 8 words wide
				uint64_t bb = b*8;

				#if 0
				assert ( ICLR.A [ bb ] <= r );
				#endif
				
				// go to next block until we have reached the correct one
				while ( r >= ICLR.A[bb] + (ICLR.A[bb+1] >> (9*6)) )
					bb += 8;
				
				#if 0
				assert ( ICLR.A [ bb ] <= r );
				assert ( ICLR.A[bb] + (ICLR.A[bb+1] >> (9*6)) > r );
				#endif
				
				// rest of rank in block
				uint64_t const rb = r - ICLR.A[bb];
				
				// now find correct mini block (word in block)
				uint64_t subwordp = 1;
				uint64_t const subblockpc = ICLR.A[bb+1];

				static uint64_t const submask = (1ull<<9)-1;
				while ( rb >= ((subblockpc >> (subwordp*9))&submask) )
					subwordp++;
					
				uint64_t const subword = subwordp-1;
				
				#if 0
				assert ( rb >= ((subblockpc >> (subword*9))&submask) );
				assert ( rb  < ((subblockpc >> ((subword+1)*9))&submask) );
				#endif
				
				// std::cerr << "r=" << r << " block= " << (bb/8) << " subblock=" << subword << std::endl;
				
				// rest rank in word
				uint64_t rbb = rb - ((subblockpc >> (subword*9))&submask);
				
				// word
				uint64_t v = ICLR.A[bb+subword+2];
				
				// now find correct position in word
				uint64_t woff = 0;
				uint64_t lpcnt;

				// 64 bits left, figure out if bit is in upper or lower half of word
				if ( rbb >= (lpcnt=::libmaus2::rank::PopCnt4<sizeof(int)>::popcnt4(v >> 32)) )
					rbb -= lpcnt, v &= 0xFFFFFFFFUL, woff += 32;
				else
					v >>= 32;
				// 32 bits left
				if ( rbb >= (lpcnt=::libmaus2::rank::PopCnt4<sizeof(int)>::popcnt4(v >> 16)) )
					rbb -= lpcnt, v &= 0xFFFFUL, woff += 16;
				else
					v >>= 16;

				// 16 bits left, use lookup table for rest
				return (((bb>>3)*6+subword)<<6)+woff + base_type::R [ ((v&0xFFFFull) << 4) | rbb ];
			}
		};
	}
}
#endif
