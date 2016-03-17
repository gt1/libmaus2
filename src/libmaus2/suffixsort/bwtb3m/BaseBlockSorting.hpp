#include <libmaus2/suffixsort/bwtb3m/BaseBlockSortThread.hpp>

/**
    libmaus2
    Copyright (C) 2009-2016 German Tischler
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
**/
#if ! defined(LIBMAUS2_SUFFIXSORT_BWTB3M_BASEBLOCKSORTING_HPP)
#define LIBMAUS2_SUFFIXSORT_BWTB3M_BASEBLOCKSORTING_HPP

namespace libmaus2
{
	namespace suffixsort
	{
		namespace bwtb3m
		{
			/**
			 * a set of thread for block sorting
			 **/
			struct BaseBlockSorting
			{
				typedef BaseBlockSorting this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

				std::vector < libmaus2::suffixsort::bwtb3m::MergeStrategyBlock::shared_ptr_type > & V;

				libmaus2::parallel::PosixSemaphore P;
				uint64_t next;
				uint64_t freemem;
				uint64_t finished;
				libmaus2::parallel::PosixMutex freememlock;
				//! inner node queue
				std::deque<libmaus2::suffixsort::bwtb3m::MergeStrategyBlock *> & itodo;
				std::deque<uint64_t> pending;

				libmaus2::autoarray::AutoArray<BaseBlockSortThread::unique_ptr_type> threads;

				BaseBlockSorting(
					std::vector < libmaus2::suffixsort::bwtb3m::MergeStrategyBlock::shared_ptr_type > & rV,
					uint64_t const rfreemem,
					uint64_t const numthreads,
					//! inner node queue
					std::deque<libmaus2::suffixsort::bwtb3m::MergeStrategyBlock *> & ritodo
				)
				: V(rV), P(), next(0), freemem(rfreemem), finished(0), freememlock(), itodo(ritodo), threads(numthreads)
				{
					for ( uint64_t i = 0; i < V.size(); ++i )
						if ( V[i]->directSortSpace() > freemem )
						{
							libmaus2::exception::LibMausException se;
							se.getStream() << "Memory provided is " << freemem << " but "
								<< V[i]->directSortSpace() << " are required for sorting block " << i << std::endl;
							se.finish();
							throw se;
						}

					for ( uint64_t i = 0; i < numthreads; ++i )
					{
						BaseBlockSortThread::unique_ptr_type tthreadsi(
							new BaseBlockSortThread(i,P,V,next,freemem,finished,freememlock,itodo,pending)
						);
						threads[i] = UNIQUE_PTR_MOVE(tthreadsi);

					}
				}

				void setup()
				{
					while ( next < V.size() && freemem >= V[next]->directSortSpace() )
					{
						freemem -= V[next]->directSortSpace();
						pending.push_back(next);
						next += 1;
					}
					uint64_t const p = pending.size();
					for ( uint64_t i = 0; i < p; ++i )
						P.post();
				}

				void start(uint64_t const stacksize)
				{
					for ( uint64_t i = 0; i < threads.size(); ++i )
						threads[i]->startStack(stacksize);

					setup();
				}

				void start()
				{
					for ( uint64_t i = 0; i < threads.size(); ++i )
						threads[i]->start();

					setup();
				}

				void join()
				{
					try
					{
						for ( uint64_t i = 0; i < threads.size(); ++i )
							threads[i]->join();
					}
					catch(std::exception const & ex)
					{
						std::cerr << ex.what() << std::endl;
					}
				}
			};
		}
	}
}
#endif
