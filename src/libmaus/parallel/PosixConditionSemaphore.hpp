/*
    libmaus
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#if ! defined(LIBMAUS_PARALLEL_POSIXCONDITIONSEMAPHORE_HPP)
#define LIBMAUS_PARALLEL_POSIXCONDITIONSEMAPHORE_HPP

#include <pthread.h>
#include <libmaus/exception/LibMausException.hpp>

namespace libmaus
{
	namespace parallel
	{
		struct PosixConditionSemaphore
		{
			pthread_cond_t cond;
			pthread_mutex_t mutex;
			
			int volatile sigcnt;
			
			PosixConditionSemaphore()
			: sigcnt(0) 
			{
				int r;
				if ( (r=pthread_cond_init(&cond,NULL)) != 0 )
				{
					int const error = r;
					libmaus::exception::LibMausException lme;
					lme.getStream() << "PosixConditionSemaphore: failed pthread_cond_init " << strerror(error) << std::endl;
					lme.finish();
					throw lme;				
				}
				if ( (r=pthread_mutex_init(&mutex,NULL)) != 0 )
				{
					pthread_cond_destroy(&cond);
				
					int const error = r;
					libmaus::exception::LibMausException lme;
					lme.getStream() << "PosixConditionSemaphore: failed pthread_mutex_init " << strerror(error) << std::endl;
					lme.finish();
					throw lme;
				}
			}
			~PosixConditionSemaphore()
			{
				pthread_cond_destroy(&cond);
				pthread_mutex_destroy(&mutex);
			}
			
			struct ScopeMutexLock
			{
				pthread_mutex_t * mutex;
			
				ScopeMutexLock(pthread_mutex_t * rmutex) : mutex(rmutex)
				{
					if ( pthread_mutex_lock(mutex) != 0 )
					{
						int const error = errno;
						libmaus::exception::LibMausException lme;
						lme.getStream() << "PosixConditionSemaphore::ScopeMutexLock failed pthread_mutex_lock " << strerror(error) << std::endl;
						lme.finish();
						throw lme;
					}					
				}
				
				~ScopeMutexLock()
				{
					if ( pthread_mutex_unlock(mutex) != 0 )
					{
						int const error = errno;
						libmaus::exception::LibMausException lme;
						lme.getStream() << "PosixConditionSemaphore::ScopeMutexLock failed pthread_mutex_unlock " << strerror(error) << std::endl;
						lme.finish();
						throw lme;
					}		
				}
			};

			bool trywait()
			{
				ScopeMutexLock slock(&mutex);
			
				bool r = false;
								
				if ( sigcnt )
				{
					sigcnt -= 1;
					r = true;
				}
								
				return r;	
			}
			
			void wait()
			{
				ScopeMutexLock slock(&mutex);

				bool done = false;
				while ( !done )
				{
					if ( sigcnt )
					{
						sigcnt -= 1;
						done    = true;
					}
					else
					{
						pthread_cond_wait(&cond,&mutex);
					}
				}
			}
			
			void post()
			{
				ScopeMutexLock slock(&mutex);

				sigcnt += 1;
				
				pthread_cond_broadcast(&cond);
			}
			
			bool timedWait()
			{
				ScopeMutexLock slock(&mutex);

				// time structures
				struct timeval tv;
				struct timezone tz;
				struct timespec waittime;

				// get time of day
				gettimeofday(&tv,&tz);
                                
				// set wait time to 1 second
				waittime.tv_sec = tv.tv_sec + 1;
				waittime.tv_nsec = static_cast<uint64_t>(tv.tv_usec)*1000;

				if ( ! sigcnt )
					pthread_cond_timedwait(&cond,&mutex,&waittime);

				bool r = false;
				if ( sigcnt )
				{
					sigcnt -= 1;
					r = true;
				}
								
				return r;
			}
		};
	}
}
#endif
