/**
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
**/

#if ! defined(LCP_HPP)
#define LCP_HPP

#include <libmaus/autoarray/AutoArray.hpp>

#if defined(_OPENMP)
#include <omp.h>
#endif

#include <libmaus/fm/SampledSA.hpp>
#include <libmaus/fm/SampledISA.hpp>
#include <libmaus/lf/LF.hpp>
#include <libmaus/timing/RealTimeClock.hpp>
#include <libmaus/bitio/CompactQueue.hpp>
#include <libmaus/util/NumberSerialisation.hpp>

namespace libmaus
{
	namespace lcp
	{
		template<typename _lf_type, typename _sampled_sa_type, typename _sampled_isa_type>
		struct SuccinctLCP
		{
			typedef _lf_type lf_type;
			typedef _sampled_sa_type sampled_sa_type;
			typedef _sampled_isa_type sampled_isa_type;
			typedef SuccinctLCP<lf_type,sampled_sa_type,sampled_isa_type> this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
		
			static unsigned int bitsPerNum(uint64_t l)
			{
				unsigned int c = 0;
				
				while ( l )
				{
					c++;
					l>>=1;
				}
				
				return c;
			}

			static inline uint64_t fm(
				uint64_t const pos,
				lf_type const & lf,
				sampled_isa_type const & ISA,
				uint64_t const n
				)
			{
				return lf[ISA[(pos+1)%n]];
			}

			template<typename key_iterator, typename elem_type>
			static void computeLCP(key_iterator y, size_t const n, elem_type const * const p, elem_type * LCP)
			{
				::libmaus::autoarray::AutoArray<elem_type> AR(n,false); elem_type * R = AR.get();
				
				for ( elem_type i = 0; i < n; ++i ) R[p[i]] = i;

				elem_type l = 0;

				for ( elem_type j = 0; j < n; ++j )
				{
					if ( l > 0 ) l = l-1;
					elem_type i = R[j];
		    
					if ( i != 0 )
					{
						elem_type jp = p[i-1];
				    
						if ( jp > j )
							while ( (jp+l<n) && (y[j+l]==y[jp+l]) ) l++;
						else
							while ( (j+l<n)  && (y[j+l]==y[jp+l]) ) l++;
					}
					else
					{
						l = 0;
					}
					LCP[i] = l;
				}        
			}

			static ::libmaus::autoarray::AutoArray<uint64_t> computeLCP(
				lf_type const & lf,
				sampled_sa_type const & SA, 
				sampled_isa_type const & ISA,
				uint64_t const n)
			{
				double const bef = clock();
				uint64_t const mask = (1ull << 20)-1;

				::libmaus::autoarray::AutoArray<uint64_t> AS( (2*n+63)/64);
				uint64_t * const S = AS.get();
				for ( uint64_t i = 0; i < (2*n+63)/64; ++i )
					S[i] = 0;

				uint64_t l0 = 0;	
				uint64_t r0 = ISA[0]; // r0 = ISA[j]
				uint64_t ri = r0;
				uint64_t l1 = 0; // l1 = LCP[ ISA[i-1] ]
				uint64_t lcpbits = 0;
				// iterate over positions
				for ( uint64_t i = 0; i < n; ++i )
				{
					if ( l0 > 0 )
						l0 = l0-1;
					else
						ri = lf.phi(ri);

					if ( r0 != 0 )
					{
						// get position for rank r0-1
						uint64_t ip = SA[r0-1];	
						uint64_t rip = ISA[(ip+l0+1)%n];

						while ( (i+l0<n) && (ip+l0<n) && (lf[ri] == lf[rip]) )
						{
							l0++;
							ri = lf.phi(ri);
							rip = lf.phi(rip);						
						}
					}
					else
					{
						l0 = 0;
					}
					
					uint64_t const I = l0+1-l1;
					bitio::putBit(S,lcpbits+I,1);
					lcpbits += (I+1);

					r0 = lf.phi(r0);
					l1 = l0;

					if ( !(i & mask) )
						std::cerr << "\r                                         \r" <<
							static_cast<double>(i)/n << std::flush;
				}

				std::cerr << "\r                                         \r" << 1 << std::endl;
				
				double const aft = clock();
				std::cerr << "lcpbits = " << lcpbits << " frac " << static_cast<double>(lcpbits)/n << " comptime " << (aft-bef)/CLOCKS_PER_SEC << std::endl;
				
				return AS;
			}

