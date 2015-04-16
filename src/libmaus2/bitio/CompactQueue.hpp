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
#if ! defined(COMPACTQUEUE_HPP)
#define COMPACTQUEUE_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/bitio/CompactArray.hpp>
#include <libmaus2/parallel/OMPLock.hpp>

namespace libmaus2
{
        namespace suffixsort
        {
                struct CompactQueue
                {
                        uint64_t const n;
                        ::libmaus2::bitio::CompactArray Q;
                        uint64_t fill;
                        ::libmaus2::parallel::OMPLock batchlock;

                        template<typename iterator>
                        void enqueBatch(iterator a, iterator e)
                        {
                                batchlock.lock();
                                while ( a !=e )
                                {
                                        enque(a->first,a->second);
                                        ++a;
                                }
                                batchlock.unlock();
                        }

                        CompactQueue(uint64_t const rn)
                        : n(rn), Q(n,2), fill(0)
                        {
                        
                        }
                        
                        void print()
                        {
                                for ( uint64_t i = 0; i < n; ++i )
                                        std::cerr << Q.get(i);
                                std::cerr << std::endl;
                        }
                        
                        void reset()
                        {
                                Q.erase();
                                fill = 0;
                        }
                        
                        void enque(uint64_t const sp, uint64_t const ep)
                        {
                                assert ( ep>sp);

                                #if 0
                                std::cerr << "enque(" << sp << "," << ep << ")" << std::endl;
                                #endif
                                
                                if ( ep-1 == sp )
                                {
                                        Q.set(sp,3);
                                }
                                else
                                {
                                        Q.set(sp,1);
                                        Q.set(ep-1,2);
                                }
                                
                                fill += 1;
                        }
                        
                        struct DequeContext
                        {
                                typedef DequeContext this_type;
                                typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
                                
                                uint64_t decptr;
                                uint64_t fill;
                                
                                DequeContext(uint64_t const rdecptr, uint64_t const rfill)
                                : decptr(rdecptr), fill(rfill)
                                {
                                
                                }
                                
                                bool done() const
                                {
                                        return (fill == 0);
                                }
                        };
                        
                        DequeContext::unique_ptr_type getGlobalDequeContext()
                        {
                                return DequeContext::unique_ptr_type(new DequeContext(0,fill));
                        }
                        
                        ::libmaus2::autoarray::AutoArray< DequeContext::unique_ptr_type > getContextList(uint64_t numcontexts) const
                        {
                                uint64_t const intervalsize = (n+numcontexts-1)/numcontexts;
                                ::libmaus2::autoarray::AutoArray< DequeContext::unique_ptr_type > contexts(numcontexts);
                                
                                #if defined(_OPENMP)
                                #pragma omp parallel for
                                #endif
                                for ( int64_t z = 0; z < static_cast<int64_t>(numcontexts); ++z )
                                {
                                        uint64_t const left = std::min(z*intervalsize,n);
                                        uint64_t const right = std::min(left+intervalsize,n);
                                        uint64_t lfill = 0;
                                        
                                        for ( uint64_t i = left; i < right; ++i )
                                                if ( Q.get(i) & 1 )
                                                        lfill += 1;
                                        
					DequeContext::unique_ptr_type tcontextsz(new DequeContext(left,lfill));
                                        contexts[z] =  UNIQUE_PTR_MOVE(tcontextsz);
                                }
                                
                                return contexts;
                        }
                        
                        std::pair<uint64_t,uint64_t> deque(DequeContext * context) const
                        {
                                assert ( context->fill );
                                context->fill -= 1;
                        
                                uint64_t v;
                                while ( ! ((v=Q.get(context->decptr))&1) )
                                        context->decptr++;
                                        
                                if ( v == 3 )
                                {
                                        uint64_t const sp = context->decptr;
                                        uint64_t const ep = context->decptr+1;
                                        context->decptr += 1;
                                        
                                        #if 0
                                        std::cerr << "deque(" << sp << "," << ep << ")" << std::endl;
                                        #endif
                                        
                                        return std::pair<uint64_t,uint64_t>(sp,ep);
                                }
                                else
                                {
                                        assert ( v == 1 );
                                        uint64_t const sp = context->decptr;
                                        context->decptr +=1 ;
                                        
                                        while ( ! (v=Q.get(context->decptr)) )
                                                context->decptr++;

                                        assert ( v == 2 );
                                        context->decptr += 1;
                                        
                                        uint64_t const ep = context->decptr;
                                        
                                        #if 0
                                        std::cerr << "deque(" << sp << "," << ep << ")" << std::endl;
                                        #endif
                                        
                                        return std::pair<uint64_t,uint64_t>(sp,ep);
                                }
                        }
                        
                        
                        struct EnqueBuffer
                        {
                                typedef EnqueBuffer this_type;
                                typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
                        
                                private:
                                CompactQueue * const Q;
                                ::libmaus2::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > B;
                                std::pair<uint64_t,uint64_t> * const pa;
                                std::pair<uint64_t,uint64_t> *       pc;
                                std::pair<uint64_t,uint64_t> * const pe;
                                
                                void commit()
                                {
                                        Q->enqueBatch(pa,pc);
                                        pc = pa;
                                }
                                
                                public:
                                EnqueBuffer(CompactQueue * rQ, uint64_t const bufsize = 64*1024)
                                : Q(rQ), B(bufsize,false), pa(B.begin()), pc(pa), pe(B.end())
                                {}
                                
                                ~EnqueBuffer()
                                {
                                        flush();
                                }
                                
                                void enque(std::pair<uint64_t,uint64_t> const P)
                                {
                                        *(pc++) = P;
                                        if ( pc == pe )
                                                commit();
                                }

                                void push_back(std::pair<uint64_t,uint64_t> const P)
                                {
                                        enque(P);
                                }
                                
                                void flush()
                                {
                                         commit();       
                                }
                        };
                        
                        EnqueBuffer::unique_ptr_type createEnqueBuffer(uint64_t const bufsize = 64*1024)
                        {
				EnqueBuffer::unique_ptr_type ptr(new EnqueBuffer(this,bufsize));
                                return UNIQUE_PTR_MOVE(ptr);
                        }
                };                
        }
}
#endif
