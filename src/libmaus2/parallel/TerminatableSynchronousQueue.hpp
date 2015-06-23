/*
    libmaus2
    Copyright (C) 2009-2015 German Tischler
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

#if ! defined(TERMINATABLESYNCHRONOUSQUEUE_HPP)
#define TERMINATABLESYNCHRONOUSQUEUE_HPP

#include <libmaus2/parallel/SynchronousQueue.hpp>

#if defined(LIBMAUS2_HAVE_PTHREADS)
namespace libmaus2
{
        namespace parallel
        {
                #if defined(__APPLE__)
                /**
                 * posix condition variable based version for Darwin, which has limited support for posix semaphores (no timed waits)
                 **/
                template<typename _value_type>
                struct TerminatableSynchronousQueue
                {
                        typedef _value_type value_type;
                        typedef TerminatableSynchronousQueue<value_type> this_type;
                                                
                        pthread_mutex_t mutex;
                        pthread_cond_t cond;
                        size_t volatile numwait;
                        std::deque<value_type> Q;
                        bool volatile terminated;
                        
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
                                                
                        TerminatableSynchronousQueue()
                        : mutex(PTHREAD_MUTEX_INITIALIZER), cond(PTHREAD_COND_INITIALIZER), numwait(0), Q(), terminated(false)
                        {
                        
                        }
                        
                        ~TerminatableSynchronousQueue()
                        {
                        
                        }

                        void enque(value_type const v)
                        {
                                MutexLock M(mutex);
                                Q.push_back(v);
                                pthread_cond_signal(&cond);
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
                                lterminated = terminated;
                                }
                                return lterminated;
                        }
                        
                        void terminate()
                        {
                                size_t numnoti = 0;
                                {
                                        MutexLock M(mutex);
                                        terminated = true;
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
                                        if ( r != 0 )
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
                                        value_type v = Q.front();
                                        Q.pop_front();
                                        return v;
                                }
                                else
                                {
                                        throw std::runtime_error("Queue is terminated");                                
                                }
                        }
                };
                #else
                template<typename value_type>
                struct TerminatableSynchronousQueue : public SynchronousQueue<value_type>
                {
                        typedef SynchronousQueue<value_type> parent_type;

                        PosixMutex terminatelock;
                        volatile bool terminated;
                        
                        TerminatableSynchronousQueue()
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
                                                throw std::runtime_error("Queue is terminated");
                                }
                                
                                parent_type::lock.lock();
                                value_type const v = parent_type::Q.front();
                                parent_type::Q.pop_front();
                                parent_type::lock.unlock();
                                return v;
                        }
                };
                #endif
        }
}
#endif
#endif