			template<typename text_type>
			static ::libmaus::autoarray::AutoArray<uint64_t> computeLCPText(
				lf_type const & lf,
				sampled_sa_type const & SA, 
				sampled_isa_type const & ISA,
				uint64_t const n,
				text_type const & text
			)
			{
				::libmaus::timing::RealTimeClock rtc; rtc.start();

				::libmaus::autoarray::AutoArray<uint64_t> AS( (2*n+63)/64);
				uint64_t * const S = AS.get();
				for ( uint64_t i = 0; i < (2*n+63)/64; ++i )
					S[i] = 0;

		#if defined(_OPENMP)
				uint64_t const subblocksize = 64*1024;
				uint64_t const superblocksize = subblocksize * omp_get_max_threads();
				uint64_t const numsubblocks = (superblocksize + (subblocksize-1)) / subblocksize;
		#else
				uint64_t const subblocksize = 64*1024;
				uint64_t const superblocksize = subblocksize * 1;
				uint64_t const numsubblocks = (superblocksize + (subblocksize-1)) / subblocksize;
		#endif

				::libmaus::autoarray::AutoArray<uint64_t> R0(superblocksize,false);

				uint64_t const numsuperblocks = (n + (superblocksize-1))/superblocksize;
				
				uint64_t l0 = 0, l1 = 0, lcpbits = 0;

				for ( uint64_t superblock = 0; superblock < numsuperblocks; ++superblock )
				{
					uint64_t const superlow = superblock * superblocksize;
					uint64_t const superhigh = std::min(n,superlow+superblocksize);
					// uint64_t const superwidth = superhigh-superlow;

		#if defined(_OPENMP)
		#pragma omp parallel for
		#endif			
					for ( int64_t subblock = 0; subblock < static_cast<int64_t>(numsubblocks); ++subblock )
					{
						uint64_t const sublow = std::min(superlow + subblock * subblocksize,superhigh);
						uint64_t const subhigh = std::min(sublow + subblocksize,superhigh);
						uint64_t const subwidth = subhigh-sublow;
						
						// std::cerr << "sublow=" << sublow << " subhigh=" << subhigh << " subwidth=" << subwidth << std::endl;
						
						uint64_t r = ISA[(subhigh % n)];
						uint64_t t = subblock * subblocksize + subwidth;

						for ( uint64_t i = 0; i < subwidth; ++i )
						{
							r = lf(r);
							// std::cerr << "t=" << t << std::endl;
							if ( r )
								R0[--t] = SA[r-1];
							else
								R0[--t] = n;
						}
							
					}
				
					for ( uint64_t i = superlow; i < superhigh; ++i )
					{
						if ( l0 > 0 )
							l0 = l0-1;

						uint64_t ip = R0[i-superlow];
						
						if ( ip != n )
						{
							while ( (i+l0<n) && (ip+l0<n) && (text[i+l0] == text[ip+l0]) )
								l0++;
						}
						else
						{
							l0 = 0;
						}
						
						uint64_t const I = l0+1-l1;
						bitio::putBit(S,lcpbits+I,1);
						lcpbits += (I+1);

						l1 = l0;			
					} 

					std::cerr << "\r                                         \r" << static_cast<double>(superhigh) / n << std::flush;
				}		
				std::cerr << "\r                                         \r" << 1 << std::endl;
				
				std::cerr << "lcpbits = " << lcpbits << " frac " << static_cast<double>(lcpbits)/n << " time " << rtc.getElapsedSeconds() << std::endl;
				
				return AS;
			}

			typedef ::libmaus::rank::ERank222B select_type;
			
			uint64_t n;
			sampled_sa_type const * SA;
			::libmaus::autoarray::AutoArray<uint64_t> ALCP;
			uint64_t const * LCP;
			select_type::unique_ptr_type eselect;

			uint64_t serialize(std::ostream & out)
			{
				uint64_t s = 0;
				s += ::libmaus::serialize::Serialize<uint64_t>::serialize(out,n);
				s += ALCP.serialize(out);
				return s;
			}
			
			uint64_t deserialize(std::istream & in)
			{
				uint64_t s = 0;
				s += ::libmaus::serialize::Serialize<uint64_t>::deserialize(in,&n);
				s += ALCP.deserialize(in);
				LCP = ALCP.get();
				eselect = UNIQUE_PTR_MOVE(select_type::unique_ptr_type( new select_type( LCP, ((2*n+63)/64)*64 ) ));
				std::cerr << "LCP: " << s << " bytes = " << s*8 << " bits" << " = " << (s+(1024*1024-1)) / (1024*1024) << " mb" << std::endl;
				return s;
			}
			
