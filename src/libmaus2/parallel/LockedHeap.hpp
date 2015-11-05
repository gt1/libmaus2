/*
    libmaus2
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
#if !defined(LIBMAUS2_PARALLEL_LOCKEDHEAP_HPP)
#define LIBMAUS2_PARALLEL_LOCKEDHEAP_HPP

#include <libmaus2/parallel/PosixSpinLock.hpp>
#include <queue>

namespace libmaus2
{
	namespace parallel
	{
		template<typename _value_type, typename _comparator_type = std::greater<_value_type> >
		struct LockedHeap
		{
			typedef _value_type value_type;
			typedef _comparator_type comparator_type;
			typedef LockedHeap<value_type,comparator_type> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			libmaus2::parallel::PosixSpinLock lock;
			std::priority_queue<value_type,std::vector<value_type>,comparator_type> Q;

			LockedHeap()
			: lock(), Q()
			{

			}

			uint64_t size()
			{
				libmaus2::parallel::ScopePosixSpinLock llock(lock);
				return Q.size();
			}

			bool empty()
			{
				libmaus2::parallel::ScopePosixSpinLock llock(lock);
				return Q.size() == 0;
			}

			void push(value_type const v)
			{
				libmaus2::parallel::ScopePosixSpinLock llock(lock);
				Q.push(v);
			}

			value_type pop()
			{
				libmaus2::parallel::ScopePosixSpinLock llock(lock);
				value_type p = Q.top();
				Q.pop();
				return p;
			}

			value_type top()
			{
				libmaus2::parallel::ScopePosixSpinLock llock(lock);
				return Q.top();
			}

			bool tryPop(value_type & v)
			{
				libmaus2::parallel::ScopePosixSpinLock llock(lock);
				if ( Q.size() )
				{
					v = Q.top();
					Q.pop();
					return true;
				}
				else
				{
					return false;
				}
			}
		};
	}
}
#endif
