/*
    libmaus2
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
#if ! defined(LIBMAUS2_PARALLEL_POSIXCONDITIONSEMAPHORE_HPP)
#define LIBMAUS2_PARALLEL_POSIXCONDITIONSEMAPHORE_HPP

#include <cstring>
#include <libmaus2/exception/LibMausException.hpp>
#include <libmaus2/parallel/SimpleSemaphoreInterface.hpp>

#if defined(LIBMAUS2_HAVE_PTHREADS)
#include <pthread.h>

namespace libmaus2
{
	namespace parallel
	{
		struct PosixConditionSemaphore : public SimpleSemaphoreInterface
		{
			pthread_cond_t cond;
			pthread_mutex_t mutex;			
			int volatile sigcnt;
			
			PosixConditionSemaphore() : cond(PTHREAD_COND_INITIALIZER), mutex(PTHREAD_MUTEX_INITIALIZER), sigcnt(0)
			{
			}
			~PosixConditionSemaphore()
			{
			}
			
			struct ScopeMutexLock
			{
				pthread_mutex_t * mutex;
			
				ScopeMutexLock(pthread_mutex_t & rmutex) : mutex(&rmutex)
				{
					if ( pthread_mutex_lock(mutex) != 0 )
					{
						int const error = errno;
						libmaus2::exception::LibMausException lme;
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
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "PosixConditionSemaphore::ScopeMutexLock failed pthread_mutex_unlock " << strerror(error) << std::endl;
						lme.finish();
						throw lme;
					}		
				}
			};

			bool trywait()
			{
				ScopeMutexLock slock(mutex);
			
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
				ScopeMutexLock slock(mutex);

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
				ScopeMutexLock slock(mutex);

				sigcnt += 1;
				
				pthread_cond_signal(&cond);
			}
		};
	}
}
#endif // PTHREADS
#endif
