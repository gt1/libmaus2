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

#if ! defined(EDGEBUFFER_HPP)
#define EDGEBUFFER_HPP

#include <libmaus2/graph/EdgeList.hpp>
#include <libmaus2/parallel/SynchronousQueue.hpp>
#include <libmaus2/parallel/PosixThread.hpp>
#include <iomanip>

namespace libmaus2
{
	namespace graph
	{
	        template<typename _edge_type>
		struct EdgeBufferBase
		{
		        typedef _edge_type edge_type;
			typedef EdgeBufferBase<edge_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			::libmaus2::autoarray::AutoArray< edge_type > B;
			edge_type * const pa;
			edge_type * pc;
			edge_type * const pe;
			uint64_t const id;

			EdgeBufferBase(uint64_t const bufsize, uint64_t const rid = 0)
			: B(bufsize,false), pa(B.get()), pc(pa), pe(pa+bufsize), id(rid)
			{

			}

			void reset()
			{
				pc = pa;
			}

			bool put(edge_type const & T)
			{
				*(pc++)	= T;
				return ( pc == pe );
			}
		};


		template<typename flush_type>
		struct EdgeBuffer : public EdgeBufferBase< typename flush_type::edge_type>
		{
		        typedef typename flush_type::edge_type edge_type;

			typedef EdgeBufferBase< edge_type > base_type;
			flush_type & F;

			void flush()
			{
				F(base_type::pa,base_type::pc-base_type::pa);
				base_type::reset();
			}

			EdgeBuffer(uint64_t const bufsize, flush_type & rF)
			: base_type(bufsize), F(rF)
			{
			}
			~EdgeBuffer()
			{
				flush();
			}
			void put(edge_type const & T)
			{
				if ( base_type::put(T) )
					flush();
			}
		};

		template < typename _edge_type >
		struct EdgeBufferBaseSet : public ::libmaus2::parallel::PosixThread
		{
		        typedef _edge_type edge_type;
			typedef EdgeBufferBaseSet<edge_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef EdgeBufferBase<edge_type> buffer_type;
			typedef typename ::libmaus2::util::unique_ptr<buffer_type>::type buffer_ptr_type;

			::libmaus2::autoarray::AutoArray < buffer_ptr_type > B;
			::libmaus2::parallel::SynchronousQueue<int64_t> fullqueue;
			::libmaus2::parallel::SynchronousQueue<int64_t> emptyqueue;
			uint64_t const numwriters;
			EdgeList & EL;

			EdgeBufferBaseSet(
				uint64_t const numbufs,
				uint64_t const bufsize,
				uint64_t const rnumwriters,
				EdgeList & rEL
			)
			: B(numbufs), numwriters(rnumwriters), EL(rEL)
			{
				for ( uint64_t i = 0; i < numbufs; ++i )
				{
					B[i] = UNIQUE_PTR_MOVE(buffer_ptr_type(new buffer_type(bufsize,i)));
					emptyqueue.enque(i);
				}
				start();
			}
			~EdgeBufferBaseSet()
			{
			        join();
			}

			buffer_type * getBuffer()
			{
				int64_t const bufid = emptyqueue.deque();
				return B[bufid].get();
			}

			void returnBuffer(buffer_type * buffer)
			{
				fullqueue.enque(buffer->id);
			}

			void terminate()
			{
				fullqueue.enque(-1);
			}

			void * run()
			{
				uint64_t numfinished = 0;

				while ( numfinished < numwriters )
				{
					int64_t const bufid = fullqueue.deque();

					if ( bufid < 0 )
					{
						numfinished++;
					}
					else
					{
						buffer_type & buffer = *B[bufid];
						EL(buffer.pa,buffer.pc-buffer.pa);
						buffer.reset();
						emptyqueue.enque(buffer.id);
					}
				}

				return 0;
			}
		};

		template<typename _edge_type>
		struct EdgeBufferProxy
		{
		        typedef _edge_type edge_type;
			typedef EdgeBufferProxy<edge_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			EdgeBufferBaseSet<edge_type> & EBBS;
			typename EdgeBufferBaseSet<edge_type>::buffer_type * buffer;

			EdgeBufferProxy(EdgeBufferBaseSet<edge_type> & rEBBS)
			: EBBS(rEBBS), buffer(EBBS.getBuffer())
			{
			}
			~EdgeBufferProxy()
			{
				EBBS.returnBuffer(buffer);
				EBBS.terminate();
			}
			void put(edge_type const & T)
			{
				if ( buffer->put(T) )
				{
					EBBS.returnBuffer(buffer);
					buffer = EBBS.getBuffer();
				}
			}
		};

		template<typename _edge_type>
		struct EdgeBufferProxySet
		{
		        typedef _edge_type edge_type;
			typedef EdgeBufferProxySet<edge_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef EdgeBufferProxy<edge_type> proxy_type;
			typedef typename proxy_type::unique_ptr_type proxy_ptr_type;

			::libmaus2::autoarray::AutoArray < proxy_ptr_type > proxies;

