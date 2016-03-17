/*
    libmaus2
    Copyright (C) 2009-2015 German Tischler
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
#if ! defined(LIBMAUS2_LCP_WAVELETLCP_HPP)
#define LIBMAUS2_LCP_WAVELETLCP_HPP

#include <libmaus2/bitio/CompactQueue.hpp>
#include <libmaus2/lcp/WaveletLCPResult.hpp>
#include <libmaus2/timing/RealTimeClock.hpp>
#include <stack>

#if defined(_OPENMP)
#include <omp.h>
#endif

namespace libmaus2
{
	namespace lcp
	{
		/**
		 * LCP array construction algorithm as described by Beller et al
		 **/
		struct WaveletLCP
		{
			struct TraversalNode
			{
				uint64_t sp;
				uint64_t ep;

				TraversalNode() : sp(0), ep(0) {}
				TraversalNode(uint64_t const rsp, uint64_t const rep) : sp(rsp), ep(rep) {}
			};

			template<typename lf_type>
			struct PrintMultiCallback
			{
				lf_type const & lf;
				std::stack < TraversalNode > & S;

				PrintMultiCallback(lf_type const & rlf, std::stack<TraversalNode> & rS)
				: lf(rlf), S(rS)
				{

				}

				void operator()(uint64_t const sym, uint64_t const sp, uint64_t const ep)
				{
					std::cerr << "sym=" << sym << " sp=" << sp << " ep=" << ep << " B=" << std::endl;
				}
			};


			template<typename lf_type>
			static WaveletLCPResult::unique_ptr_type computeLCP(
				lf_type const * LF,
				uint64_t const rnumthreads,
				bool const zdif /* = true */)
			{
				uint64_t const n = LF->getN();
				WaveletLCPResult::small_elem_type const unset = std::numeric_limits< WaveletLCPResult::small_elem_type>::max();
				WaveletLCPResult::unique_ptr_type res(new WaveletLCPResult(n));

				::libmaus2::autoarray::AutoArray< WaveletLCPResult::small_elem_type> & WLCP = *(res->WLCP);

				std::stack < TraversalNode > st;
				st.push( TraversalNode(0,n) );
				// typedef PrintMultiCallback<lf_type> print_callback_type;
				PrintMultiCallback<lf_type> PMC(*LF,st);

				while ( ! st.empty() )
				{
					TraversalNode tn = st.top(); st.pop();
					typedef typename lf_type::wt_type wt_type;
					wt_type const & W = *(LF->W.get());
					W.multiRankCallBack(tn.sp,tn.ep,LF->D.get(),PMC);
				}

				::libmaus2::autoarray::AutoArray<uint64_t> symfreq( LF->getSymbolThres() );

				std::cerr << "[V] symbol threshold is " << symfreq.size() << std::endl;

				// symbol frequencies
				for ( uint64_t i = 0; i < symfreq.getN(); ++i )
					symfreq[i] = n?LF->W->rank(i,n-1):0;

				std::fill ( WLCP.get(), WLCP.get()+WLCP.getN(),unset);

				::libmaus2::suffixsort::CompactQueue Q0(n);
				::libmaus2::suffixsort::CompactQueue Q1(n);
				::libmaus2::suffixsort::CompactQueue * PQ0 = &Q0;
				::libmaus2::suffixsort::CompactQueue * PQ1 = &Q1;

				uint64_t s = 0;
				uint64_t cc = 0;
				uint64_t acc = 0;

				if ( zdif )
				{
					// special handling of zero symbols
					for ( uint64_t zc = 0; zc < symfreq[0]; ++zc )
					{
						WLCP[zc] = 0;
						Q0.enque(zc,zc+1);
					}
					s += symfreq[cc++];
					acc += symfreq[0];
				}

				// other symbols
				for ( ; cc < symfreq.getN(); ++cc )
				{
					WLCP[acc] = 0;
					s++;

					if ( symfreq[cc] )
					{
						Q0.enque(acc,acc+symfreq[cc]);
						acc += symfreq[cc];
					}
				}
				WLCP[n] = 0;

				::libmaus2::timing::RealTimeClock lcprtc; lcprtc.start();
				std::cerr << "Computing LCP...";
				uint64_t cur_l = 1;
				while ( PQ0->fill && cur_l < unset )
				{
					std::cerr << "cur_l=" << static_cast<uint64_t>(cur_l) << " fill=" << PQ0->fill << " set=" << s << " (" << static_cast<double>(s)/LF->getN() << ")" << std::endl;

					PQ1->reset();

					#if defined(LIBMAUS2_HAVE_SYNC_OPS)
					uint64_t const numthreads = rnumthreads;
					#else
					uint64_t const numthreads = 1;
					#endif

					uint64_t const numcontexts = numthreads;
					::libmaus2::autoarray::AutoArray < ::libmaus2::suffixsort::CompactQueue::DequeContext::unique_ptr_type > deqcontexts = PQ0->getContextList(numcontexts,numthreads);

					#if defined(_OPENMP) && defined(LIBMAUS2_HAVE_SYNC_OPS)
					#pragma omp parallel for num_threads(numthreads)
					#endif
					for ( int64_t c = 0; c < static_cast<int64_t>(deqcontexts.size()); ++c )
					{
						::libmaus2::suffixsort::CompactQueue::DequeContext * deqcontext = deqcontexts[c].get();
						::libmaus2::suffixsort::CompactQueue::EnqueBuffer::unique_ptr_type encbuf = PQ1->createEnqueBuffer();

						while ( !deqcontext->done() )
						{
							std::pair<uint64_t,uint64_t> const qe = PQ0->deque(deqcontext);
							uint64_t const locals = LF->W->multiRankLCPSet(qe.first,qe.second,LF->D.get(),WLCP.get(),unset,cur_l,encbuf.get());
							#if defined(_OPENMP) && defined(LIBMAUS2_HAVE_SYNC_OPS)
							__sync_fetch_and_add(&s,locals);
							#else
							s += locals;
							#endif
						}

						assert ( deqcontext->fill == 0 );
					}
					std::swap(PQ0,PQ1);

					cur_l ++;
				}
				std::cerr << "done, time " << lcprtc.getElapsedSeconds() << std::endl;

				if ( PQ0->fill )
				{
					// cur_l should now be the largest value for the small type
					assert ( cur_l == unset );

					// extract compact queues into non compact ones
					std::deque< std::pair<uint64_t,uint64_t> > Q0, Q1;
					::libmaus2::suffixsort::CompactQueue::DequeContext::unique_ptr_type dcontext = PQ0->getGlobalDequeContext();
					while ( dcontext->fill )
						Q0.push_back( PQ0->deque(dcontext.get()) );

					// prepare result for storing "large" values
					res->setupLargeValueVector(n, unset);

					uint64_t prefill = 0;

					while ( Q0.size() )
					{
						if ( Q0.size() != prefill )
						{
							std::cerr << "cur_l=" << static_cast<uint64_t>(cur_l) << " fill=" << Q0.size() << " set=" << s << " (" << static_cast<double>(s)/LF->getN() << ")" << std::endl;
							prefill = Q0.size();
						}

						assert ( Q1.size() == 0 );

						while ( Q0.size() )
						{
							std::pair<uint64_t,uint64_t> const qe = Q0.front(); Q0.pop_front();
							uint64_t const locals = LF->W->multiRankLCPSetLarge(qe.first,qe.second,LF->D.get(),*res,cur_l,&Q1);
							#if defined(_OPENMP) && defined(LIBMAUS2_HAVE_SYNC_OPS)
							__sync_fetch_and_add(&s,locals);
							#else
							s += locals;
							#endif
						}

						assert ( ! Q0.size() );
						Q0.swap(Q1);

						cur_l ++;
					}

				}

				return UNIQUE_PTR_MOVE(res);
			}
		};


	}
}
#endif
