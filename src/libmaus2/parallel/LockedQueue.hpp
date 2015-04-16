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
#if !defined(LIBMAUS2_PARALLEL_LOCKEDQUEUE_HPP)
#define LIBMAUS2_PARALLEL_LOCKEDQUEUE_HPP

#include <libmaus2/parallel/PosixSpinLock.hpp>
#include <deque>

namespace libmaus2
{
	namespace parallel
	{	
		template<typename _value_type>
		struct LockedQueue
		{
			typedef _value_type value_type;
			typedef LockedQueue<value_type> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			libmaus2::parallel::PosixSpinLock lock;
			std::deque<value_type> Q;
			
			LockedQueue()
			: lock(), Q()
			{
			
			}
			
			libmaus2::parallel::PosixSpinLock & getLock()
			{
				return lock;
			}
			
			bool emptyUnlocked()
			{
				return Q.size() == 0;
			}
			
			uint64_t sizeUnlocked()
			{
				return Q.size();
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
			
			void push_back(value_type const v)
			{
				libmaus2::parallel::ScopePosixSpinLock llock(lock);
				Q.push_back(v);
			}

			void push_backUnlocked(value_type const v)
			{
				Q.push_back(v);
			}

			uint64_t push_back_and_size(value_type const v)
			{
				libmaus2::parallel::ScopePosixSpinLock llock(lock);
				Q.push_back(v);
				return Q.size();
			}

			void push_front(value_type const v)
			{
				libmaus2::parallel::ScopePosixSpinLock llock(lock);
				Q.push_front(v);
			}

			void pop_back()
			{
				libmaus2::parallel::ScopePosixSpinLock llock(lock);
				Q.pop_back();
			}

			void pop_front()
			{
				libmaus2::parallel::ScopePosixSpinLock llock(lock);
				Q.pop_front();
			}

			value_type front()
			{
				libmaus2::parallel::ScopePosixSpinLock llock(lock);
				return Q.front();
			}

			value_type back()
			{
				libmaus2::parallel::ScopePosixSpinLock llock(lock);
				return Q.back();
			}

			value_type frontUnlocked()
			{
				return Q.front();
			}

			value_type backUnlocked()
			{
				return Q.back();
			}

			void pop_backUnlocked()
			{
				Q.pop_back();
			}

			void pop_frontUnlocked()
			{
				Q.pop_front();
			}

			value_type dequeFront()
			{
				libmaus2::parallel::ScopePosixSpinLock llock(lock);
				value_type const v = Q.front();
				Q.pop_front();
				return v;
			}

			value_type dequeFrontUnlocked()
			{
				value_type const v = Q.front();
				Q.pop_front();
				return v;
			}
			
			bool tryDequeFront(value_type & v)
			{
				libmaus2::parallel::ScopePosixSpinLock llock(lock);
				if ( Q.size() )
				{
					v = Q.front();
					Q.pop_front();
					return true;
				}		
				else
				{
					return false;
				}	
			}
			
			uint64_t tryDequeFront(std::vector<value_type> & V, uint64_t max)
			{
				libmaus2::parallel::ScopePosixSpinLock llock(lock);
				while ( max-- && Q.size() )
				{
					V.push_back(Q.front());
					Q.pop_front();
				}
				return V.size();			
			}

			value_type dequeBack()
			{
				libmaus2::parallel::ScopePosixSpinLock llock(lock);
				value_type const v = Q.back();
				Q.pop_back();
				return v;
			}
		};
	}
}
#endif
