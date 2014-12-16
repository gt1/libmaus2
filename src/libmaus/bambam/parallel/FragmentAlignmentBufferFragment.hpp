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
				
				uint64_t push(uint8_t const * T, size_t l)
				{
					ptrdiff_t const off = pc-pa;
					
					while ( (pe-pc) < (l+sizeof(uint32_t)) )
						extend();
					assert ( pe-pc >= l+sizeof(uint32_t) );
					
					pc[0] = (l>>0) &0xFF;
					pc[1] = (l>>8) &0xFF;
					pc[2] = (l>>16)&0xFF;
					pc[3] = (l>>24)&0xFF;
					pc += 4;
					
					std::copy(T,T+l,pc);

					pc += l;
					f += 1;
					
					return static_cast<uint64_t>(off);
				}
				
				uint32_t getLength(uint64_t const offset) const
				{
					return
						(static_cast<uint32_t>(pa[offset+0]) << 0) |
						(static_cast<uint32_t>(pa[offset+1]) << 8) |
						(static_cast<uint32_t>(pa[offset+2]) << 16) |
						(static_cast<uint32_t>(pa[offset+3]) << 24);
				}
			};
		}
	}
}
#endif
