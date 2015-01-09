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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_FRAGMENTALIGNMENTBUFFERFRAGMENT_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_FRAGMENTALIGNMENTBUFFERFRAGMENT_HPP

#include <libmaus/autoarray/AutoArray.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			struct FragmentAlignmentBufferFragment
			{
				typedef FragmentAlignmentBufferFragment this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
				libmaus::autoarray::AutoArray<uint8_t,libmaus::autoarray::alloc_type_c> A;
				uint8_t * pa;
				uint8_t * pc;
				uint8_t * pe;
				size_t f;
				
				FragmentAlignmentBufferFragment() : pa(0), pc(0), pe(0), f(0) {}

				void reset()
				{
					pc = pa;
					f = 0;
				}
				
				void extend()
				{
					ptrdiff_t const off = pc-pa;
					size_t const sizeadd = std::max(static_cast<size_t>(1),static_cast<size_t>(A.size() / 16));
					size_t const newsize = A.size() + sizeadd;
					
					libmaus::autoarray::AutoArray<uint8_t,libmaus::autoarray::alloc_type_c> B(newsize,false);
					
					std::copy(pa,pc,B.begin());

					A = B;
					
					pa = A.begin();
					pc = pa + off;
					pe = A.end();
				}
				
				uint64_t getOffset() const
				{
					return static_cast<uint64_t>(pc-pa);
				}
				
				void pushAlignmentBlock(uint8_t const * T, size_t l)
				{
					while ( (pe-pc) < static_cast<ptrdiff_t>(l+sizeof(uint32_t)) )
						extend();
					assert ( static_cast<ssize_t>(pe-pc) >= static_cast<ssize_t>(l+sizeof(uint32_t)) );
					
					pc[0] = (l>>0) &0xFF;
					pc[1] = (l>>8) &0xFF;
					pc[2] = (l>>16)&0xFF;
					pc[3] = (l>>24)&0xFF;
					pc += 4;
					
					std::copy(T,T+l,pc);

					pc += l;
					f += 1;					
				}

				void push(uint8_t const * T, size_t l)
				{
					while ( (pe-pc) < static_cast<ptrdiff_t>(l) )
						extend();
					assert ( static_cast<ssize_t>(pe-pc) >= static_cast<ssize_t>(l) );
					
					std::copy(T,T+l,pc);
					pc += l;
				}
				
				void replaceLength(uint64_t const offset, uint32_t const l)
				{
					uint8_t * const pp = pa + offset;
					pp[0] = (l>>0) &0xFF;
					pp[1] = (l>>8) &0xFF;
					pp[2] = (l>>16)&0xFF;
					pp[3] = (l>>24)&0xFF;
				}
				
				uint32_t getLength(uint64_t const offset) const
				{
					return
						(static_cast<uint32_t>(pa[offset+0]) << 0) |
						(static_cast<uint32_t>(pa[offset+1]) << 8) |
						(static_cast<uint32_t>(pa[offset+2]) << 16) |
						(static_cast<uint32_t>(pa[offset+3]) << 24);
				}

				void getLinearOutputFragments(
					uint64_t const maxblocksize, std::vector<std::pair<uint8_t *,uint8_t *> > & V
				)
				{
					// total length of character data in bytes
					uint64_t const totallen = pc - pa;
					// target number of blocks
					uint64_t const tnumblocks = (totallen + maxblocksize - 1)/maxblocksize;
					// block size
					uint64_t const blocksize = tnumblocks ? ((totallen + tnumblocks - 1)/tnumblocks) : 0;
					// number of blocks
					uint64_t const numblocks = 
						std::max(blocksize ? ((totallen + blocksize - 1)/blocksize) : 0,static_cast<uint64_t>(1));
					
					for ( uint64_t o = 0; o < numblocks; ++o )
					{
						uint64_t const low  = o*blocksize;
						uint64_t const high = std::min(low+blocksize,totallen); 
						V.push_back(std::pair<uint8_t *,uint8_t *>(pa+low,pa+high));
					}					
				}

			};
		}
	}
}
#endif
