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
#if ! defined(LIBMAUS2_UTIL_FREELIST_HPP)
#define LIBMAUS2_UTIL_FREELIST_HPP

#include <libmaus2/autoarray/AutoArray.hpp>

namespace libmaus2
{
	namespace util
	{
		template<typename _element_type>
		struct FreeListDefaultAllocator
		{
			typedef _element_type element_type;
			
			FreeListDefaultAllocator()
			{
			}
			~FreeListDefaultAllocator()
			{
			}
			
			element_type * operator()() const
			{
				return new element_type;
			}
		};
		
		template<typename _element_type>
		struct FreeListDefaultTypeInfo
		{
			typedef _element_type element_type;
			typedef element_type * pointer_type;			
			
			static pointer_type deallocate(pointer_type p)
			{
				delete p;
				p = 0;
				return p;
			}
			
			static pointer_type getNullPointer()
			{
				pointer_type p = 0;
				return p;
			}
		};
	
		template<
			typename _element_type, 
			typename _allocator_type = FreeListDefaultAllocator<_element_type>,
			typename _type_info_type = FreeListDefaultTypeInfo<_element_type> 
		>
		struct FreeList
		{
			typedef _element_type element_type;
			typedef _allocator_type allocator_type;
			typedef _type_info_type type_info_type;
			typedef FreeList<element_type,allocator_type,type_info_type> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			libmaus2::autoarray::AutoArray< typename type_info_type::pointer_type > freelist;
			uint64_t freecnt;
			allocator_type allocator;
			
			void cleanup()
			{
				for ( uint64_t i = 0; i < freelist.size(); ++i )
				{
					freelist[i] = type_info_type::deallocate(freelist[i]);
				}	
			}
			
			FreeList(uint64_t const numel, allocator_type rallocator = allocator_type()) : freelist(numel), freecnt(numel), allocator(rallocator)
			{
				try
				{
					for ( uint64_t i = 0; i < numel; ++i )
						freelist[i] = type_info_type::getNullPointer();
					for ( uint64_t i = 0; i < numel; ++i )
						freelist[i] = allocator();
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
			
			bool full() const
			{
				return freecnt == freelist.size();
			}
			
			uint64_t free() const
			{
				return freecnt;
			}
			
			typename type_info_type::pointer_type get()
			{
				typename type_info_type::pointer_type p = typename type_info_type::pointer_type();
				assert ( ! empty() );

				p = freelist[--freecnt];
				freelist[freecnt] = type_info_type::getNullPointer();

				return p;
			}
			
			std::vector<typename type_info_type::pointer_type> getAll()
			{
				std::vector<typename type_info_type::pointer_type> V;
				while ( !empty() )
					V.push_back(get());
				return V;
			}
			
			void put(std::vector<typename type_info_type::pointer_type> V)
			{
				for ( typename std::vector<typename type_info_type::pointer_type>::size_type i = 0; i < V.size(); ++i )
					put(V[i]);
			}
			
			void put(typename type_info_type::pointer_type ptr)
			{
				freelist[freecnt++] = ptr;
			}

			uint64_t putAndCount(typename type_info_type::pointer_type ptr)
			{
				freelist[freecnt++] = ptr;
				return freecnt;
			}
			
			uint64_t capacity() const
			{
				return freelist.size();
			}

			size_t byteSize()
			{
				typedef typename type_info_type::pointer_type pointer_type;
				std::vector<pointer_type> V = getAll();
				size_t s = 0;
				for ( uint64_t i = 0; i < V.size(); ++i )
					s += V[i]->byteSize();
				put(V);
				
				s += freelist.byteSize();
				s += sizeof(freecnt);
				s += sizeof(allocator);
				
				return s;
			}
		};
	}
}
#endif
