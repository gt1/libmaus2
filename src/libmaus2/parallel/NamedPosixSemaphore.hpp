/*
    libmaus2
    Copyright (C) 2011-2013 Genome Research Limited
    Copyright (C) 2009-2015 German Tischler

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
#if ! defined(LIBMAUS2_PARALLEL_NAMEDPOSIXSEMAPHORE_HPP)
#define LIBMAUS2_PARALLEL_NAMEDPOSIXSEMAPHORE_HPP

#include <cstring>              
#include <string>
#include <sstream>
#include <iomanip>

#include <libmaus2/exception/LibMausException.hpp>
#include <libmaus2/parallel/SimpleSemaphoreInterface.hpp>

#if defined(LIBMAUS2_HAVE_PTHREADS)
#include <fcntl.h>           /* For O_* constants */
#include <semaphore.h>
#include <sys/stat.h>        /* For mode constants */
#include <sys/types.h>
#include <unistd.h>

namespace libmaus2
{
	namespace parallel
	{
		struct NamedPosixSemaphore : public libmaus2::parallel::SimpleSemaphoreInterface
		{
			private:
			NamedPosixSemaphore & operator=(NamedPosixSemaphore const &);
			NamedPosixSemaphore(NamedPosixSemaphore const &);
		
			public:
			std::string const semname;
			bool const primary;
			sem_t * psemaphore;
			
			static std::string getDefaultName()
			{
				std::ostringstream ostr;
				ostr << "/s" 
					<< std::hex << std::setw(4) << std::setfill('0') << (getpid()&0xFFFF) << std::setw(0) << std::dec
					<< std::hex << std::setw(8) << std::setfill('0') << (time(0)&0xFFFFFFFFULL) << std::setw(0) << std::dec;
				return ostr.str();			
			}
			
			NamedPosixSemaphore(std::string const & rsemname, bool rprimary)
			: semname(rsemname), primary(rprimary), psemaphore(0)
			{
				if ( primary )
					psemaphore = sem_open(semname.c_str(), O_CREAT | O_EXCL, 0700, 0);
				else
					psemaphore = sem_open(semname.c_str(), 0);
					
				if ( psemaphore == SEM_FAILED )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Failed to open semaphore " << semname << ": " << strerror(errno) << " primary " << primary << std::endl;
					se.finish();
					throw se;
				}
			}
			~NamedPosixSemaphore()
			{
				sem_close(psemaphore);
				if ( primary )
					sem_unlink(semname.c_str());
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

                        int getValue()
                        {
                        	int v = 0;
                        	int r = sem_getvalue(psemaphore,&v);
                        	
                        	if ( r != 0 )
                        	{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Failed to sem_getvalue: " << strerror(errno) << std::endl;
					se.finish();
					throw se;
                        	}
                        	
                        	return v;
                        }
                        
                        void assureNonZero()
                        {
                        	if ( !getValue() )
                        		post();
                        }
		};
	}
}
#endif // PTHREADS

#endif
