/*
    libmaus2
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
#if ! defined(LIBMAUS2_PARALLEL_THREADPOOL_HPP)
#define LIBMAUS2_PARALLEL_THREADPOOL_HPP

#include <libmaus2/parallel/PosixSemaphore.hpp>
#include <libmaus2/parallel/TerminatableSynchronousHeap.hpp>
#include <libmaus2/parallel/ThreadPoolThread.hpp>
#include <libmaus2/parallel/ThreadWorkPackageComparator.hpp>
#include <libmaus2/util/unordered_map.hpp>

namespace libmaus2
{
	namespace parallel
	{		
		struct ThreadPool : public ThreadPoolInterface
		{
			typedef ThreadPool this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			
			uint64_t nextpackageid;
			libmaus2::parallel::PosixMutex nextpackageidlock;
			
			libmaus2::parallel::PosixSemaphore startsem;
			
			std::map<uint64_t, ThreadWorkPackage::shared_ptr_type> packagemap;
			
			libmaus2::autoarray::AutoArray<ThreadPoolThread::unique_ptr_type> threads;
			libmaus2::parallel::TerminatableSynchronousHeap<
				libmaus2::parallel::ThreadWorkPackage *,
				libmaus2::parallel::ThreadWorkPackageComparator
			> Q;
			
			libmaus2::util::unordered_map<uint64_t,ThreadWorkPackageDispatcher *>::type dispatchers;
			
			ThreadPool(uint64_t const rnumthreads)
			: nextpackageid(0), threads(rnumthreads)
			{
				for ( uint64_t i = 0; i < threads.size(); ++i )
				{
					ThreadPoolThread::unique_ptr_type tptr(new ThreadPoolThread(*this));
					threads[i] = UNIQUE_PTR_MOVE(tptr);
				}
				for ( uint64_t i = 0; i < threads.size(); ++i )
				{
					threads[i]->start();
					// wait until thread is running
					startsem.wait();
				}
			}
			~ThreadPool()
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
						
			void enque(ThreadWorkPackage const & P)
			{
				ThreadWorkPackage::shared_ptr_type SP = P.sclone();
				libmaus2::parallel::ScopePosixMutex lock(nextpackageidlock);
				SP->packageid = nextpackageid++;
				packagemap[SP->packageid] = SP;
				Q.enque(SP.get());
			}
			
			void terminate()
			{
				Q.terminate();
			}
			void freePackage(libmaus2::parallel::ThreadWorkPackage * P)
			{
				libmaus2::parallel::ScopePosixMutex lock(nextpackageidlock);
				std::map<uint64_t, ThreadWorkPackage::shared_ptr_type>::iterator it = 
					packagemap.find(P->packageid);			
				if ( it != packagemap.end() )
					packagemap.erase(it);
			}
			void notifyThreadStart()
			{
				startsem.post();
			}
			ThreadWorkPackage * getPackage()
			{
				return Q.deque();
			}
			ThreadWorkPackageDispatcher * getDispatcher(libmaus2::parallel::ThreadWorkPackage * P)
			{
				libmaus2::util::unordered_map<uint64_t,ThreadWorkPackageDispatcher *>::type::iterator it = 
					dispatchers.find(P->dispatcherid);
				assert ( it != dispatchers.end() );
				return it->second;
			}
			void registerDispatcher(uint64_t const id, ThreadWorkPackageDispatcher * D)
			{
				dispatchers[id] = D;
			}
		};
	}
}
#endif