			SuccinctLCP(lf_type const & lf, sampled_sa_type const & rsa, sampled_isa_type const & isa)
			: n(lf.getN()), SA(&rsa), ALCP(computeLCP(lf,rsa,isa,n)), LCP(ALCP.get()),
			  eselect( new select_type( LCP, ((2*n+63)/64)*64 ) )
			{}

			template<typename text_type>
			SuccinctLCP(lf_type const & lf, sampled_sa_type const & rsa, sampled_isa_type const & isa, text_type const & text)
			: n(lf.getN()), SA(&rsa), ALCP(computeLCPText(lf,rsa,isa,n,text)), LCP(ALCP.get()),
			  eselect( new select_type( LCP, ((2*n+63)/64)*64 ) )
			{}
			
			SuccinctLCP(std::istream & in, sampled_sa_type const & rsa)
			: SA(&rsa)
			{
				deserialize(in);
			}
			SuccinctLCP(std::istream & in, sampled_sa_type const & rsa, uint64_t & s)
			: SA(&rsa)
			{
				s += deserialize(in);
			}

			uint64_t operator[](uint64_t i) const
			{
				uint64_t const sa = (*SA)[i];
				return eselect->select1(sa) - (sa<<1) - 1;
			}
		};

		// compute plcp	array
		template<typename key_type, typename elem_type>
		::libmaus::autoarray::AutoArray<elem_type> plcp(key_type const * t, size_t const n, elem_type const * const SA)
		{
			// compute Phi
			::libmaus::autoarray::AutoArray<elem_type> APhi(n,false); elem_type * Phi = APhi.get();
			if ( n )
				Phi[SA[0]] = n;
			for ( size_t i = 1; i < n; ++i )
				Phi[SA[i]] = SA[i-1];
			  
			unsigned int l = 0;
			  
			::libmaus::autoarray::AutoArray<elem_type> APLCP(n,false); elem_type * PLCP = APLCP.get();
			key_type const * const tn = t+n;
			for ( size_t i = 0; i < n; ++i )
			{
				unsigned int const j = Phi[i];
			    
				key_type const * ti = t+i+l;
				key_type const * tj = t+j+l;
			    
				if ( j < i ) while ( (ti != tn) && (*ti == *tj) ) ++l, ++ti, ++tj;
				else         while ( (tj != tn) && (*ti == *tj) ) ++l, ++ti, ++tj;
			    
				PLCP[i] = l;
				if ( l >= 1 )
					l -= 1;
				else
					l = 0;
			}
			  
			return APLCP;
		}

		// compute lcp array using plcp array
		template<typename key_type, typename elem_type>
		::libmaus::autoarray::AutoArray<elem_type> computeLcp(
			key_type const * t, 
			size_t const n, 
			elem_type const * const SA
		)
		{
			::libmaus::autoarray::AutoArray<elem_type> APLCP = plcp(t,n,SA); elem_type const * const PLCP = APLCP.get();
			::libmaus::autoarray::AutoArray<elem_type> ALCP(n,false); elem_type * LCP = ALCP.get();
			  
			for ( size_t i = 0; i < n; ++i )
				LCP[i] = PLCP[SA[i]];
			  
			return ALCP;
		}

		template<typename key_string, typename sa_elem_type, typename lcp_elem_type>
		void computeLCPKasai(key_string const & y, uint64_t const n, sa_elem_type const * const sa, lcp_elem_type * lcp)
		{
			::libmaus::autoarray::AutoArray<sa_elem_type> isa(n);

			for ( uint64_t i = 0; i < n; ++i ) isa[sa[i]] = i;
		
			lcp_elem_type l = 0;
		
			for ( uint64_t j = 0; j < n; ++j )
			{
				if ( l > 0 ) l = l-1;
				sa_elem_type const i = isa[j];
			
				if ( i != 0 )
				{
					sa_elem_type jp = sa[i-1];
					
					if ( static_cast<sa_elem_type>(j) < jp )
						while ( (jp+l<static_cast<sa_elem_type>(n)) && (y[j+l]==y[jp+l]) )
							l++;
					else
						while ( (j+l<n) && (y[j+l]==y[jp+l]) )
							l++;
				}
				else
				{
					l = 0;
				}
				lcp[i] = l;
			}        
		}
		
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

