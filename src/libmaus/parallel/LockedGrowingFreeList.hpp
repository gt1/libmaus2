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
		template<typename _element_type>
		struct LockedGrowingFreeList : private libmaus::util::GrowingFreeList<_element_type>
		{
			typedef _element_type element_type;
			typedef libmaus::util::GrowingFreeList<element_type> base_type;
			typedef LockedGrowingFreeList<element_type> this_type;
			typedef typename libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

			libmaus::parallel::PosixSpinLock lock;

			LockedGrowingFreeList()
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
			
			element_type * get()
			{
				libmaus::parallel::ScopePosixSpinLock slock(lock);
				return base_type::get();
			}

			void put(element_type * ptr)
			{
				libmaus::parallel::ScopePosixSpinLock slock(lock);
				base_type::put(ptr);
			}
		};
	}
}
#endif