			EdgeBufferProxySet(EdgeBufferBaseSet<edge_type> & EBBS, uint64_t const numproxies)
			: proxies(numproxies)
			{
				for ( uint64_t i = 0; i < numproxies; ++i )
					proxies[i] = UNIQUE_PTR_MOVE(proxy_ptr_type(new proxy_type(EBBS)));
			}

			proxy_type * operator[](uint64_t const i)
			{
			        return proxies[i].get();
			}
		};

		template<typename _edge_type>
		struct ParallelEdgeList : public EdgeList
		{
		        typedef _edge_type edge_type;
		        typedef ParallelEdgeList<edge_type> this_type;
		        typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			typedef EdgeBufferBaseSet<edge_type> buffer_base_set;
			typedef typename buffer_base_set::unique_ptr_type buffer_base_set_ptr;
			typedef EdgeBufferProxySet<_edge_type> buffer_proxy_set;
			typedef typename buffer_proxy_set::unique_ptr_type buffer_proxy_set_ptr;
			typedef typename buffer_proxy_set::proxy_type proxy_type;

			buffer_base_set_ptr bbs;
			buffer_proxy_set_ptr bps;

			ParallelEdgeList(
				uint64_t const redgelow, uint64_t const redgehigh, uint64_t const rmaxedges,
				uint64_t const numbufs, uint64_t const bufsize, uint64_t const numwriters
			)
			: EdgeList(redgelow,redgehigh,rmaxedges),
			  bbs(new buffer_base_set(numbufs,bufsize,numwriters,*this)),
			  bps(new buffer_proxy_set(*bbs,numwriters))
			{}
			~ParallelEdgeList()
			{
			        terminate();
			}

			void terminate()
			{
			        bps.reset();
			        bbs.reset();
			}

			proxy_type * operator[](uint64_t const i)
			{
			        return (*bps)[i];
			}
		};

		template<typename _edge_type>
		struct ParallelEdgeListSet
		{
		        typedef _edge_type edge_type;
		        typedef ParallelEdgeListSet<edge_type> this_type;
		        typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

		        typedef ParallelEdgeList<edge_type> edge_list_type;
		        typedef typename edge_list_type::unique_ptr_type edge_list_ptr_type;
		        typedef typename ParallelEdgeList<edge_type>::proxy_type proxy_type;
		        typedef proxy_type buffer_type;

		        uint64_t const numlists;
		        uint64_t const readsperlist;
		        ::libmaus2::autoarray::AutoArray < edge_list_ptr_type > edgelists;
		        ::libmaus2::autoarray::AutoArray < proxy_type * > proxies;

		        ParallelEdgeListSet(
		                uint64_t const edgelow,
		                uint64_t const edgehigh,
		                uint64_t const maxedges,
		                uint64_t const rnumlists,
		                uint64_t const numbufs,
		                uint64_t const bufsize,
		                uint64_t const numwriters
		        ) : numlists(rnumlists),
		            readsperlist(((edgehigh-edgelow+numlists-1)/numlists)), edgelists(numlists), proxies(numlists*numwriters)
		        {
		                 for ( uint64_t i = 0; i < numlists; ++i )
		                 {
		                         uint64_t const l = std::min(edgelow + i*readsperlist,edgehigh);
		                         uint64_t const h = std::min(l + readsperlist,edgehigh);
		                         edgelists[i] = UNIQUE_PTR_MOVE(edge_list_ptr_type(new edge_list_type(l,h,maxedges,numbufs,bufsize,numwriters)));

		                         for ( uint64_t j = 0; j < numwriters; ++j )
		                                 proxies[numlists*j+i] = (*(edgelists[i]))[j];
		                 }
		        }
		        void terminate()
		        {
		                for ( uint64_t i = 0; i < edgelists.size(); ++i )
		                        edgelists[i]->terminate();
		        }

		        buffer_type ** getBuffers(uint64_t const readerid)
		        {
		                return proxies.get() + (readerid * numlists);
		        }

		        uint64_t writeFiles(
		                std::string const & filenamebase, uint64_t & base,
		                std::vector < std::string > & listfilenames,
		                std::vector < std::string > & linkfilenames,
		                std::vector < std::string > & readfilenames
                        )
		        {
		                uint64_t numedges = 0;

		                for ( uint64_t i = 0; i < numlists; ++i )
		                {
		                        std::ostringstream fnostr;
		                        fnostr << filenamebase << "_" << std::setw(4) << std::setfill('0') << (base++);
		                        std::string fnprefix = fnostr.str();

        		                listfilenames.push_back(fnprefix+".list");
        		                linkfilenames.push_back(fnprefix+".link");
        		                readfilenames.push_back(fnprefix+".read");

		                        numedges += edgelists[i]->writeNumEdgeFile(listfilenames.back());
		                        edgelists[i]->writeEdgeTargets(linkfilenames.back());
		                        edgelists[i]->writeEdgeWeights(readfilenames.back());
		                }

		                return numedges;
		        }
		};
	}
}
#endif
