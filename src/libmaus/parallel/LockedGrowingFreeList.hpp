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
#if ! defined(LIBMAUS_PARALLEL_LOCKEDGROWINGFREELIST_HPP)
#define LIBMAUS_PARALLEL_LOCKEDGROWINGFREELIST_HPP

#include <libmaus/util/GrowingFreeList.hpp>

namespace libmaus
{
	namespace parallel
	{
		template<
			typename _element_type, 
			typename _allocator_type = libmaus::util::FreeListDefaultAllocator<_element_type>,
			typename _type_info_type = libmaus::util::FreeListDefaultTypeInfo<_element_type> 
		>
		struct LockedGrowingFreeList : private libmaus::util::GrowingFreeList<_element_type,_allocator_type,_type_info_type>
		{
			typedef _element_type element_type;
			typedef _allocator_type allocator_type;
			typedef _type_info_type type_info_type;
			typedef libmaus::util::GrowingFreeList<element_type,allocator_type,type_info_type> base_type;
			typedef LockedGrowingFreeList<element_type,allocator_type,type_info_type> this_type;
			typedef typename libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

			libmaus::parallel::PosixSpinLock lock;

			LockedGrowingFreeList(allocator_type allocator = allocator_type())
			: base_type(allocator)
			{
			}
			
			virtual ~LockedGrowingFreeList()
			{
			}
			
			bool empty() const
			{
				libmaus::parallel::ScopePosixSpinLock slock(lock);
				return base_type::empty();
			}
			
			typename type_info_type::pointer_type get()
			{
				libmaus::parallel::ScopePosixSpinLock slock(lock);
				return base_type::get();
			}

			void put(typename type_info_type::pointer_type ptr)
			{
				libmaus::parallel::ScopePosixSpinLock slock(lock);
				base_type::put(ptr);
			}

			void put(std::vector < typename type_info_type::pointer_type > V)
			{
				libmaus::parallel::ScopePosixSpinLock slock(lock);
				base_type::put(V);				
			}

			std::vector < typename type_info_type::pointer_type > getAll()
			{
				libmaus::parallel::ScopePosixSpinLock slock(lock);
				return base_type::getAll();			
			}
		};
	}
}
#endif
