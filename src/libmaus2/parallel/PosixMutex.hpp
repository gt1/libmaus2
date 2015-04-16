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
#if ! defined(POSIXMUTEX_HPP)
#define POSIXMUTEX_HPP

#include <libmaus2/LibMausConfig.hpp>
#include <libmaus2/exception/LibMausException.hpp>
#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/util/shared_ptr.hpp>
#include <cerrno>

#if defined(LIBMAUS2_HAVE_PTHREADS)
#include <pthread.h>

namespace libmaus2
{
	namespace parallel
	{
                struct PosixMutex
                {
                	typedef PosixMutex this_type;
                	typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
                	typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
                
                        pthread_mutex_t mutex;
                        
                        PosixMutex() : mutex()
                        {
                                pthread_mutex_init(&mutex,0);
                        }
                        ~PosixMutex()
                        {
                                pthread_mutex_destroy(&mutex);
                        }
                        
                        void lock()
                        {
                        	if ( pthread_mutex_lock ( &mutex ) )
                        	{
                        		::libmaus2::exception::LibMausException se;
                        		se.getStream() << "pthread_mutex_lock failed" << std::endl;
                        		se.finish();
                        		throw se;
                        	}
                        }
                        void unlock()
                        {
                        	if ( pthread_mutex_unlock ( &mutex ) )
                        	{
                        		::libmaus2::exception::LibMausException se;
                        		se.getStream() << "pthread_mutex_unlock failed" << std::endl;
                        		se.finish();
                        		throw se;                        	
                        	}
                        }
                        /**
                         * try to lock mutex. returns true if locking was succesful, false if mutex
                         * was already locked
                         **/
                        bool trylock()
                        {
                        	int const r = pthread_mutex_trylock(&mutex);
                        	if ( r == EBUSY )
                        	{
                        		return false;
				}
				else if ( r )
				{
                        		::libmaus2::exception::LibMausException se;
                        		se.getStream() << "pthread_mutex_trylock failed" << std::endl;
                        		se.finish();
                        		throw se;                        						
				}
				else
				{
					return true;
				}
                        }
                        
                        /*
                         * try to lock mutex. if succesful, mutex is unlocked and return value is true,
                         * otherwise return value is false
                         */
                        bool tryLockUnlock()
                        {
                        	bool const r = trylock();
                        	if ( r )
                        		unlock();
				return r;
                        }
                };
                
                struct ScopePosixMutex
                {
                	PosixMutex & mutex;
                	
                	ScopePosixMutex(PosixMutex & rmutex)
                	: mutex(rmutex)
                	{
                		mutex.lock();
                	}
                	~ScopePosixMutex()
                	{
                		mutex.unlock();
                	}
                };
	}
}
#endif
#endif