			struct LCPResult
			{
				typedef LCPResult this_type;
				typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			
				typedef uint8_t small_elem_type;
				::libmaus::autoarray::AutoArray<uint8_t>::unique_ptr_type WLCP;
				::libmaus::autoarray::AutoArray<uint64_t>::unique_ptr_type U;
				::libmaus::rank::ERank222B::unique_ptr_type Urank;
				::libmaus::autoarray::AutoArray<uint64_t>::unique_ptr_type LLCP;

				void serialise(std::ostream & out) const
				{
					WLCP->serialize(out);
					::libmaus::util::NumberSerialisation::serialiseNumber(out,U.get()!=0);
					if ( U.get() )
					{
						U->serialize(out);
						LLCP->serialize(out);
					}
				}
				
				static unique_ptr_type load(std::string const & filename)
				{
					std::ifstream istr(filename.c_str(),std::ios::binary);
					
					if ( ! istr.is_open() )
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "Failed to open file " << filename << " in LCPResult::load()" << std::endl;
						se.finish();
						throw se;
					}
					
					unique_ptr_type P ( new this_type(istr) );
					
					if ( ! istr )
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "Failed to load file " << filename << " in LCPResult::load()" << std::endl;
						se.finish();
						throw se;					
					}
					
					return UNIQUE_PTR_MOVE(P);
				}
				
				LCPResult(std::istream & in)
				: WLCP(::libmaus::autoarray::AutoArray<uint8_t>::unique_ptr_type(new ::libmaus::autoarray::AutoArray<uint8_t>(in)))
				{
					bool const haveU = ::libmaus::util::NumberSerialisation::deserialiseNumber(in);
					if ( haveU )
					{
						U = UNIQUE_PTR_MOVE(::libmaus::autoarray::AutoArray<uint64_t>::unique_ptr_type(new ::libmaus::autoarray::AutoArray<uint64_t>(in)));
						LLCP = UNIQUE_PTR_MOVE(::libmaus::autoarray::AutoArray<uint64_t>::unique_ptr_type(new ::libmaus::autoarray::AutoArray<uint64_t>(in)));
						uint64_t const n = WLCP->size()-1;
						uint64_t const n64 = (n+63)/64;
						Urank = UNIQUE_PTR_MOVE(::libmaus::rank::ERank222B::unique_ptr_type(new ::libmaus::rank::ERank222B(U->get(),n64*64)));
					}
				}
				
				LCPResult(uint64_t const n)
				: WLCP(::libmaus::autoarray::AutoArray<uint8_t>::unique_ptr_type(new ::libmaus::autoarray::AutoArray<uint8_t>(n+1)))
				{
				
				}
				
				bool isUnset(uint64_t i) const
				{
					if ( (*WLCP)[i] != std::numeric_limits<small_elem_type>::max() )
						return false;
					else if ( !(U.get()) )
						return true;
					else
					{
						assert ( ::libmaus::bitio::getBit(U->get(),i) );
						return (*LLCP) [ Urank->rank1(i)-1 ] == std::numeric_limits<uint64_t>::max();
					}
				}
				
				void set(uint64_t const i, uint64_t const v)
				{
					assert ( (*WLCP)[i] == std::numeric_limits<small_elem_type>::max() ) ;
					assert ( U->get() );
					assert ( ::libmaus::bitio::getBit(U->get(),i) );
					(*LLCP) [ Urank->rank1(i)-1 ] = v;
				}
				
				uint64_t operator[](uint64_t const i) const
				{
					if ( ! U )
						return (*WLCP)[i];
					else if ( ! ::libmaus::bitio::getBit(U->get(),i) )
						return (*WLCP)[i];
					else
						return (*LLCP) [ Urank->rank1(i)-1 ];
				}
				
