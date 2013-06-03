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
                        volatile uint64_t terminated;
                        uint64_t const terminatedthreshold;
                        
                        TerminatableSynchronousHeap(uint64_t const rterminatedthreshold = 1)
                        : terminated(0), terminatedthreshold(rterminatedthreshold)
                        {
                        }

                        TerminatableSynchronousHeap(compare const & comp, uint64_t const rterminatedthreshold = 1)
                        : parent_type(comp), terminated(0), terminatedthreshold(rterminatedthreshold)
                        {
                        }
                        
                        void terminate()
                        {
                                terminatelock.lock();
                                terminated++;
                                terminatelock.unlock();
                        }
                        bool isTerminated()
                        {
                                terminatelock.lock();
                                bool const isTerm = terminated >= terminatedthreshold;
                                terminatelock.unlock();
                                return isTerm;
                        }
                        
                        value_type deque()
                        {
                                while ( !parent_type::semaphore.timedWait() )
                                {
                                        terminatelock.lock();
                                        bool const isterminated = terminated >= terminatedthreshold;
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
