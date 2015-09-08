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

#if ! defined(TERMINATABLESYNCHRONOUSREORDERSET_HPP)
#define TERMINATABLESYNCHRONOUSREORDERSET_HPP

#include <libmaus2/parallel/SynchronousReorderSet.hpp>

#if defined(LIBMAUS2_HAVE_PTHREADS)
namespace libmaus2
{
        namespace parallel
        {
                template<typename value_type>
                struct TerminatableSynchronousReorderSet
                {
                	typedef std::pair< uint64_t, value_type > pair_type;
                	typedef typename ::std::set<pair_type>::iterator iterator_type;

                        pthread_mutex_t mutex;
                        pthread_cond_t cond;
                        size_t volatile numwait;
                        bool volatile terminated;
                	uint64_t next;
                        std::set < pair_type > Q;

			struct SynchronousReorderSetPairComp
			{
				SynchronousReorderSetPairComp() {}
				
				bool operator()(pair_type const & A, pair_type const & B)
				{
					if ( A.first != B.first )
						return A.first < B.first;
					else
						return A.second < B.second;
				}
			};

                        
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

                        TerminatableSynchronousReorderSet(uint64_t const rnext = 0)
                        : mutex(), cond(), numwait(0), terminated(false), next(rnext), Q()
                        {
				initCond();
				try
				{
					initMutex();
				}
				catch(...)
				{
					pthread_cond_destroy(&cond);
					throw;
				}                        
                        }
                        
                        ~TerminatableSynchronousReorderSet()
                        {
                        	pthread_mutex_destroy(&mutex);
                        	pthread_cond_destroy(&cond);
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

                        void enque(uint64_t const idx, value_type const q)
                        {
                        	uint64_t postcnt = 0;

                                MutexLock M(mutex);
                        	
                                Q.insert(pair_type(idx,q));
                                
                                for ( iterator_type I = Q.begin(); I != Q.end() && I->first == next; ++I )
                               		next++, postcnt++;

                                M.release();

                                for ( uint64_t i = 0; i < postcnt; ++i )
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

	                                iterator_type const I = Q.begin();
        	                        std::pair<uint64_t,value_type> const P = *I;
                	                Q.erase(I);
                	                return P.second;
                                }
                                else
                                {
                                        throw std::runtime_error("Queue is terminated");                                
                                }
                        }
                };
        }
}
#endif
#endif
