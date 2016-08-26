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

#if ! defined(TERMINATABLESYNCHRONOUSHEAP_HPP)
#define TERMINATABLESYNCHRONOUSHEAP_HPP

#include <libmaus2/parallel/SynchronousHeap.hpp>
#include <libmaus2/parallel/PosixSemaphore.hpp>

#if defined(LIBMAUS2_HAVE_PTHREADS)
namespace libmaus2
{
        namespace parallel
        {
        	#if defined(__APPLE__)
                /**
                 * posix condition variable based version
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

			void initCond()
			{
				if ( pthread_cond_init(&cond,NULL) != 0 )
				{
					int const error = errno;
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "PosixConditionSemaphore::initCond(): failed pthread_cond_init " << strerror(error) << std::endl;
					lme.finish();
					throw lme;
				}
			}

			void initMutex()
			{
				if ( pthread_mutex_init(&mutex,NULL) != 0 )
				{
					int const error = errno;
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "PosixConditionSemaphore::initMutex(): failed pthread_mutex_init " << strerror(error) << std::endl;
					lme.finish();
					throw lme;
				}
			}

                        TerminatableSynchronousHeap(uint64_t const rterminatedthreshold = 1)
                        : numwait(0), Q(), terminated(0), terminatedthreshold(rterminatedthreshold)
			{
				initMutex();
				try
				{
					initCond();
				}
				catch(...)
				{
					pthread_mutex_destroy(&mutex);
					throw;
				}
			}

			TerminatableSynchronousHeap(compare const & comp, uint64_t const rterminatedthreshold = 1)
			: numwait(0), Q(comp), terminated(0), terminatedthreshold(rterminatedthreshold)
			{
				initMutex();
				try
				{
					initCond();
				}
				catch(...)
				{
					pthread_mutex_destroy(&mutex);
					throw;
				}
			}

                        ~TerminatableSynchronousHeap()
                        {
				pthread_mutex_destroy(&mutex);
				pthread_cond_destroy(&cond);
                        }

                        void enque(value_type const v)
                        {
                                MutexLock M(mutex);
                                Q.push(v);
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
                        	MutexLock M(mutex);
                                terminated += 1;

	                        size_t numnoti = 0;
				if ( terminated >= terminatedthreshold )
                                	numnoti = numwait;

				for ( size_t i = 0; i < numnoti; ++i )
        	                	pthread_cond_signal(&cond);
                        }

                        value_type deque()
                        {
                                MutexLock M(mutex);

                                while ( (terminated < terminatedthreshold) && (!Q.size()) )
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
                /**
                 * semaphore variable based version
                 **/
                template<typename _value_type, typename _compare = ::std::less<_value_type> >
                struct TerminatableSynchronousHeap
                {
                        typedef _value_type value_type;
                        typedef _compare compare;
                        typedef TerminatableSynchronousHeap<value_type,compare> this_type;

                        libmaus2::parallel::SimpleSemaphoreInterface::unique_ptr_type Psemaphore;
                        libmaus2::parallel::SimpleSemaphoreInterface & semaphore;
                        libmaus2::parallel::PosixSpinLock lock;

                        size_t volatile numwait;
                        std::priority_queue<value_type, std::vector<value_type>, compare > Q;
                        volatile uint64_t terminated;
                        uint64_t const terminatedthreshold;

                        TerminatableSynchronousHeap(uint64_t const rterminatedthreshold = 1)
                        : Psemaphore(new libmaus2::parallel::PosixSemaphore),
                          semaphore(*Psemaphore),
                          lock(),
                          numwait(0), Q(), terminated(0), terminatedthreshold(rterminatedthreshold)
                        {

                        }

                        TerminatableSynchronousHeap(compare const & comp, uint64_t const rterminatedthreshold = 1)
                        : Psemaphore(new libmaus2::parallel::PosixSemaphore),
                          semaphore(*Psemaphore),
                          lock(),
                          numwait(0), Q(comp), terminated(0), terminatedthreshold(rterminatedthreshold)
                        {

                        }

                        ~TerminatableSynchronousHeap()
                        {

                        }

                        void enque(value_type const v)
                        {
                        	ScopePosixSpinLock M(lock);
                                Q.push(v);
                                semaphore.post();
                        }

                        size_t getFillState()
                        {
                        	ScopePosixSpinLock M(lock);
                                uint64_t f = Q.size();
                                return f;
                        }

                        bool isTerminated()
                        {
                        	ScopePosixSpinLock M(lock);
                                return terminated >= terminatedthreshold;
                        }

                        void terminate()
                        {
                        	ScopePosixSpinLock M(lock);
                                terminated += 1;

	                        size_t numnoti = 0;
				if ( terminated >= terminatedthreshold )
                                	numnoti = numwait;

				for ( size_t i = 0; i < numnoti; ++i )
        	                	semaphore.post();
                        }

                        value_type deque()
                        {
                        	ScopePosixSpinLock M(lock);

                                while ( (terminated < terminatedthreshold) && (!Q.size()) )
                                {
                                        numwait++;

                                        M.unlock();
                                        semaphore.wait();
                                        M.lock();

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
	                        	ScopePosixSpinLock M(lock);
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
                #endif
        }
}
#endif
#endif
