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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_SORTINGOVERLAPBLOCKINPUT_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_SORTINGOVERLAPBLOCKINPUT_HPP

#include <libmaus2/dazzler/align/OverlapHeapComparator.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/aio/InputStreamInstance.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct SortingOverlapBlockInput
			{
				typedef SortingOverlapBlockInput this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				
				libmaus2::aio::InputStreamInstance & ISI;
				bool const small;
				std::pair<uint64_t,uint64_t> block;
				libmaus2::autoarray::AutoArray<libmaus2::dazzler::align::Overlap> B;
				libmaus2::dazzler::align::Overlap * pa;
				libmaus2::dazzler::align::Overlap * pc;
				libmaus2::dazzler::align::Overlap * pe;
				
				bool haveprev;
				libmaus2::dazzler::align::Overlap OVLprev;
				
				SortingOverlapBlockInput(libmaus2::aio::InputStreamInstance & rISI, bool const rsmall, std::pair<uint64_t,uint64_t> rblock, uint64_t rn)
				: ISI(rISI), small(rsmall), block(rblock), B(rn), pa(B.begin()), pc(B.begin()), pe(B.begin()), haveprev(false)
				{
				
				}
				
				void fillBuffer()
				{
					assert ( pc == pe );
				
					if ( block.second )
					{
						ISI.clear();
						ISI.seekg(block.first);
						
						uint64_t s = 0;
						
						uint64_t const toread = std::min(block.second,B.size());
						
						for ( uint64_t i = 0; i < toread; ++i )
							libmaus2::dazzler::align::AlignmentFile::readOverlap(ISI,B[i],s,small);

						block.first += s;
						block.second -= toread;
						
						pa = B.begin();
						pc = B.begin();
						pe = B.begin() + toread;						
					}
				}

				bool getNext(libmaus2::dazzler::align::Overlap & OVL)
				{
					if ( pc == pe )
						fillBuffer();
					if ( pc == pe )
						return false;
					
					OVL = *(pc++);
					
					if ( haveprev )
					{
						bool const ok = !(OVL < OVLprev);
						assert ( ok );
					}
					
					haveprev = true;
					OVLprev = OVL;
					
					return true;
				}
			};
		}
	}
}
#endif
