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

#if ! defined(TERMINATABLESYNCHRONOUSHEAP_HPP)
#define TERMINATABLESYNCHRONOUSHEAP_HPP

#include <libmaus2/parallel/SynchronousHeap.hpp>

#if defined(LIBMAUS2_HAVE_PTHREADS)
namespace libmaus2
{
        namespace parallel
        {
                #if defined(__APPLE__)
                /**
                 * posix condition variable based version for Darwin, which has limited support for posix semaphores (no timed waits)
                 **/
                template<typename _value_type, typename _compare = ::std::less<_value_type> >
                struct TerminatableSynchronousHeap
                {
                        typedef _value_type value_type;
                        typedef _compare compare;
                        typedef TerminatableSynchronousHeap<value_type,compare> this_type;
                                                
                        pthread_mutex_t mutex;
                        pthread_cond_t cond;
                        size_t volatile numwait;
                        std::priority_queue<value_type, std::vector<value_type>, compare > Q;
                        volatile uint64_t terminated;
                        uint64_t const terminatedthreshold;
                                             
                        struct MutexLock
                        {
                                pthread_mutex_t * mutex;
                                bool locked;
                                
                                void obtain()
                                {
                                        if ( ! locked )
                                        {
                                                int const r = pthread_mutex_lock(mutex);
                                                if ( r != 0 )
                                                {
                                                        int const error = errno;
                                                        libmaus2::exception::LibMausException lme;
                                                        lme.getStream() << "MutexLock: " << strerror(error) << std::endl;
                                                        lme.finish();
                                                        throw lme;
                                                }
                                                locked = true;
                                        }
                                }
                                
                                MutexLock(pthread_mutex_t & rmutex) : mutex(&rmutex), locked(false)
                                {
                                        obtain();
                                }
                                
                                void release()
                                {
                                        if ( locked )
                                        {
                                                int const r = pthread_mutex_unlock(mutex);
                                                if ( r != 0 )
                                                {
                                                        int const error = errno;
                                                        libmaus2::exception::LibMausException lme;
                                                        lme.getStream() << "~MutexLock: " << strerror(error) << std::endl;
                                                        lme.finish();
                                                        throw lme;
                                                }

                                                locked = false;
                                        }                                
                                }
                                
                                ~MutexLock()
                                {
                                        release();
                                }
                        };
                                                
                        TerminatableSynchronousHeap(uint64_t const rterminatedthreshold = 1)
                        : mutex(PTHREAD_MUTEX_INITIALIZER), cond(PTHREAD_COND_INITIALIZER), numwait(0), Q(), terminated(0), terminatedthreshold(rterminatedthreshold)
                        {
                        
                        }

                        TerminatableSynchronousHeap(compare const & comp, uint64_t const rterminatedthreshold = 1)
                        : mutex(PTHREAD_MUTEX_INITIALIZER), cond(PTHREAD_COND_INITIALIZER), numwait(0), Q(comp), terminated(0), terminatedthreshold(rterminatedthreshold)
                        {
                        
                        }
                        
                        ~TerminatableSynchronousHeap()
                        {
                        
                        }

                        void enque(value_type const v)
                        {
                                {
                                        MutexLock M(mutex);
                                        Q.push(v);
                                }
                                
                                int const r = pthread_cond_signal(&cond);
                                                                
                                if ( r )
                                {
                                        int const error = errno;
                                        libmaus2::exception::LibMausException lme;
                                        lme.getStream() << "enque: " << strerror(error) << std::endl;
                                        lme.finish();
                                        throw lme;
                                }
                        }

                        size_t getFillState()
                        {
                                uint64_t f;
                                
                                {
                                        MutexLock M(mutex);
                                        f = Q.size();
                                }
                                
                                return f;
                        }
                        
                        bool isTerminated()
                        {
                                bool lterminated;
                                {
                                MutexLock M(mutex);
                                lterminated = terminated >= terminatedthreshold;
                                }
                                return lterminated;
                        }
                        
                        void terminate()
                        {
                                size_t numnoti = 0;
                                {
                                        MutexLock M(mutex);
                                        terminated += 1;

					if ( terminated >= terminatedthreshold )
                                 	       numnoti = numwait;
                                }
                                for ( size_t i = 0; i < numnoti; ++i )
                                        pthread_cond_signal(&cond);
                        }
                        
                        value_type deque()
                        {
                                MutexLock M(mutex);
                                
                                while ( (! terminated) && (!Q.size()) )
                                {
                                        numwait++;
                                        int const r = pthread_cond_wait(&cond,&mutex);
                                        
                                        if ( r )
                                        {
                                                int const error = errno;
                                                libmaus2::exception::LibMausException lme;
                                                lme.getStream() << "~MutexLock: " << strerror(error) << std::endl;
                                                lme.finish();
                                                throw lme;
                                        }
                                        
                                        numwait--;
                                }
                                
                                if ( Q.size() )
                                {
                                        value_type v = Q.top();
                                        Q.pop();
                                        return v;
                                }
                                else
                                {
                                        throw std::runtime_error("Heap is terminated");                                
                                }
                        }

                        std::vector<value_type> pending()
                        {
	                        std::priority_queue < value_type, std::vector<value_type>, compare > C;
	                        {
	                        	MutexLock M(mutex);
	                        	C = Q;
				}
	                        std::vector<value_type> V;
	                        while ( ! C.empty() )
	                        {
	                        	V.push_back(C.top());
	                        	C.pop();
				}
				
				return V;
                        }
                };
                #else
                template<typename value_type, typename compare = ::std::less<value_type> >
                struct TerminatableSynchronousHeap : public SynchronousHeap<value_type,compare>
                {
                        typedef SynchronousHeap<value_type,compare> parent_type;

                        PosixSpinLock terminatelock;
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
                #endif
        }
}
#endif
#endif