				void setupLargeValueVector(uint64_t const n, small_elem_type const unset)
				{
					// set up large value bit vector
					uint64_t const n64 = (n+63)/64;
					this->U = UNIQUE_PTR_MOVE(::libmaus::autoarray::AutoArray<uint64_t>::unique_ptr_type(new ::libmaus::autoarray::AutoArray<uint64_t>(n64)));
					for ( uint64_t i = 0; i < n; ++i )
						if ( (*WLCP)[i] == unset )
							::libmaus::bitio::putBit(this->U->get(),i,1);
					// set up rank dictionary for large value bit vector
					this->Urank = UNIQUE_PTR_MOVE(::libmaus::rank::ERank222B::unique_ptr_type(new ::libmaus::rank::ERank222B(this->U->get(),n64*64)));
					// set up array for large values
					this->LLCP = UNIQUE_PTR_MOVE(::libmaus::autoarray::AutoArray<uint64_t>::unique_ptr_type(new ::libmaus::autoarray::AutoArray<uint64_t>(this->Urank->rank1(n-1))));
					// mark all large values as unset
					for ( uint64_t i = 0; i < this->LLCP->size(); ++i )
						(*(this->LLCP))[i] = std::numeric_limits<uint64_t>::max();				
				}
			};

			template<typename lf_type>
			static LCPResult::unique_ptr_type computeLCP(lf_type const * LF, bool const zdif = true)
			{
				uint64_t const n = LF->getN();
				LCPResult::small_elem_type const unset = std::numeric_limits< LCPResult::small_elem_type>::max();
				LCPResult::unique_ptr_type res(new LCPResult(n));

				::libmaus::autoarray::AutoArray< LCPResult::small_elem_type> & WLCP = *(res->WLCP);

				std::stack < TraversalNode > st;
				st.push( TraversalNode(0,n) );
				typedef PrintMultiCallback<lf_type> print_callback_type;
				PrintMultiCallback<lf_type> PMC(*LF,st);

				while ( ! st.empty() )
				{
					TraversalNode tn = st.top(); st.pop();
					typedef typename lf_type::wt_type wt_type;
					wt_type const & W = *(LF->W.get());
					W.multiRankCallBack(tn.sp,tn.ep,LF->D.get(),PMC);
				}

				::libmaus::autoarray::AutoArray<uint64_t> symfreq( LF->getSymbolThres() );
				
				std::cerr << "[V] symbol threshold is " << symfreq.size() << std::endl;
				
				// symbol frequencies			
				for ( uint64_t i = 0; i < symfreq.getN(); ++i )
					symfreq[i] = n?LF->W->rank(i,n-1):0;

				std::fill ( WLCP.get(), WLCP.get()+WLCP.getN(),unset);
							
				::libmaus::suffixsort::CompactQueue Q0(n);
				::libmaus::suffixsort::CompactQueue Q1(n);
				::libmaus::suffixsort::CompactQueue * PQ0 = &Q0;
				::libmaus::suffixsort::CompactQueue * PQ1 = &Q1;

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

				::libmaus::timing::RealTimeClock lcprtc; lcprtc.start();
				std::cerr << "Computing LCP...";
				uint64_t cur_l = 1;
				while ( PQ0->fill && cur_l < unset )
				{
					std::cerr << "cur_l=" << static_cast<uint64_t>(cur_l) << " fill=" << PQ0->fill << " set=" << s << " (" << static_cast<double>(s)/LF->getN() << ")" << std::endl;

					PQ1->reset();

					#if defined(_OPENMP) && defined(LIBMAUS_HAVE_SYNC_OPS)
					uint64_t const numthreads = omp_get_max_threads();
					#else
					uint64_t const numthreads = 1;
					#endif
					
					uint64_t const numcontexts = numthreads;
					::libmaus::autoarray::AutoArray < ::libmaus::suffixsort::CompactQueue::DequeContext::unique_ptr_type > deqcontexts = PQ0->getContextList(numcontexts);

					#if defined(_OPENMP) && defined(LIBMAUS_HAVE_SYNC_OPS)
					#pragma omp parallel for
					#endif
					for ( int64_t c = 0; c < static_cast<int64_t>(deqcontexts.size()); ++c )
					{
						::libmaus::suffixsort::CompactQueue::DequeContext * deqcontext = deqcontexts[c].get();
						::libmaus::suffixsort::CompactQueue::EnqueBuffer::unique_ptr_type encbuf = PQ1->createEnqueBuffer();

						while ( !deqcontext->done() )
						{
							std::pair<uint64_t,uint64_t> const qe = PQ0->deque(deqcontext);
							uint64_t const locals = LF->W->multiRankLCPSet(qe.first,qe.second,LF->D.get(),WLCP.get(),unset,cur_l,encbuf.get());
							#if defined(_OPENMP) && defined(LIBMAUS_HAVE_SYNC_OPS)
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
					::libmaus::suffixsort::CompactQueue::DequeContext::unique_ptr_type dcontext = PQ0->getGlobalDequeContext();
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
							#if defined(_OPENMP) && defined(LIBMAUS_HAVE_SYNC_OPS)
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
