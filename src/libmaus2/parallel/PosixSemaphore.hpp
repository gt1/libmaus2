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

#if ! defined(LIBMAUS2_PARALLEL_POSIXSEMAPHORE_HPP)
#define LIBMAUS2_PARALLEL_POSIXSEMAPHORE_HPP

#include <libmaus2/exception/LibMausException.hpp>
#include <libmaus2/parallel/TimedSemaphoreInterface.hpp>

#include <ctime>
#include <cerrno>
#include <cstring>
#include <stdexcept>

#if defined(LIBMAUS2_HAVE_PTHREADS)
#include <semaphore.h>
#include <sys/time.h>
	
namespace libmaus2
{
	namespace parallel
	{		
                struct PosixSemaphore : public TimedSemaphoreInterface
                {
                	typedef PosixSemaphore this_type;
                	typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
                	typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
                
			#if defined(__APPLE__)
			std::string semname;
			#endif
		        sem_t semaphore;
			sem_t * psemaphore;
			
			private:
			PosixSemaphore(PosixSemaphore const &);
			PosixSemaphore operator=(PosixSemaphore const &);
			
			public:
                        PosixSemaphore() : semaphore(), psemaphore(0)
                        {
				#if defined(__APPLE__)
				std::ostringstream semnamestr;
				semnamestr << "real_" << getpid() << "_" << reinterpret_cast<u_int64_t>(this);
				semname = semnamestr.str();
				#endif

				memset ( & semaphore, 0, sizeof(sem_t) );        

				#if defined(__APPLE__)
				psemaphore = sem_open(semname.c_str(), O_CREAT | O_EXCL, 0777, 0);
				if ( psemaphore == SEM_FAILED )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "sem_open failed: " << strerror(errno) << std::endl;
					se.finish();
					throw se;
				}
				#else
				if ( sem_init ( & semaphore, 0, 0 ) )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "sem_init failed: " << strerror(errno) << std::endl;
					se.finish();
					throw se;
				}
				psemaphore = &semaphore;
				#endif
                        }
                        ~PosixSemaphore()
                        {
				#if defined(__APPLE__)
				sem_close(psemaphore);
				sem_unlink(semname.c_str());
				#else
				sem_destroy(psemaphore);        
				#endif
                        }

                        void post()
                        {
                        	while ( sem_post ( psemaphore ) != 0 )
                        	{
                        		int const lerrno = errno;
                        		
                        		switch ( lerrno )
                        		{
                        			case EINTR:
							break;
						default:
						{
							libmaus2::exception::LibMausException se;
							se.getStream() << "PosixSemaphore::post(): " << strerror(lerrno) << std::endl;
							se.finish();
							throw se;
						}
                        		}
                        	}
                        }

                        void wait()
                        {
                        	while ( sem_wait ( psemaphore ) != 0 )
                        	{
                        		int const lerrno = errno;
                        		
                        		switch ( lerrno )
                        		{
                        			case EINTR:
							break;
						default:
						{
							libmaus2::exception::LibMausException se;
							se.getStream() << "PosixSemaphore::wait(): " << strerror(lerrno) << std::endl;
							se.finish();
							throw se;
						}
                        		}
                        	
                        	}
			}

                        bool trywait()
                        {
                        	while ( sem_trywait ( psemaphore ) != 0 )
                        	{
                        		int const lerrno = errno;
                        		
                        		switch ( lerrno )
                        		{
                        			case EAGAIN:
                        				return false;
                        				break;
                        			case EINTR:
							break;
						default:
						{
							libmaus2::exception::LibMausException se;
							se.getStream() << "PosixSemaphore::trywait(): " << strerror(lerrno) << std::endl;
							se.finish();
							throw se;
						}
                        		}
                        	}
                        	
                        	return true;
			}
                        
                        bool timedWait()
                        {
                        	#if defined(LIBMAUS2_HAVE_SEM_TIMEDWAIT)
                        	// time structures
                                struct timeval tv;
                                struct timezone tz;
                                struct timespec waittime;

                                // get time of day
                                gettimeofday(&tv,&tz);
                                
                                // set wait time to 1 second
                                waittime.tv_sec = tv.tv_sec + 1;
                                waittime.tv_nsec = static_cast<u_int64_t>(tv.tv_usec)*1000;

                                int const v = sem_timedwait(psemaphore,&waittime);
                                
                                if ( v < 0 )
                                {
                                        if ( errno == ETIMEDOUT )
                                                return false;
                                        else
                                                throw std::runtime_error("timedWait failed on semaphore.");
                                }
                                else
                                {
                                        return true;
                                }
                                #else
				int const v = sem_trywait(psemaphore);

				if ( v < 0 )
				{
					if ( errno == EAGAIN )
						return false;
					else
					{
						std::cerr << "sem_trywait failed with error " << strerror(errno) << " on semaphore " << semname << std::endl;
						throw std::runtime_error("sem_trywait failed on semaphore.");
					}
				}   
				else
				{
					return true;
				}
                                #endif
                        }
                };
	}
}
#endif
#endif
