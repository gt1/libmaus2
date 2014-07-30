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
#if ! defined(LIBMAUS_UTIL_GROWINGFREELIST_HPP)
#define LIBMAUS_UTIL_GROWINGFREELIST_HPP

#include <libmaus/autoarray/AutoArray.hpp>

namespace libmaus
{
	namespace util
	{
		template<typename _element_type>
		struct GrowingFreeList
		{
			typedef _element_type element_type;
			typedef GrowingFreeList<element_type> this_type;

			private:
			libmaus::autoarray::AutoArray<element_type *> alloclist;
			libmaus::autoarray::AutoArray<element_type *> freelist;
			uint64_t freelistfill;

			void cleanup()
			{
				for ( uint64_t i = 0; i < alloclist.size(); ++i )
					delete alloclist[i];
				alloclist = libmaus::autoarray::AutoArray<element_type *>(0);	
				freelist = libmaus::autoarray::AutoArray<element_type *>(0);	
				freelistfill = 0;
			}
			
			public:
			GrowingFreeList()
			: alloclist(0), freelist(0), freelistfill(0)
			{
				
			}
				
			~GrowingFreeList()
			{
				cleanup();
			}
			
			element_type * get()
			{
				if ( ! freelistfill )
				{
					// allocate more alignment objects
					libmaus::autoarray::AutoArray<element_type *> nalloclist(
						std::max(
							static_cast<uint64_t>(1),
							static_cast<uint64_t>(2*alloclist.size())
						)
						,false
					);

					std::copy(alloclist.begin(),alloclist.end(),nalloclist.begin());
					element_type * nullp = 0;
					std::fill(nalloclist.begin()+alloclist.size(),nalloclist.end(),nullp);
					
					for ( element_type ** p = nalloclist.begin()+alloclist.size();
						p != nalloclist.end(); ++p )
						*p = new element_type;
					
					libmaus::autoarray::AutoArray<element_type *> nfreelist(
						std::max(
							static_cast<uint64_t>(1),
							static_cast<uint64_t>(2*freelist.size())
						)
						,false			
					);
					
					std::copy(freelist.begin(),freelist.end(),nfreelist.begin());
					std::fill(nfreelist.begin()+freelist.size(),nfreelist.end(),nullp);
				
					freelist = nfreelist;
					
					for ( element_type ** p = nalloclist.begin()+alloclist.size();
						p != nalloclist.end(); ++p )
						freelist[freelistfill++] = *p;			
					
					alloclist = nalloclist;
				}
				
				return freelist[--freelistfill];
			}
			
			void put(element_type * p)
			{
				freelist[freelistfill++] = p;
			}
		};
	}
}
#endif
