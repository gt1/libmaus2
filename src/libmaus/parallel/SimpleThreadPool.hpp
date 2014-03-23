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
#if ! defined(LIBMAUS_PARALLEL_SIMPLETHREADPOOL_HPP)
#define LIBMAUS_PARALLEL_SIMPLETHREADPOOL_HPP

#include <libmaus/parallel/PosixSpinLock.hpp>
#include <libmaus/parallel/PosixSemaphore.hpp>
#include <libmaus/parallel/TerminatableSynchronousHeap.hpp>
#include <libmaus/parallel/SimpleThreadPoolThread.hpp>
#include <libmaus/parallel/SimpleThreadPoolInterface.hpp>
#include <libmaus/parallel/SimpleThreadWorkPackageDispatcher.hpp>
#include <libmaus/parallel/SimpleThreadWorkPackageComparator.hpp>
#include <libmaus/util/unordered_map.hpp>

namespace libmaus
{
	namespace parallel
	{		
		struct SimpleThreadPool : public SimpleThreadPoolInterface
		{
			typedef SimpleThreadPool this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			uint64_t nextpackageid;
			libmaus::parallel::PosixSpinLock nextpackageidlock;
			
			// semaphore for notifying about start completion
			libmaus::parallel::PosixSemaphore startsem;
				
			// threads		
			libmaus::autoarray::AutoArray<SimpleThreadPoolThread::unique_ptr_type> threads;
			// package heap
			libmaus::parallel::TerminatableSynchronousHeap<
				libmaus::parallel::SimpleThreadWorkPackage *,
				libmaus::parallel::SimpleThreadWorkPackageComparator
			> Q;
			
			// dispatcher map
			libmaus::util::unordered_map<uint64_t,SimpleThreadWorkPackageDispatcher *>::type dispatchers;
			
			SimpleThreadPool(uint64_t const rnumthreads)
			: nextpackageid(0), threads(rnumthreads)
			{
				for ( uint64_t i = 0; i < threads.size(); ++i )
				{
					SimpleThreadPoolThread::unique_ptr_type tptr(new SimpleThreadPoolThread(*this));
					threads[i] = UNIQUE_PTR_MOVE(tptr);
				}
				for ( uint64_t i = 0; i < threads.size(); ++i )
				{
					threads[i]->start();
					// wait until thread is running
					startsem.wait();
				}
			}
			~SimpleThreadPool()
			{
				join();
				
				for ( uint64_t i = 0; i < threads.size(); ++i )
					threads[i].reset();			
			}
			
			void join()
			{
				for ( uint64_t i = 0; i < threads.size(); ++i )
					threads[i]->tryJoin();			
			}
						
			void enque(SimpleThreadWorkPackage * P)
			{
				libmaus::parallel::ScopePosixSpinLock lock(nextpackageidlock);
				P->packageid = nextpackageid++;
				Q.enque(P);
			}
			
			void terminate()
			{
				Q.terminate();
			}
			void notifyThreadStart()
			{
				startsem.post();
			}
			SimpleThreadWorkPackage * getPackage()
			{
				return Q.deque();
			}
			SimpleThreadWorkPackageDispatcher * getDispatcher(libmaus::parallel::SimpleThreadWorkPackage * P)
			{
				libmaus::util::unordered_map<uint64_t,SimpleThreadWorkPackageDispatcher *>::type::iterator it = 
					dispatchers.find(P->dispatcherid);
				assert ( it != dispatchers.end() );
				return it->second;
			}
			void registerDispatcher(uint64_t const id, SimpleThreadWorkPackageDispatcher * D)
			{
				dispatchers[id] = D;
			}
		};
	}
}
#endif
