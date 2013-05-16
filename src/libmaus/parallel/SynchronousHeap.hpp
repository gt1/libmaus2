/**
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
**/

#if ! defined(SYNCHRONOUSHEAP_HPP)
#define SYNCHRONOUSHEAP_HPP

#include <libmaus/LibMausConfig.hpp>

#if defined(LIBMAUS_HAVE_PTHREADS)
#include <libmaus/parallel/PosixMutex.hpp>
#include <libmaus/parallel/PosixSemaphore.hpp>
#include <deque>
#include <queue>

namespace libmaus
{
        namespace parallel
        {
                template<typename value_type, typename compare = ::std::less<value_type> >
                struct SynchronousHeap
                {
                        std::priority_queue < value_type, std::vector<value_type>, compare > Q;
                        PosixMutex lock;
                        PosixSemaphore semaphore;
                        
                        SynchronousHeap()
                        {
                        
                        }
                        
                        SynchronousHeap(compare const & comp)
                        : Q(comp)
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
                                Q.push(q);
                                lock.unlock();
                                semaphore.post();
                        }
                        value_type deque()
                        {
                                semaphore.wait();
                                lock.lock();
                                value_type const v = Q.top();
                                Q.pop();
                                lock.unlock();
                                return v;
                        }
                };                
        }
}
#endif
#endif
