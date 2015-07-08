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

#if ! defined(SYNCHRONOUSREORDERSET_HPP)
#define SYNCHRONOUSREORDERSET_HPP

#include <libmaus2/LibMausConfig.hpp>

#if defined(LIBMAUS2_HAVE_PTHREADS)
#include <libmaus2/parallel/PosixMutex.hpp>
#include <libmaus2/parallel/PosixConditionSemaphore.hpp>
#include <deque>
#include <set>

namespace libmaus2
{
        namespace parallel
        {
                template<typename value_type>
                struct SynchronousReorderSet
                {
                	typedef std::pair< uint64_t, value_type > pair_type;
                	typedef typename ::std::set<pair_type>::iterator iterator_type;

			struct SynchronousReorderSetPairComp
			{
				SynchronousReorderSetPairComp() {}
				
				bool operator()(pair_type const & A, pair_type const & B)
				{
					if ( A.first != B.first )
						return A.first < B.first;
					else
						return A.second < B.second;
				}
			};

                	uint64_t next;
                        std::set < pair_type > Q;
                        PosixMutex lock;
                        PosixConditionSemaphore semaphore;
                        
                        SynchronousReorderSet(uint64_t const rnext = 0)
                        : next(rnext)
                        {
                        
                        }
                        
                        unsigned int getFillState()
                        {
                                lock.lock();
                                unsigned int const fill = Q.size();
                                lock.unlock();
                                return fill;
                        }
                        
                        void enque(uint64_t const idx, value_type const q)
                        {
                        	uint64_t postcnt = 0;
                        	
                                lock.lock();
                                Q.insert(pair_type(idx,q));
                                
                                for ( iterator_type I = Q.begin(); I != Q.end(); ++I )
                                	if ( I->first == next )
                                		next++, postcnt++;

                                lock.unlock();
                                
                                for ( uint64_t i = 0; i < postcnt; ++i )
	                                semaphore.post();
                        }
                        value_type deque()
                        {
                                semaphore.wait();
                                lock.lock();
                                iterator_type const I = Q.begin();
                                std::pair<uint64_t,value_type> const P = *I;
                                Q.erase(I);
                                lock.unlock();
                                return P.second;
                        }
                };
        }
}
#endif
#endif
