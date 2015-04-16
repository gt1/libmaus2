/*
    libmaus2
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#if ! defined(LIBMAUS2_UTIL_FINITESIZEHEAP_HPP)
#define LIBMAUS2_UTIL_FINITESIZEHEAP_HPP

#include <libmaus2/autoarray/AutoArray.hpp>

namespace libmaus2
{
	namespace util
	{
		/**
		 * min heap class holding up to a finite number of elements, number is specified at
		 * construction time
		 **/
		template<typename _element_type, typename _comparator_type = std::less<_element_type> >
		struct FiniteSizeHeap
		{
			typedef _element_type element_type;
			typedef _comparator_type comparator_type;
			typedef FiniteSizeHeap<element_type,comparator_type> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;	
			
			libmaus2::autoarray::AutoArray<element_type> H;
			size_t f;
			comparator_type comp;
			
			FiniteSizeHeap(uint64_t const size, comparator_type rcomp = comparator_type())
			: H(size,false), f(0), comp(rcomp)
			{
			
			}
			
			bool empty() const
			{
				return (!f);
			}
			
			bool full() const
			{
				return f == H.size();
			}
			
			template<typename init_type>
			void pushset(init_type const & I)
			{
				size_t i = f++;
				H[i].set(I);
				
				// while not root
				while ( i )
				{
					// parent index
					size_t p = (i-1)>>1;
					
					// order wrong?
					if ( comp(H[i],H[p]) )
					{
						std::swap(H[i],H[p]);
						i = p;
					}
					// order correct, break loop
					else
					{
						break;
					}
				}
			}
			
			void push(element_type const & entry)
			{
				size_t i = f++;
				H[i] = entry;
				
				// while not root
				while ( i )
				{
					// parent index
					size_t p = (i-1)>>1;
					
					// order wrong?
					if ( comp(H[i],H[p]) )
					{
						std::swap(H[i],H[p]);
						i = p;
					}
					// order correct, break loop
					else
					{
						break;
					}
				}
			}
			
			element_type const & top()
			{
				return H[0];
			}
			
			void popvoid()
			{
				// put last element at root
				H[0] = H[--f];
				
				size_t i = 0;
				size_t r;
			
				// while both children exist	
				while ( (r=((i<<1)+2)) < f )
				{
					size_t const m = comp(H[r-1],H[r]) ? r-1 : r;
					
					// order correct?
					if ( comp(H[i],H[m]) )
						return;
					
					std::swap(H[i],H[m]);
					i = m;
				}
				
				// does left child exist?
				size_t l;
				if ( ((l = ((i<<1)+1)) < f) && (!(comp(H[i],H[l]))) )
					std::swap(H[i],H[l]);
			}
			
			void pop(element_type & E)
			{
				E = H[0];
				popvoid();
			}
			
			element_type pop()
			{
				element_type E;
				pop(E);
				return E;
			}
		};
	}
}
#endif
