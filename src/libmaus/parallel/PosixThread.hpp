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

#if ! defined(POSIXMUTEXARRAY_HPP)
#define POSIXMUTEXARRAY_HPP

#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/parallel/PosixMutex.hpp>

#if defined(__FreeBSD__)
#include <pthread_np.h>
#endif

#if defined(LIBMAUS_HAVE_PRCTL)
#include <sys/prctl.h>      
#endif

namespace libmaus
{
	namespace parallel
	{
		struct PosixThread
		{
			typedef PosixThread this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::unique_ptr<pthread_t>::type thread_ptr_type;

			private:			
			thread_ptr_type thread;
			std::string name;

			static void * dispatch(void * p)
			{
				void * q = 0;
				try
				{
					this_type * t = reinterpret_cast<this_type *>(p);
					
					if ( t->name.size() )
						t->setName(t->name);

					q = t->run();
				}
				catch(std::exception const & ex)
				{
					std::cerr << "Uncaught exception in thread:" << std::endl;
					std::cerr << ex.what() << std::endl;
				}
				
				return q;
			}
			
			public:
			PosixThread(std::string const rname = std::string())
			: thread(), name(rname)
			{
			
			}
			virtual ~PosixThread()
			{
				if ( thread.get() )
					join();
			}
			
			#if defined(__FreeBSD__)
			typedef cpuset_t cpu_set_t;
			#endif
			
			#if defined(LIBMAUS_HAVE_PTHREAD_SETAFFINITY_NP)
			void setaffinity(std::vector<uint64_t> const & procs)
			{
				cpu_set_t cpuset;
				
				CPU_ZERO(&cpuset);
				for ( uint64_t i = 0; i < procs.size(); ++i )
					CPU_SET(procs[i],&cpuset);
					
				int const err = pthread_setaffinity_np(*thread, sizeof(cpu_set_t), &cpuset);
				
				if ( err != 0 )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "pthread_setaffinity_np failed: " << strerror(errno) << std::endl;
					se.finish();
					throw se;
				}
			}
			
			void setaffinity(uint64_t const proc)
			{
				setaffinity(std::vector<uint64_t>(1,proc));
			}
			#endif
			
			void startStack(uint64_t const rstacksize = 64*1024)
			{
				if ( ! thread.get() )
				{
					thread_ptr_type tthread(new pthread_t);
					thread = UNIQUE_PTR_MOVE(tthread);

					#if 0
					std::cerr << "Creating thread without affinity." << std::endl;
					std::cerr << ::libmaus::util::StackTrace::getStackTrace() << std::endl;
					#endif

					pthread_attr_t attr;
					if ( pthread_attr_init(&attr) )
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "pthread_attr_init failed:" << strerror(errno);
						se.finish();
						throw se; 					
					}

					uint64_t const stacksize = std::max(rstacksize,static_cast<uint64_t>(PTHREAD_STACK_MIN));

					if ( pthread_attr_setstacksize(&attr,stacksize) != 0 )
					{
						pthread_attr_destroy(&attr);
						::libmaus::exception::LibMausException se;
						se.getStream() << "pthread_attr_setstacksize() failed in PosixThread::startStack(): " << strerror(errno) << std::endl;
						se.finish();
						throw se; 						
					}
					
					
					if ( pthread_create(thread.get(),&attr,dispatch,this) )
					{
						pthread_attr_destroy(&attr);
						::libmaus::exception::LibMausException se;
						se.getStream() << "pthread_create() failed in PosixThread::start()";
						se.finish();
						throw se; 	
					}

					if ( pthread_attr_destroy(&attr) )
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "pthread_attr_destroy failed:" << strerror(errno);
						se.finish();
						throw se; 					
					
					}
				}
				else
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "PosixThread::start() called but object is already in use.";
					se.finish();
					throw se; 					
				}
			
			}
						
			void start()
			{
				if ( ! thread.get() )
				{
					thread_ptr_type tthread(new pthread_t);
					thread = UNIQUE_PTR_MOVE(tthread);

					#if 0
					std::cerr << "Creating thread without affinity." << std::endl;
					std::cerr << ::libmaus::util::StackTrace::getStackTrace() << std::endl;
					#endif
					
					if ( pthread_create(thread.get(),0,dispatch,this) )
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "pthread_create() failed in PosixThread::start()";
						se.finish();
						throw se; 	
					}
				}
				else
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "PosixThread::start() called but object is already in use.";
					se.finish();
					throw se; 					
				}
			}

			#if defined(__APPLE__)
			void start(uint64_t const)
			{
				start();
			}
			#endif

			#if defined(LIBMAUS_HAVE_PTHREAD_SETAFFINITY_NP)
			void start(uint64_t const proc)
			{
				start ( std::vector<uint64_t>(1,proc) );
			}

			void start(std::vector<uint64_t> const & procs)
			{
				if ( ! thread.get() )
				{
					thread_ptr_type tthread(new pthread_t);
					thread = UNIQUE_PTR_MOVE(tthread);
				
					pthread_attr_t attr;
					if ( pthread_attr_init(&attr) )
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "pthread_attr_init failed:" << strerror(errno);
						se.finish();
						throw se; 					
					}
					
					cpu_set_t cpuset;
				
					CPU_ZERO(&cpuset);
					for ( uint64_t i = 0; i < procs.size(); ++i )
						CPU_SET(procs[i],&cpuset);
					
					if ( pthread_attr_setaffinity_np(&attr,sizeof(cpu_set_t),&cpuset) )
					{
						pthread_attr_destroy(&attr);
						::libmaus::exception::LibMausException se;
						se.getStream() << "pthread_attr_setaffinity_np failed:" << strerror(errno);
						se.finish();
						throw se; 										
					}
					
					#if 0
					std::cerr << "Creating thread with affinity." << std::endl;
					std::cerr << ::libmaus::util::StackTrace::getStackTrace() << std::endl;
					#endif
					
					if ( pthread_create(thread.get(),&attr,dispatch,this) )
					{
						pthread_attr_destroy(&attr);
						::libmaus::exception::LibMausException se;
						se.getStream() << "pthread_create() failed in PosixThread::start()";
						se.finish();
						throw se; 	
					}
					
					if ( pthread_attr_destroy(&attr) )
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "pthread_attr_destroy failed:" << strerror(errno);
						se.finish();
						throw se; 					
					
					}
				}
				else
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "PosixThread::start() called but object is already in use.";
					se.finish();
					throw se; 					
				}
			}
			#endif
			
			virtual void * run() = 0;
			
			void * tryJoin()
			{
				if ( thread.get() )
				{
					void * p = 0;

					if ( pthread_join(*thread,&p) )
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "pthread_join() failed in PosixThread::join()";
						se.finish();
						throw se;				
					}
					
					thread.reset();
	
					return p;
				}
				else
				{
					return 0;
				}
			}
			
			void * join()
			{
				if ( thread.get() )
				{
					void * p = 0;

					if ( pthread_join(*thread,&p) )
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "pthread_join() failed in PosixThread::join()";
						se.finish();
						throw se;				
					}
					
					thread.reset();
	
					return p;
				}
				else
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "PosixThread::join() called but no thread initialised";
					se.finish();
					throw se; 	
				}
			}
			
			void setName(
				std::string const & 
				#if defined(LIBMAUS_HAVE_PRCTL)
					name
				#endif
			)
			{
				#if defined(LIBMAUS_HAVE_PRCTL) && defined(PR_SET_NAME)
				prctl(PR_SET_NAME,name.c_str(),0,0,0);
				#endif
			}
		};
	}
}
#endif
