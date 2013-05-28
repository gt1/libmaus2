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

#if ! defined(SYNCHRONOUSQUEUE_HPP)
#define SYNCHRONOUSQUEUE_HPP

#include <libmaus/LibMausConfig.hpp>

#if defined(LIBMAUS_HAVE_PTHREADS)
#include <libmaus/parallel/PosixMutex.hpp>
#include <libmaus/parallel/PosixSemaphore.hpp>
#include <deque>

namespace libmaus
{
        namespace parallel
        {
                template<typename _value_type>
                struct SynchronousQueue
                {
                	typedef _value_type value_type;
                	typedef SynchronousQueue<value_type> this_type;
                	
                        std::deque < value_type > Q;
                        PosixMutex lock;
                        PosixSemaphore semaphore;
                        
                        this_type * parent;
                        
                        SynchronousQueue() : parent(0)
                        {

                        }
                        
                        virtual ~SynchronousQueue()
                        {
                        
                        }
                        
                        unsigned int getFillState()
                        {
                                lock.lock();
                                unsigned int const fill = Q.size();
                                lock.unlock();
                                return fill;
                        }
                        
                        void enque(value_type const q)
                        {
                                lock.lock();
                                Q.push_back(q);
                                lock.unlock();
                                semaphore.post();
                                
                                if ( parent )
	                                parent->enque(q);
                        }
                        virtual value_type deque()
                        {
                                semaphore.wait();
                                lock.lock();
                                value_type const v = Q.front();
                                Q.pop_front();
                                lock.unlock();
                                return v;
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
					::libmaus::exception::LibMausException se;
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
				return v;
                        }
                };
        }
}
#endif
#endif
