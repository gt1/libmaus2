/*
    libmaus
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS_PARALLEL_THREADPOOLTHREAD_HPP)
#define LIBMAUS_PARALLEL_THREADPOOLTHREAD_HPP

#include <libmaus/parallel/PosixThread.hpp>
#include <libmaus/parallel/SimpleThreadPoolInterface.hpp>
#include <libmaus/parallel/SimpleThreadWorkPackage.hpp>
#include <libmaus/parallel/SimpleThreadWorkPackageDispatcher.hpp>

namespace libmaus
{
	namespace parallel
	{		
		struct SimpleThreadPoolThread : libmaus::parallel::PosixThread
		{
			typedef SimpleThreadPoolThread this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			SimpleThreadPoolInterface & tpi;

			libmaus::parallel::PosixSpinLock curpacklock;
			libmaus::parallel::SimpleThreadWorkPackage * curpack;
			
			SimpleThreadPoolThread(SimpleThreadPoolInterface & rtpi) : tpi(rtpi), curpack(0)
			{
			}
			virtual ~SimpleThreadPoolThread() {}
			
			libmaus::parallel::SimpleThreadWorkPackage * getCurrentPackage()
			{
				libmaus::parallel::ScopePosixSpinLock lcurpacklock(curpacklock);
				return curpack;				
			}
		
			void * run()
			{
				try
				{
					// notify pool this thread is now running
					tpi.notifyThreadStart();
					
					bool running = true;
					while ( running )
					{
						libmaus::parallel::SimpleThreadWorkPackage * P = tpi.getPackage();
						{
							libmaus::parallel::ScopePosixSpinLock lcurpacklock(curpacklock);
							curpack = P;
						}
						SimpleThreadWorkPackageDispatcher * disp = tpi.getDispatcher(P);
						
						try
						{
							disp->dispatch(P,tpi);
						}
						catch(libmaus::exception::LibMausException const & ex)
						{
							tpi.panic(ex);
						}
						catch(std::exception const & ex)
						{
							tpi.panic(ex);
						}
						
						{
							libmaus::parallel::ScopePosixSpinLock lcurpacklock(curpacklock);
							curpack = 0;							
						}
					}
				}
				catch(std::exception const & ex)
				{
					// std::cerr << ex.what() << std::endl;	
				}
								
				return 0;
			}		
		};
	}
}
#endif
