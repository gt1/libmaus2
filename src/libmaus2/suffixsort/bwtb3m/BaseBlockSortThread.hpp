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
#if !defined(LIBMAUS2_SUFFIXSORT_BWTB3M_BASEBLOCKSORTTHREAD_HPP)
#define LIBMAUS2_SUFFIXSORT_BWTB3M_BASEBLOCKSORTTHREAD_HPP

#include <libmaus2/parallel/PosixThread.hpp>
#include <libmaus2/parallel/PosixSemaphore.hpp>
#include <libmaus2/suffixsort/bwtb3m/MergeStrategyBaseBlock.hpp>
#include <libmaus2/suffixsort/bwtb3m/BWTB3MBase.hpp>

namespace libmaus2
{
	namespace suffixsort
	{
		namespace bwtb3m
		{
			/**
			 * sorting thread for base blocks
			 **/
			struct BaseBlockSortThread : public libmaus2::parallel::PosixThread, public libmaus2::suffixsort::bwtb3m::BWTB3MBase
			{
				typedef BaseBlockSortThread this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

				//! thread id
				uint64_t tid;

				/**
				 * semaphore. delivers a message whenever there is sufficient
				 * free space to process the next element
				 **/
				libmaus2::parallel::PosixSemaphore & P;
				//! vector of blocks to be processed
				std::vector < libmaus2::suffixsort::bwtb3m::MergeStrategyBlock::shared_ptr_type > & V;

				//! next package to be processed
				volatile uint64_t & next;
				//! amount of free memory
				volatile uint64_t & freemem;
				//! number of finished threads
				volatile uint64_t & finished;
				//! lock for the above
				libmaus2::parallel::PosixMutex & freememlock;
				//! inner node queue
				std::deque<libmaus2::suffixsort::bwtb3m::MergeStrategyBlock *> & itodo;
				//! pending
				std::deque<uint64_t> & pending;
				//! log stream
				std::ostream * logstr;

				BaseBlockSortThread(
					uint64_t rtid,
					libmaus2::parallel::PosixSemaphore & rP,
					std::vector < libmaus2::suffixsort::bwtb3m::MergeStrategyBlock::shared_ptr_type > & rV,
					uint64_t & rnext,
					uint64_t & rfreemem,
					uint64_t & rfinished,
					libmaus2::parallel::PosixMutex & rfreememlock,
					std::deque<libmaus2::suffixsort::bwtb3m::MergeStrategyBlock *> & ritodo,
					std::deque<uint64_t> & rpending,
					std::ostream * rlogstr
				) : tid(rtid), P(rP), V(rV), next(rnext), freemem(rfreemem), finished(rfinished), freememlock(rfreememlock),
				    itodo(ritodo), pending(rpending), logstr(rlogstr)
				{

				}

				void * run()
				{
					bool running = true;

					while ( running )
					{
						// wait until sufficient memory is free
						P.wait();

						// get package id
						uint64_t pack = 0;

						{
							// get lock
							libmaus2::parallel::ScopePosixMutex scopelock(freememlock);

							if ( pending.size() )
							{
								pack = pending.front();
								pending.pop_front();
							}
							else
							{
								running = false;
								P.post();
							}
						}

						if ( running )
						{
							try
							{
								// perform sorting
								libmaus2::suffixsort::bwtb3m::MergeStrategyBaseBlock * block = dynamic_cast<libmaus2::suffixsort::bwtb3m::MergeStrategyBaseBlock *>(V[pack].get());
								block->sortresult = ::libmaus2::suffixsort::BwtMergeBlockSortResult::load(block->sortreq.dispatch<rl_encoder>());
							}
							catch(std::exception const & ex)
							{
								libmaus2::parallel::ScopePosixMutex scopelock(freememlock);
								if ( logstr )
									*logstr << tid << " failed " << pack << " " << ex.what() << std::endl;
							}

							{
								// get lock
								libmaus2::parallel::ScopePosixMutex scopelock(freememlock);

								if ( logstr )
									*logstr << "[V] [" << tid << "] sorted block " << pack << std::endl;

								if ( V[pack]->parent )
								{
									bool const pfinished = V[pack]->parent->childFinished();

									if ( pfinished )
									{
										itodo.push_back(V[pack]->parent);
									}
								}

								// "free" memory
								freemem += V[pack]->directSortSpace();

								// post if there is room for another active sorting thread
								while ( next < V.size() && freemem >= V[next]->directSortSpace() )
								{
									freemem -= V[next]->directSortSpace();
									pending.push_back(static_cast<uint64_t>(next));
									next += 1;
									P.post();
								}

								if ( next == V.size() )
									P.post();
							}
						}
					}

					{
					libmaus2::parallel::ScopePosixMutex scopelock(freememlock);
					finished++;
					}

					// quit
					return 0;
				}
			};
		}
	}
}
#endif
