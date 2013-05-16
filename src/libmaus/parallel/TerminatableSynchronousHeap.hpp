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

#if ! defined(TERMINATABLESYNCHRONOUSHEAP_HPP)
#define TERMINATABLESYNCHRONOUSHEAP_HPP

#include "SynchronousHeap.hpp"

#if defined(LIBMAUS_HAVE_PTHREADS)
namespace libmaus
{
        namespace parallel
        {
                template<typename value_type, typename compare = ::std::less<value_type> >
                struct TerminatableSynchronousHeap : public SynchronousHeap<value_type,compare>
                {
                        typedef SynchronousHeap<value_type,compare> parent_type;

                        PosixMutex terminatelock;
                        volatile bool terminated;
                        
                        TerminatableSynchronousHeap()
                        {
                                terminated = false;
                        }

                        TerminatableSynchronousHeap(compare const & comp)
                        : parent_type(comp)
                        {
                                terminated = false;
                        }
                        
                        void terminate()
                        {
                                terminatelock.lock();
                                terminated = true;
                                terminatelock.unlock();
                        }
                        bool isTerminated()
                        {
                                terminatelock.lock();
                                bool const isTerm = terminated;
                                terminatelock.unlock();
                                return isTerm;
                        }
                        
                        value_type deque()
                        {
                                while ( !parent_type::semaphore.timedWait() )
                                {
                                        terminatelock.lock();
                                        bool const isterminated = terminated;
                                        terminatelock.unlock();
                                        
                                        if ( isterminated )
                                                throw std::runtime_error("Heap is terminated");
                                }
                                
                                parent_type::lock.lock();
                                value_type const v = parent_type::Q.top();
                                parent_type::Q.pop();
                                parent_type::lock.unlock();
                                return v;
                        }
                };
        }
}
#endif
#endif
