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
#if ! defined(LIBMAUS_PARALLEL_LOCKEDFREELIST_HPP)
#define LIBMAUS_PARALLEL_LOCKEDFREELIST_HPP

#include <libmaus/util/FreeList.hpp>
#include <libmaus/parallel/PosixSpinLock.hpp>
#include <vector>

namespace libmaus
{
	namespace parallel
	{
		template<
			typename _element_type, 
			typename _allocator_type = libmaus::util::FreeListDefaultAllocator<_element_type>,
			typename _type_info_type = libmaus::util::FreeListDefaultTypeInfo<_element_type> 
		>
		struct LockedFreeList : private libmaus::util::FreeList<_element_type,_allocator_type,_type_info_type>
		{
			typedef _element_type element_type;
			typedef _allocator_type allocator_type;
			typedef _type_info_type type_info_type;
			typedef libmaus::util::FreeList<element_type,allocator_type,type_info_type> base_type;
			typedef LockedFreeList<element_type,allocator_type,type_info_type> this_type;
			typedef typename libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

			libmaus::parallel::PosixSpinLock lock;

			LockedFreeList(uint64_t const numel, allocator_type allocator = allocator_type()) : base_type(numel,allocator)
			{
			}
			
			virtual ~LockedFreeList()
			{
			}
			
			libmaus::parallel::PosixSpinLock & getLock()
			{
				return lock;
			}
			
			bool empty()
			{
				libmaus::parallel::ScopePosixSpinLock slock(lock);
				return base_type::empty();
			}
			
			bool emptyUnlocked()
			{
				return base_type::empty();
			}

			typename type_info_type::pointer_type getUnlocked()
			{
				return base_type::get();
			}

			bool full()
			{
				libmaus::parallel::ScopePosixSpinLock slock(lock);
				return base_type::full();
			}
			
			typename type_info_type::pointer_type get()
			{
				libmaus::parallel::ScopePosixSpinLock slock(lock);
				return base_type::get();
			}

			typename type_info_type::pointer_type getIf()
			{
				libmaus::parallel::ScopePosixSpinLock slock(lock);
				
				if ( base_type::empty() )
					return type_info_type::getNullPointer();
				else
					return base_type::get();
			}
			
			uint64_t get(std::vector<typename type_info_type::pointer_type> & V, uint64_t max)
			{
				libmaus::parallel::ScopePosixSpinLock slock(lock);
				while ( max-- && (!base_type::empty()) )
					V.push_back(base_type::get());
				return V.size();
			}
			
			void put(typename type_info_type::pointer_type ptr)
			{
				libmaus::parallel::ScopePosixSpinLock slock(lock);
				base_type::put(ptr);
			}

			uint64_t putAndCount(typename type_info_type::pointer_type ptr)
			{
				libmaus::parallel::ScopePosixSpinLock slock(lock);
				return base_type::putAndCount(ptr);
			}
			
			uint64_t capacity() const
			{
				return base_type::capacity();
			}
			
			uint64_t freeUnlocked() const
			{
				return base_type::free();
			}
			
			size_t byteSize()
			{
				return base_type::byteSize();
			}
		};
	}
}
#endif
