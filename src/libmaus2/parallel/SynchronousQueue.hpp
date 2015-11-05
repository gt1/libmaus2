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

#if ! defined(SYNCHRONOUSQUEUE_HPP)
#define SYNCHRONOUSQUEUE_HPP

#include <libmaus2/LibMausConfig.hpp>

#if defined(LIBMAUS2_HAVE_PTHREADS)
#include <libmaus2/parallel/PosixMutex.hpp>
#include <libmaus2/parallel/PosixSpinLock.hpp>
#include <libmaus2/parallel/PosixConditionSemaphore.hpp>
#include <libmaus2/parallel/PosixSemaphore.hpp>
#include <deque>

namespace libmaus2
{
        namespace parallel
        {
                template<typename _value_type>
                struct SynchronousQueue
                {
                	typedef _value_type value_type;
                	typedef SynchronousQueue<value_type> this_type;
                	typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

                        std::deque < value_type > Q;
                        PosixSpinLock lock;
                        libmaus2::parallel::SimpleSemaphoreInterface::unique_ptr_type Psemaphore;
                        libmaus2::parallel::SimpleSemaphoreInterface & semaphore;

                        this_type * parent;

                        SynchronousQueue() : Q(), lock(), Psemaphore(new libmaus2::parallel::PosixSemaphore), semaphore(*Psemaphore), parent(0)
                        {

                        }

                        virtual ~SynchronousQueue()
                        {

                        }

                        bool empty()
                        {
                        	return getFillState() == 0;
                        }

			size_t size()
                        {
                        	return getFillState();
                        }

			size_t getFillState()
                        {
                        	libmaus2::parallel::ScopePosixSpinLock llock(lock);
                                size_t const fill = Q.size();
                                return fill;
                        }

                        void enque(value_type const q)
                        {
                        	{
	                        	libmaus2::parallel::ScopePosixSpinLock llock(lock);
        	                        Q.push_back(q);
				}

                                semaphore.post();

                                if ( parent )
	                                parent->enque(q);
                        }
                        virtual value_type deque()
                        {
                                semaphore.wait();

                        	libmaus2::parallel::ScopePosixSpinLock llock(lock);
                                value_type const v = Q.front();
                                Q.pop_front();
                                return v;
                        }
                        virtual bool trydeque(value_type & v)
                        {
                        	bool const ok = semaphore.trywait();

                                if ( ok )
                                {
	                        	libmaus2::parallel::ScopePosixSpinLock llock(lock);
	                        	v = Q.front();
                	                Q.pop_front();
                	                return true;
				}
				else
				{
					return false;
				}
                        }
                        value_type peek()
                        {
                        	lock.lock();

                        	if ( Q.size() )
                        	{
	                        	value_type const v = Q.front();
	                        	lock.unlock();
	                        	return v;
				}
				else
				{
					lock.unlock();
					::libmaus2::exception::LibMausException se;
					se.getStream() << "SynchronousQueue::peek() called on empty queue." << std::endl;
					se.finish();
					throw se;
				}
                        }

                        bool peek(value_type & v)
                        {
                        	lock.lock();
                        	bool ok = Q.size() != 0;
                        	if ( ok )
                        		v = Q.front();
				lock.unlock();
				return ok;
                        }
                };
        }
}
#endif
#endif
