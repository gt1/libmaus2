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
#if ! defined(LIBMAUS_UTIL_FREELIST_HPP)
#define LIBMAUS_UTIL_FREELIST_HPP

#include <libmaus/autoarray/AutoArray.hpp>

namespace libmaus
{
	namespace util
	{
		template<typename _element_type>
		struct FreeList
		{
			typedef _element_type element_type;
			typedef FreeList<element_type> this_type;
			typedef typename libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			libmaus::autoarray::AutoArray< element_type * > freelist;
			uint64_t freecnt;
			
			void cleanup()
			{
				for ( uint64_t i = 0; i < freelist.size(); ++i )
				{
					delete freelist[i];
					freelist[i] = 0;
				}	
			}
			
			FreeList(uint64_t const numel) : freelist(numel), freecnt(numel)
			{
				try
				{
					for ( uint64_t i = 0; i < numel; ++i )
						freelist[i] = 0;
					for ( uint64_t i = 0; i < numel; ++i )
						freelist[i] = new element_type;
				}
				catch(...)
				{
					cleanup();
					throw;
				}
			}
			
			virtual ~FreeList()
			{
				cleanup();
			}
			
			bool empty() const
			{
				return freecnt == 0;
			}
			
			element_type * get()
			{
				element_type * p = 0;
				assert ( ! empty() );

				p = freelist[--freecnt];
				freelist[freecnt] = 0;

				return p;
			}
			
			void put(element_type * ptr)
			{
				freelist[freecnt++] = ptr;
			}
		};
	}
}
#endif
