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

#if ! defined(UNIQUEINDEX_HPP)
#define UNIQUEINDEX_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/suffixsort/divsufsort.hpp>
#include <libmaus2/lf/LF.hpp>
#include <libmaus2/util/Array832.hpp>
#include <libmaus2/util/NegativeDifferenceArray.hpp>
#include <libmaus2/util/PositiveDifferenceArray.hpp>
#include <libmaus2/util/ArgInfo.hpp>
#include <libmaus2/aio/SynchronousFastReaderBase.hpp>
#include <libmaus2/wavelet/ExternalWaveletGenerator.hpp>
#include <libmaus2/suffixsort/SkewSuffixSort.hpp>
#include <libmaus2/bitio/CompactQueue.hpp>
#include <libmaus2/bitio/SignedCompactArray.hpp>
#include <libmaus2/timing/RealTimeClock.hpp>

namespace libmaus2
{
	namespace util
	{
		struct UniqueIndex2
		{
			typedef UniqueIndex2 this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
		
			typedef uint8_t char_type;
			#if 0
			enum { ALPHABET_SIZE=256 };
			typedef ::libmaus2::suffixsort::DivSufSort<64,char_type *,char_type const *,int64_t *,int64_t const *,ALPHABET_SIZE> sort_type;
			typedef ::libmaus2::suffixsort::DivSufSortUtils<64,char_type *,char_type const *,int64_t *,int64_t const *,ALPHABET_SIZE> sort_util_type;
			typedef ::libmaus2::suffixsort::DivSufSortUtils<64,char_type *,char_type const *,uint32_t *,uint32_t const *,ALPHABET_SIZE> sort_util_post_type;
			typedef sort_type::saidx_t saidx_t;
			#endif
			static uint64_t const sasamplingrate = 1024*1024;

			typedef ::libmaus2::bitio::CompactArray text_array_type;
			typedef text_array_type::unique_ptr_type text_array_ptr_type;
			typedef ::libmaus2::bitio::CompactArray::const_iterator compact_const_it;
			typedef ::libmaus2::bitio::CompactArray::iterator compact_it;
			typedef ::libmaus2::rank::ERank222B rank_type;			
			typedef rank_type::writer_type rank_writer_type;
			typedef rank_writer_type::data_type rank_data_type;
			typedef rank_type::unique_ptr_type rank_ptr_type;
			typedef ::libmaus2::wavelet::QuickWaveletTree<rank_type,uint64_t> wt_type;
			typedef wt_type::unique_ptr_type wt_ptr_type;
			typedef ::libmaus2::lf::QuickLF lf_type;
			typedef lf_type::unique_ptr_type lf_ptr_type;

			text_array_ptr_type data;
			uint64_t n;
			lf_ptr_type lf;
			::libmaus2::util::Array864::unique_ptr_type LCP;
			::libmaus2::util::PositiveDifferenceArray64::unique_ptr_type next;
			::libmaus2::util::NegativeDifferenceArray64::unique_ptr_type prev;
			::libmaus2::autoarray::AutoArray<rank_data_type> R;
			rank_ptr_type dolrank;
			::libmaus2::autoarray::AutoArray<uint64_t> SISA;
			::libmaus2::autoarray::AutoArray<uint64_t> U;

			static char_type fw(char_type const v)
			{
				switch ( v )
				{
					case '#': return 0;
					case 'A': return 1;
					case 'C': return 2;
					case 'G': return 3;
					case 'T': return 4;
					case 'N': return 5;
					case '$': return 6;
					default: assert ( false );
				}
			}

			static uint8_t rv(char_type const v)
			{
				switch ( v )
				{
					case 0: return '#';
					case 1: return 'A';
					case 2: return 'C';
					case 3: return 'G';
					case 4: return 'T';
					case 5: return 'N';
					default: return '$';
				}
			}

			static uint64_t rc(uint64_t const c)
			{
				switch (c)
				{
					case 1: return 4; // A->T
					case 2: return 3; // C->G
					case 3: return 2; // G->C
					case 4: return 1; // T->A
					default: return c;
				}
			}

			static std::pair<uint64_t,uint64_t> countChars(std::vector<std::string> const & filenames)
			{
				::libmaus2::aio::SynchronousFastReaderBase SFR(filenames);
				std::string line;
				
				uint64_t numc = 0;
				uint64_t numsep = 0;
				bool lastwasdata = true;
				bool first = true;
				
				while ( SFR.getLine(line) )
					if ( line.size() )
					{
						if ( line[0] != '>' )
						{
							if ( !lastwasdata && ! first )
							{
								numc++; // count separator
								numsep++;
							}
							for ( uint64_t i = 0; i < line.size(); ++i )
								if ( ! isspace(line[i]) )
									numc++;

							lastwasdata = true;
							first = false;
						}
						else
						{
							lastwasdata = false;
						}
					}
				
				// numc += 1;

				return std::pair<uint64_t,uint64_t>(numc,numsep);
			}


			static text_array_ptr_type readFiles(std::vector<std::string> const & filenames, uint64_t const numc, uint64_t const numsep)
			{
				uint64_t const alphsize = (2*numsep+1) /* separators between reads */ + 5 /* ACGTN */ + 1 /* 0 terminator */;
				uint64_t const albits = ::libmaus2::math::bitsPerNum(alphsize-1);
			
				text_array_ptr_type A = text_array_ptr_type(new text_array_type(2*numc+2,albits));
				::libmaus2::aio::SynchronousFastReaderBase SFR(filenames);
				std::string line;
				
				compact_it p(A.get());
				compact_it pa(A.get());
				compact_it pr(A.get()); pr += numc;

				char_type sep = 6;
				
				bool lastwasdata = true;
				bool first = true;
				
				while ( SFR.getLine(line) )
					if ( line.size() )
					{
						if ( line[0] != '>' )
						{
							if ( !lastwasdata && (!first) )
							{
								*(p++) = (sep++);
								*(pr++) = (sep++);
								
								compact_it pt = p-1;
								do {
									*(pr++) = rc(*(--pt));
								} while ( pt != pa );
								
								pa = p;
							}
							for ( uint64_t i = 0; i < line.size(); ++i )
								if ( ! isspace(line[i]) )
								{
									switch ( toupper ( line[i] ) )
									{
										case 'A': case 'C':
										case 'G': case 'T':
											*(p++) = fw(toupper(line[i])); break;
										default:
											*(p++) = fw('N'); break;
									}
								}
							
							lastwasdata = true;
							first = false;
						}
						else
						{
							lastwasdata = false;
						}
					}

				*(p++) = (sep++);
				*(pr++) = (sep++);
								
				compact_it pt = p-1;
				do {
					*(pr++) = rc(*(--pt));
				} while ( pt != pa );
				
				*(pr++) = 0;
					
				assert ( pr.i == A->n );

				#if 0
				for ( compact_it i(A.get()); i.i != A->size(); ++i )
				{
					std::cerr << rv(*i);
					/*
					if ( rv(*i) == '$' )
						std::cerr << "(" << static_cast<int>(*i) << ")";
					*/
				}
				std::cerr << std::endl;
				#endif

				return A;
			}
			

			/**
			 * algorithm nsv864
			 **/
			template<typename lcp_array_type>
			static ::libmaus2::util::PositiveDifferenceArray64::unique_ptr_type nsv864(lcp_array_type const & LCP, int64_t const n)
			{
				// allocate result array
				::libmaus2::autoarray::AutoArray<uint64_t> next(n,false);

				// do not crash for n==0
				if ( n )
				{
					// initialize next for rank n-1
					next[n-1] = n;
				}
			  
				// compute next for the remaining ranks
				for ( int64_t r = static_cast<int64_t>(n)-2; r >= 0; --r )
				{
					int64_t t = r + 1;

					while ( (t < n) && (LCP[t] >= LCP[r]) )
						t = next[t];
					next[r] = t;
				}
			  
				for ( int64_t r = 0; r < n; ++r )
				{
					next[r] = next[r]-r;
				}

				::libmaus2::util::Array864::unique_ptr_type anext(new ::libmaus2::util::Array864(next.begin(),next.end()));
			  
				// return result
				return ::libmaus2::util::PositiveDifferenceArray64::unique_ptr_type(new ::libmaus2::util::PositiveDifferenceArray64(anext));
			}

			/**
			 * algorithm psv864
			 **/
			template<typename lcp_array_type>
			static ::libmaus2::util::NegativeDifferenceArray64::unique_ptr_type psv864(lcp_array_type const & LCP, uint64_t const n)
			{
				// allocate result array
				::libmaus2::autoarray::AutoArray<uint64_t> prev(n,false);

				// do not crash for n==0
				if ( n )
					prev[0] = 0;

				for ( uint64_t r = 1; r < n; ++r )
				{
					uint64_t t = r - 1;
			  
					while ( (t > 0) && (LCP[t] >= LCP[r]) )
						t = prev[t];

					prev[r] = t;
				}
			  
				for ( uint64_t r = 0; r < n; ++r )
					prev[r] = r-prev[r];
				
				::libmaus2::util::Array864::unique_ptr_type aprev(new ::libmaus2::util::Array864(prev.begin(),prev.end()));

				return ::libmaus2::util::NegativeDifferenceArray64::unique_ptr_type(new ::libmaus2::util::NegativeDifferenceArray64(aprev));
			}


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
			static ::libmaus2::util::Array864::unique_ptr_type computeLCP(lf_type const * LF)
			{
				typedef uint32_t lcp_elem_type;
				uint64_t const n = LF->getN();

				std::stack < TraversalNode > st;
				st.push( TraversalNode(0,n) );
				PrintMultiCallback<lf_type> PMC(*LF,st);

				while ( ! st.empty() )
				{
					TraversalNode tn = st.top(); st.pop();
					LF->W->multiRankCallBack(tn.sp,tn.ep,LF->D.get(),PMC);
				}

				::libmaus2::autoarray::AutoArray<uint64_t> symfreq( (1ull<< LF->getB()) );
							
				for ( uint64_t i = 0; i < symfreq.getN(); ++i )
					symfreq[i] = n?LF->W->rank(i,n-1):0;

				::libmaus2::autoarray::AutoArray<lcp_elem_type> WLCP(n+1,false);
				std::fill ( WLCP.get(), WLCP.get()+WLCP.getN(), std::numeric_limits<lcp_elem_type>::max() );
							
				::libmaus2::suffixsort::CompactQueue Q0(n);
				::libmaus2::suffixsort::CompactQueue Q1(n);
				::libmaus2::suffixsort::CompactQueue * PQ0 = &Q0;
				::libmaus2::suffixsort::CompactQueue * PQ1 = &Q1;

				for ( uint64_t i = 0; i < symfreq[0]; ++i )
				{
					WLCP[i] = 0;
					Q0.enque(i,i+1);
				}
							
				uint64_t acc = symfreq[0];
				for ( uint64_t i = 1; i < symfreq.getN(); ++i )
				{
					WLCP[acc] = 0;
					if ( symfreq[i] )
					{
						Q0.enque(acc,acc+symfreq[i]);
						acc += symfreq[i];
					}
				}                
				WLCP[n] = 0;

				::libmaus2::timing::RealTimeClock lcprtc; lcprtc.start();
				std::cerr << "Computing LCP...";
				lcp_elem_type cur_l = 1;
				lcp_elem_type const unset = std::numeric_limits<lcp_elem_type>::max();
				uint64_t const fillthres = 128*1024;
				bool seenabove = false;
				while ( PQ0->fill )
				{
					if ( PQ0->fill >= fillthres )
						seenabove = true;
				
					std::cerr << "cur_l=" << static_cast<uint64_t>(cur_l) << " fill=" << PQ0->fill << std::endl;

					PQ1->reset();

					#if defined(_OPENMP)
					uint64_t const numthreads = omp_get_max_threads();
					#else
					uint64_t const numthreads = 1;
					#endif
					
					uint64_t const numcontexts = numthreads;
					::libmaus2::autoarray::AutoArray < ::libmaus2::suffixsort::CompactQueue::DequeContext::unique_ptr_type > deqcontexts = PQ0->getContextList(numcontexts);

					#if defined(_OPENMP)
					#pragma omp parallel for
					#endif
					for ( int64_t c = 0; c < static_cast<int64_t>(deqcontexts.size()); ++c )
					{
					  ::libmaus2::suffixsort::CompactQueue::DequeContext * deqcontext = deqcontexts[c].get();
					  ::libmaus2::suffixsort::CompactQueue::EnqueBuffer::unique_ptr_type encbuf = PQ1->createEnqueBuffer();
					  
					  while ( !deqcontext->done() )
					  {
						  std::pair<uint64_t,uint64_t> const qe = PQ0->deque(deqcontext);
						  LF->W->multiRankLCPSet(qe.first,qe.second,LF->D.get(),WLCP.get(),unset,cur_l,encbuf.get());
					  }
					  
					  assert ( deqcontext->fill == 0 );
					}
					std::swap(PQ0,PQ1);
					
					cur_l ++;
					
					if ( seenabove && PQ0->fill < fillthres )
					{
						std::cerr << "Fill below threshold, switching to sparse queue." << std::endl;
						break;
					}
				}
				
				if ( PQ0->fill )
				{
					std::deque< std::pair<uint64_t,uint64_t> > SQ0, SQ1;
					::libmaus2::suffixsort::CompactQueue::DequeContext::unique_ptr_type pcontext = PQ0->getGlobalDequeContext();
					while ( !(pcontext->done()) )
						SQ0.push_back(PQ0->deque(pcontext.get()));
						
					uint64_t prefill = 0;
					while ( SQ0.size() )
					{
						if ( (cur_l & (128*1024-1)) == 0 || SQ0.size() != prefill )
							std::cerr << "cur_l=" << cur_l << " fill=" << SQ0.size() << std::endl;
						
						prefill = SQ0.size();
						
						while ( SQ0.size() )
						{
							std::pair<uint64_t,uint64_t> const qe = SQ0.front(); SQ0.pop_front();
							LF->W->multiRankLCPSet(qe.first,qe.second,LF->D.get(),WLCP.get(),unset,cur_l,&SQ1);
						}
						
						cur_l++;
						SQ0.swap(SQ1);
					}
				}
			     
				::libmaus2::util::Array864::unique_ptr_type LCP = ::libmaus2::util::Array864::unique_ptr_type(
					new ::libmaus2::util::Array864(WLCP.begin(),WLCP.end())
				);
				std::cerr << "done, time " << lcprtc.getElapsedSeconds() << std::endl;

				return LCP;
			}

			struct Node
			{
				uint64_t left;
				uint64_t right;
				uint64_t depth;
				
				Node() : left(0), right(0), depth(0) {}
				Node(uint64_t r) : left(r), right(r), depth(0) {}
				Node(uint64_t rleft, uint64_t rright, uint64_t rdepth)
				: left(rleft), right(rright), depth(rdepth) {}
				
				std::string toString() const
				{
					std::ostringstream ostr;
					ostr << "Node(" << left << "," << right << "," << depth << ")";
					return ostr.str();
				}
			};
			
			inline Node parent(Node const & I) const
			{
				if ( I.left == 0 && I.right == n-1 )
				{
					std::cerr << "Warning, called parent on root node, returning root." << std::endl;
					return Node(0,n-1,0);
				}
					
				uint64_t const k = ( (I.right+1 >= n) || ((*LCP)[I.left] > (*LCP)[I.right+1]) ) ? I.left : (I.right+1);
				// depth LCP[k]
				return Node ((*prev)[k],(*next)[k]-1, (*LCP)[k]);
			}
			
			uint64_t nextImproper(uint64_t const p) const
			{
				if ( ::libmaus2::bitio::getBit(R.get(),p) )
					return p;
				else
					return dolrank->select1 ( dolrank->rank1(p) );
			}

			struct ThreadLimit
			{
				uint64_t numthreads;
				
				ThreadLimit(uint64_t const setthreads)
				: 
					numthreads(
						#if defined(_OPENMP)
						omp_get_max_threads()
						#else
						1
						#endif
					)
				{
					#if defined(_OPENMP)
					omp_set_num_threads(setthreads);
					#endif
				}
				~ThreadLimit()
				{
					#if defined(_OPENMP)
					omp_set_num_threads(numthreads);
					#endif
				}
			};
			
			void readText(::libmaus2::util::ArgInfo const & arginfo, uint64_t & numc, uint64_t & numsep)
			{
				std::cerr << "Counting characters...";
				std::pair<uint64_t,uint64_t> NC = countChars(arginfo.restargs);
				numc = NC.first;
				numsep = NC.second;
				std::cerr << "done, numc="<<numc<<", numsep=" << numsep << std::endl;
				
				std::cerr << "Reading data...";
				data = readFiles(arginfo.restargs, numc, numsep);
				n = data->size();
				std::cerr << "done, got " << n << " symbols." << std::endl;
			}
			
			void readDolVec()
			{
				std::cerr << "Loading proper symbol vector...";
				libmaus2::aio::InputStreamInstance dolistr(dolvecname);
				R = libmaus2::autoarray::AutoArray<uint64_t>(dolistr);
				assert ( dolistr );
				dolistr.close();
				std::cerr << "done." << std::endl;				
			
				std::cerr << "Constructing rank dictionary for proper symbol vector...";
				dolrank = rank_ptr_type(new rank_type(R.get(),R.size()*sizeof(rank_data_type)*8));
				std::cerr << "done." << std::endl;
			}

			std::string const fbwtname;
			std::string const flcpfilename;
			std::string const fprevname;
			std::string const fnextname;
			std::string const dolvecname;
			std::string const fsisafilename;
			std::string const fufilename;

			UniqueIndex2(::libmaus2::util::ArgInfo const & arginfo)
			: fbwtname("unique2.bwt"), flcpfilename("unique2.lcp"), fprevname("unique2.prev"),
			  fnextname("unique2.next"), dolvecname("unique2.dolvec"), fsisafilename("unique2.sisa"),
			  fufilename("unique2.uni")
			{
				::libmaus2::util::TempFileNameGenerator tmpgen("tmp",2);


				if ( 
					(! ::libmaus2::util::GetFileSize::fileExists(fufilename))
				) 
				{
					if ( 
						(! ::libmaus2::util::GetFileSize::fileExists(fbwtname)) 
						||
						(! ::libmaus2::util::GetFileSize::fileExists(fsisafilename))
					)
					{
						std::string const fsafilename = "unique2.sa";

						uint64_t numc, numsep;
						readText(arginfo, numc, numsep);
									
						std::cerr << "Constucting proper symbol vector...";
						uint64_t rankbitsperword = 8*sizeof(rank_data_type);
						uint64_t rankdatawords = ((numc+1)+rankbitsperword-1)/rankbitsperword;
						R = ::libmaus2::autoarray::AutoArray<rank_data_type>(rankdatawords);
						rank_writer_type rwt(R.get());
						for ( uint64_t i = 0; i < numc+1; ++i )
							rwt.writeBit( data->get(i) < 1 || data->get(i) > 4 );
						rwt.flush();
						std::cerr << "done." << std::endl;
						
						std::cerr << "Writing proper symbol vector to disk...";
						{
						libmaus2::aio::OutputStreamInstance dolvecostr(dolvecname);
						R.serialize(dolvecostr);
						dolvecostr.flush();
						}
						std::cerr << "done." << std::endl;
						
						assert ( numc == n/2-1 );

						::libmaus2::bitio::SignedCompactArray::unique_ptr_type CSA;

						if ( ! ::libmaus2::util::GetFileSize::fileExists(fsafilename) )
						{
							CSA = ::libmaus2::bitio::SignedCompactArray::unique_ptr_type(new ::libmaus2::bitio::SignedCompactArray(n, ::libmaus2::math::bitsPerNum(n) + 1 ));
							typedef ::libmaus2::bitio::SignedCompactArray::const_iterator compact_index_const_it;
							typedef ::libmaus2::bitio::SignedCompactArray::iterator compact_index_it;
							unsigned int const bitwidth = 64;
							typedef ::libmaus2::suffixsort::DivSufSort<bitwidth,compact_it,compact_const_it,compact_index_it,compact_index_const_it> compact_index_sort_type;
							
							if ( (2*numsep+1)+5+1 > compact_index_sort_type::ALPHABET_SIZE )
							{
								::libmaus2::exception::LibMausException se;
								se.getStream() << "too many fragments: " << numsep;
								se.finish();
								throw se;
							}
							
							compact_const_it cci = data->begin();
							compact_index_it cii = CSA->begin();
							std::cerr << "Running divsufsort on double compact representation using " << CSA->getB() << " bits per position...";
							::libmaus2::timing::RealTimeClock rtc; rtc.start();
							{
								ThreadLimit TL(1); // no multi threading in here
								compact_index_sort_type::divsufsort(cci, cii, n);
							}
							std::cerr << "done, time " << rtc.getElapsedSeconds() << std::endl;

							std::cerr << "Writing SA to disk...";
							{
							libmaus2::aio::OutputStreamInstance ostr(fsafilename);
							CSA->serialize(ostr);
							ostr.flush();
							}
							std::cerr << "done." << std::endl;

							#if 0
							std::cerr << "Checking suffix array...";
							cci = compact_const_it(data.get());
							cii = compact_index_it(CSA.get());
							int r = ::libmaus2::suffixsort::DivSufSortUtils<
								bitwidth,compact_it,compact_const_it,compact_index_it,compact_index_const_it>::sufcheck(cci,cii,n,1);
							std::cerr << r << "," << "done." << std::endl;
							#endif
						
							#if 0	
							for ( uint64_t r = 0; r < n; ++r )
							{
								std::cerr << r << "\t" << CSA->get(r) << "\t";
								uint64_t p = CSA->get(r);
								for ( uint64_t i = p; i < n; ++i )
									std::cerr << rv(data->get(i));
								std::cerr << std::endl;
							}
							#endif
						}
						else
						{
							std::cerr << "Reading SA from disk...";
							libmaus2::aio::InputStreamInstance istr(fsafilename);
							CSA = ::libmaus2::bitio::SignedCompactArray::unique_ptr_type(new ::libmaus2::bitio::SignedCompactArray(istr));
							assert ( istr );
							istr.close();
							std::cerr << "done." << std::endl;
							assert ( CSA->size() == n );
						}
						
						if ( ! ::libmaus2::util::GetFileSize::fileExists(fsisafilename) )
						{
							std::cerr << "Computing sampled inverse suffix array...";
							SISA = ::libmaus2::autoarray::AutoArray<uint64_t>( (numc+1+sasamplingrate-1)/sasamplingrate,false );
							#if defined(_OPENMP)
							#pragma omp parallel for
							#endif
							for ( int64_t i = 0; i < static_cast<int64_t>(CSA->n); ++i )
								if ( CSA->get(i) % sasamplingrate == 0 )
									if ( CSA->get(i)/sasamplingrate < SISA.size() )
										SISA [ CSA->get(i)/sasamplingrate ] = i;
							std::cerr << "done." << std::endl;
							
							std::cerr << "Storing sampled inverse suffix array on disk...";
							{
							libmaus2::aio::OutputStreamInstance ostr(fsisafilename);
							SISA.serialize(ostr);
							ostr.flush();
							}
							std::cerr << "done." << std::endl;
						}
						else
						{
							std::cerr << "Loading sampled inverse suffix array from disk...";
							libmaus2::aio::InputStreamInstance istr(fsisafilename);
							SISA = ::libmaus2::autoarray::AutoArray<uint64_t>(istr);
							assert ( istr );
							std::cerr << "done." << std::endl;
						}
						
						if ( ! ::libmaus2::util::GetFileSize::fileExists(fbwtname) )
						{
							std::cerr << "Computing BWT...";
							text_array_ptr_type BWT = text_array_ptr_type(new text_array_type(n,data->getB()));
							uint64_t p0rank = 0;
							for ( uint64_t i = 0; i < n; ++i )
							{
								uint64_t const p = CSA->get(i);
								
								if ( p )
									BWT->set(i,data->get(p-1));
								else
								{
									p0rank = i;
									BWT->set(i,data->get(n-1));
								}
							}
							std::cerr << "done." << std::endl;
							
							std::cerr << "Converting to wavelet tree bits...";
							libmaus2::autoarray::AutoArray<uint64_t> WTbits = ::libmaus2::wavelet::toWaveletTreeBitsParallel(BWT.get());
							std::cerr << "done." << std::endl;

							std::cerr << "Constructing wavelet tree...";
							wt_ptr_type qwt = wt_ptr_type(new wt_type(WTbits,n,data->getB()));
							std::cerr << "done." << std::endl;
							
							std::cerr << "Setting up LF...";
							lf = lf_ptr_type(new lf_type(qwt));
							std::cerr << "done." << std::endl;
							
							#if 0
							uint64_t r = p0rank;
							for ( uint64_t i = 0; i < n; ++i )
							{
								std::cerr << rv((*lf)[r]);
								r = (*lf)(r);
							}
							std::cerr << std::endl;
							#endif
							
							std::cerr << "Writing LF to disk...";
							{
							libmaus2::aio::OutputStreamInstance ostr(fbwtname);
							lf->serialize(ostr);
							ostr.flush();
							}
							std::cerr << "done." << std::endl;
						}
						else
						{
							std::cerr << "Reading LF from disk...";
							libmaus2::aio::InputStreamInstance istr(fbwtname);
							lf = lf_ptr_type(new lf_type(istr));
							assert ( istr );
							istr.close();
							std::cerr << "done." << std::endl;
						}
					}
					else
					{
						std::cerr << "Loading proper symbol vector...";
						libmaus2::aio::InputStreamInstance dolistr(dolvecname);
						R = libmaus2::autoarray::AutoArray<uint64_t>(dolistr);
						assert ( dolistr );
						dolistr.close();
						std::cerr << "done." << std::endl;				

						std::cerr << "Loading sampled inverse suffix array from disk...";
						libmaus2::aio::InputStreamInstance ssaistr(fsisafilename);
						SISA = ::libmaus2::autoarray::AutoArray<uint64_t>(ssaistr);
						assert ( ssaistr );
						ssaistr.close();
						std::cerr << "done." << std::endl;

						std::cerr << "Reading LF from disk...";
						libmaus2::aio::InputStreamInstance istr(fbwtname);
						lf = lf_ptr_type(new lf_type(istr));
						assert ( istr );
						istr.close();
						std::cerr << "done." << std::endl;
					}
					
					std::cerr << "Constructing rank dictionary for proper symbol vector...";
					dolrank = rank_ptr_type(new rank_type(R.get(),R.size()*sizeof(rank_data_type)*8));
					std::cerr << "done." << std::endl;
					
					n = lf->getN();

					if ( ! ::libmaus2::util::GetFileSize::fileExists(flcpfilename) )
					{
						std::cerr << "Computing LCP...";
						LCP = computeLCP(lf.get());
						std::cerr << "done." << std::endl;
					
						std::cerr << "Serialising LCP...";
						{
						libmaus2::aio::OutputStreamInstance ostr(flcpfilename);
						LCP->serialise(ostr);
						ostr.flush();
						}
						std::cerr << "done." << std::endl;
					}
					else
					{
						std::cerr << "Reading LCP from disk...";
						libmaus2::aio::InputStreamInstance istr(flcpfilename);
						LCP = ::libmaus2::util::Array864::unique_ptr_type(new ::libmaus2::util::Array864(istr));
						assert ( istr );
						assert ( lf->getN() == n );
						istr.close();
						std::cerr << "done." << std::endl;				
					}

					if ( ! ::libmaus2::util::GetFileSize::fileExists(fprevname) )
					{
						std::cerr << "Computing prev...";
						prev = psv864(*LCP,n);
						std::cerr << "done." << std::endl;
					
						std::cerr << "Writing prev to disk...";	
						{
						libmaus2::aio::OutputStreamInstance ostr(fprevname);
						prev->serialise(ostr);
						ostr.flush();
						}
						std::cerr << "done." << std::endl;
					}
					else
					{				
						std::cerr << "Reading prev from disk...";
						libmaus2::aio::InputStreamInstance istr(fprevname);
						prev = ::libmaus2::util::NegativeDifferenceArray64::unique_ptr_type(new ::libmaus2::util::NegativeDifferenceArray64(istr));
						assert ( istr );
						assert ( lf->getN() == n );
						istr.close();
						std::cerr << "done." << std::endl;				
					}
					
					if ( ! ::libmaus2::util::GetFileSize::fileExists(fnextname) )
					{
						std::cerr << "Computing next...";
						next = nsv864(*LCP,n);
						std::cerr << "done." << std::endl;
					
						std::cerr << "Writing next to disk...";	
						{
						libmaus2::aio::OutputStreamInstance ostr(fnextname);
						next->serialise(ostr);
						ostr.flush();
						}
						std::cerr << "done." << std::endl;
					}
					else
					{				
						std::cerr << "Reading next from disk...";
						libmaus2::aio::InputStreamInstance istr(fnextname);
						next = ::libmaus2::util::PositiveDifferenceArray64::unique_ptr_type(new ::libmaus2::util::PositiveDifferenceArray64(istr));
						assert ( istr );
						assert ( lf->getN() == n );
						istr.close();
						std::cerr << "done." << std::endl;				
					}
					
					typedef std::pair < uint64_t,uint64_t > upair;
					typedef std::pair < upair, uint64_t > qpair;
					std::vector < qpair > todo;
					for ( uint64_t i = 0; (i+1) < SISA.size(); ++i )
					{
						uint64_t const plow = i*sasamplingrate;
						uint64_t const phigh = plow + sasamplingrate - 1;
						uint64_t const r = (*lf)(SISA[i+1]);
						todo.push_back( qpair(upair(plow,phigh),r) );
						// std::cerr << "plow=" << plow << " phigh=" << phigh << " r=" << r << std::endl;
					}
					if ( todo.size() )
						todo.push_back( qpair( upair(todo.back().first.second,n/2-1), n-2 ) );
					else
						todo.push_back( qpair( upair(0,n/2-1), n-2 ) );				

					::libmaus2::autoarray::AutoArray<uint64_t> udist(1024);				
						
					::libmaus2::parallel::OMPLock lock;	
					
					uint64_t finished = 0;
					
					U = ::libmaus2::autoarray::AutoArray<uint64_t>(n/2,false);
					
					#if defined(_OPENMP)
					#pragma omp parallel for
					#endif
					for ( int64_t i = 0; i < static_cast<int64_t>(todo.size()); ++i )
					{
						::libmaus2::autoarray::AutoArray<uint64_t> ludist(udist.size());

						qpair const & qp = todo[i];
						upair const & up = qp.first;
											
						uint64_t tr = qp.second;
						for ( int64_t p = up.second; p >= static_cast<int64_t>(up.first); --p )
						{
							Node N = parent(Node(tr));
							uint64_t const s = nextImproper(p)-p;				
							uint64_t const preudepth = N.depth+1;
							uint64_t const udepth = (preudepth <= s) ? preudepth : std::numeric_limits<uint64_t>::max();
							
							if ( udepth < ludist.size() )
								ludist[udepth]++;
								
							U[p] = udepth;

							#if 0
							lock.lock();
							std::cerr << "position " << p << " rank " << tr << " unique depth " << udepth
								<<  " s=" << s << std::endl;
							lock.unlock();
							#endif

							tr = (*lf)(tr);
						}
						
						lock.lock();
						for ( uint64_t j = 0; j < udist.size(); ++j )
							udist[j] += ludist[j];
						finished += (up.second-up.first);
						std::cerr << "[" << up.first << "," << up.second << "] " << qp.second << " finished " << finished << " / " << (n/2-1) << std::endl;
						lock.unlock();
					}
					// std::cerr << "numc=" << (n/2-1) << std::endl;
					
					for ( uint64_t i = 0; i < udist.size(); ++i )
						if ( udist[i] )
							std::cerr << i << "\t" << udist[i] << std::endl;
							
					libmaus2::aio::OutputStreamInstance ostr(fufilename);
					U.serialize(ostr);
					ostr.flush();
					assert ( ostr );
				}
				else
				{
					libmaus2::aio::InputStreamInstance istr(fufilename);
					U = ::libmaus2::autoarray::AutoArray<uint64_t>(istr);
					assert ( istr );
					istr.close();
				}
			}
		};
	
		struct UniqueIndex
		{
			typedef uint8_t char_type;
			enum { ALPHABET_SIZE=256 };
			typedef ::libmaus2::suffixsort::DivSufSort<64,char_type *,char_type const *,int64_t *,int64_t const *,ALPHABET_SIZE> sort_type;
			typedef ::libmaus2::suffixsort::DivSufSortUtils<64,char_type *,char_type const *,int64_t *,int64_t const *,ALPHABET_SIZE> sort_util_type;
			typedef ::libmaus2::suffixsort::DivSufSortUtils<64,char_type *,char_type const *,uint32_t *,uint32_t const *,ALPHABET_SIZE> sort_util_post_type;
			typedef sort_type::saidx_t saidx_t;

			typedef UniqueIndex this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			static std::pair<uint64_t,uint64_t> countChars(std::vector<std::string> const & filenames)
			{
				::libmaus2::aio::SynchronousFastReaderBase SFR(filenames);
				std::string line;
				
				uint64_t numc = 0;
				uint64_t numsep = 0;
				bool lastwasdata = true;
				bool first = true;
				
				while ( SFR.getLine(line) )
					if ( line.size() )
					{
						if ( line[0] != '>' )
						{
							if ( !lastwasdata && ! first )
							{
								numc++; // count separator
								numsep++;
							}
							for ( uint64_t i = 0; i < line.size(); ++i )
								if ( ! isspace(line[i]) )
									numc++;

							lastwasdata = true;
							first = false;
						}
						else
						{
							lastwasdata = false;
						}
					}
				
				numc += 1;

				return std::pair<uint64_t,uint64_t>(numc,numsep);
			}

			static char_type fw(char_type const v)
			{
				switch ( v )
				{
					case '#': return 0;
					case 'A': return 1;
					case 'C': return 2;
					case 'G': return 3;
					case 'T': return 4;
					case 'N': return 5;
					case '$': return 6;
					default: assert ( false );
				}
			}

			static uint8_t rv(char_type const v)
			{
				switch ( v )
				{
					case 0: return '#';
					case 1: return 'A';
					case 2: return 'C';
					case 3: return 'G';
					case 4: return 'T';
					case 5: return 'N';
					default: return '$';
				}
			}

			static uint64_t rc(uint64_t const c)
			{
				switch (c)
				{
					case 1: return 4; // A->T
					case 2: return 3; // C->G
					case 3: return 2; // G->C
					case 4: return 1; // T->A
					default: return c;
				}
			}


			static ::libmaus2::autoarray::AutoArray<char_type> readFiles(std::vector<std::string> const & filenames, uint64_t const numc)
			{
				::libmaus2::autoarray::AutoArray<char_type> A(numc,false);
				::libmaus2::aio::SynchronousFastReaderBase SFR(filenames);
				std::string line;
				char_type * p = A.get();
				char_type sep = 6;
				
				bool lastwasdata = true;
				bool first = true;
				
				while ( SFR.getLine(line) )
					if ( line.size() )
					{
						if ( line[0] != '>' )
						{
							if ( !lastwasdata && (!first) )
							{
								*(p++) = (sep++);
							}
							for ( uint64_t i = 0; i < line.size(); ++i )
								if ( ! isspace(line[i]) )
								{
									switch ( toupper ( line[i] ) )
									{
										case 'A': case 'C':
										case 'G': case 'T':
											*(p++) = fw(toupper(line[i])); break;
										default:
											*(p++) = fw('N'); break;
									}
								}
							
							lastwasdata = true;
							first = false;
						}
						else
						{
							lastwasdata = false;
						}
					}
					
				*(p++) = 0;

				assert ( p == A.end() );

				#if 0
				for ( char_type const * i = A.begin(); i != A.end(); ++i )
				{
					std::cerr << rv(*i);
					if ( rv(*i) == '$' )
						std::cerr << "(" << static_cast<int>(*i) << ")";
				}
				std::cerr << std::endl;
				#endif

				return A;
			}

			#if 0
			static ::libmaus2::autoarray::AutoArray<char_type> computeBWT(::libmaus2::autoarray::AutoArray<char_type> const & A)
			{
				unsigned int const k = 8;
				unsigned int const w = 32;
				
				uint64_t const n = A.size();
				if ( ! n )
					return ::libmaus2::autoarray::AutoArray<char_type>();

				unsigned int const albits = getAlBits();
				::libmaus2::wavelet::DynamicWaveletTree<k,w> B(albits);
				::libmaus2::cumfreq::DynamicCumulativeFrequencies scf(sort_type::ALPHABET_SIZE);
				// ::libmaus2::cumfreq::SlowCumFreq scf(sort_type::ALPHABET_SIZE);
				
				uint64_t p = 0;
				for ( int64_t i = n-2; i >= 0; --i )
				{
					if ( (i & (1024*1024-1)) == 0 )
						std::cerr << "(dbwt " << i << ")" << std::endl;
					uint64_t const c = A[i];
					uint64_t const f = scf[c]+1; scf.inc(c);
					p = B.insertAndRank(c,p) - 1 + f;
				}
				B.insert ( 0, p );
				scf.inc(0);

				std::cerr << "(dbwt " << 0 << ")" << std::endl;
				
				::libmaus2::autoarray::AutoArray<char_type> BWT(n,false);
				
				#if defined(_OPENMP)
				#pragma omp parallel for
				#endif
				for ( int64_t i = 0; i < static_cast<int64_t>(n); ++i )
					BWT[i] = B[i];
					
				#if 0
				for ( uint64_t i = 0; i < n; ++i )
					std::cerr << rv( B[i]  );
				std::cerr << std::endl;
				#endif
				
				return BWT;
			}
			#endif

			typedef ::libmaus2::lf::QuickLF lf_type;
			typedef ::libmaus2::rank::ERank222B rank_type;
			typedef rank_type::unique_ptr_type rank_ptr_type;
			//typedef ::libmaus2::lf::LF lf_type;
			typedef lf_type::unique_ptr_type lf_ptr_type;
			typedef lf_type::wt_type wt_type;
			typedef wt_type::unique_ptr_type wt_ptr_type;

			uint64_t numc;
			uint64_t numsep;
			::libmaus2::autoarray::AutoArray<char_type> data;
			::libmaus2::autoarray::AutoArray<char_type> rdata;
			uint64_t n;
			::libmaus2::autoarray::AutoArray<uint32_t> SA;
			::libmaus2::autoarray::AutoArray<uint32_t> ISA;
			::libmaus2::util::Array832::unique_ptr_type LCP;
			::libmaus2::util::NegativeDifferenceArray32::unique_ptr_type prev;
			::libmaus2::util::PositiveDifferenceArray32::unique_ptr_type next;
			::libmaus2::autoarray::AutoArray<uint32_t> RSA;
			::libmaus2::autoarray::AutoArray<uint32_t> RISA;
			::libmaus2::util::Array832::unique_ptr_type RLCP;
			::libmaus2::util::NegativeDifferenceArray32::unique_ptr_type rprev;
			::libmaus2::util::PositiveDifferenceArray32::unique_ptr_type rnext;

			wt_ptr_type qwt;
			lf_ptr_type lf;
			wt_ptr_type rqwt;
			lf_ptr_type rlf;
			uint64_t p0rank;
			uint64_t rp0rank;

			void extendRight(uint64_t const sym, uint64_t & spf, uint64_t & epf, uint64_t & spr, uint64_t & epr) const
			{
				// forward search using reverse
				uint64_t smaller, larger;
				rlf->W->smallerLarger( sym, spr, epr, smaller, larger );
				// update straight
				spf += smaller;
				epf -= larger;

				// backward search on reverse
				spr = rlf->step(sym, spr);
				epr = rlf->step(sym, epr);		
			
			}

			void extendLeft(uint64_t const sym, uint64_t & spf, uint64_t & epf, uint64_t & spr, uint64_t & epr) const
			{
				// forward search using reverse
				uint64_t smaller, larger;
				lf->W->smallerLarger( sym, spf, epf, smaller, larger );
				// update straight
				spr += smaller;
				epr -= larger;

				// backward search on reverse
				spf = lf->step(sym, spf);
				epf = lf->step(sym, epf);
			}

			/**
			 * algorithm nsv832
			 **/
			template<typename lcp_array_type>
			static ::libmaus2::util::PositiveDifferenceArray32::unique_ptr_type nsv832(lcp_array_type const & LCP, int64_t n)
			{
				// allocate result array
				::libmaus2::autoarray::AutoArray<uint32_t> next(n,false);

				// do not crash for n==0
				if ( n )
				{
					// initialize next for rank n-1
					next[n-1] = n;
				}
			  
				// compute next for the remaining ranks
				for ( int64_t r = static_cast<int64_t>(n)-2; r >= 0; --r )
				{
					int64_t t = r + 1;

					while ( (t < n) && (LCP[t] >= LCP[r]) )
						t = next[t];
					next[r] = t;
				}
			  
				for ( int64_t r = 0; r < n; ++r )
				{
					next[r] = next[r]-r;
				}

				::libmaus2::util::Array832::unique_ptr_type anext(new ::libmaus2::util::Array832(next.begin(),next.end()));
			  
				// return result
				return ::libmaus2::util::PositiveDifferenceArray32::unique_ptr_type(new ::libmaus2::util::PositiveDifferenceArray32(anext));
			}

			/**
			 * algorithm psv832
			 **/
			template<typename lcp_array_type>
			static ::libmaus2::util::NegativeDifferenceArray32::unique_ptr_type psv832(lcp_array_type const & LCP, uint64_t n)
			{
				// allocate result array
				::libmaus2::autoarray::AutoArray<uint32_t> prev(n,false);

				// do not crash for n==0
				if ( n )
					prev[0] = 0;

				for ( uint64_t r = 1; r < n; ++r )
				{
					uint64_t t = r - 1;
			  
					while ( (t > 0) && (LCP[t] >= LCP[r]) )
						t = prev[t];

					prev[r] = t;
				}
			  
				for ( uint64_t r = 0; r < n; ++r )
					prev[r] = r-prev[r];
				
				::libmaus2::util::Array832::unique_ptr_type aprev(new ::libmaus2::util::Array832(prev.begin(),prev.end()));

				return ::libmaus2::util::NegativeDifferenceArray32::unique_ptr_type(new ::libmaus2::util::NegativeDifferenceArray32(aprev));
			}

			struct Node
			{
				uint32_t left;
				uint32_t right;
				uint32_t depth;
				
				Node() : left(0), right(0), depth(0) {}
				Node(uint32_t r) : left(r), right(r), depth(0) {}
				Node(uint32_t rleft, uint32_t rright, uint32_t rdepth)
				: left(rleft), right(rright), depth(rdepth) {}
				
				std::string toString() const
				{
					std::ostringstream ostr;
					ostr << "Node(" << left << "," << right << "," << depth << ")";
					return ostr.str();
				}
			};
			
			inline uint64_t suffixLink(uint64_t const r) const
			{
				return ISA[(SA[r]+1)%n];
			}
			inline uint64_t reverseSuffixLink(uint64_t const r) const
			{
				return RISA[(RSA[r]+1)%n];
			}

			inline Node parent(Node const & I) const
			{
				if ( I.left == 0 && I.right == n-1 )
				{
					std::cerr << "Warning, called parent on root node, returning root." << std::endl;
					return Node(0,n-1,0);
				}
					
				uint32_t const k = ( (I.right+1 >= n) || ((*LCP)[I.left] > (*LCP)[I.right+1]) ) ? I.left : (I.right+1);
				// depth LCP[k]
				return Node ((*prev)[k],(*next)[k]-1, (*LCP)[k]);
			}
			inline Node rparent(Node const & I) const
			{
				if ( I.left == 0 && I.right == n-1 )
				{
					std::cerr << "Warning, called rparent on root node, returning root." << std::endl;
					return Node(0,n-1,0);
				}

				uint32_t const k = ( (I.right+1 >= n) || ((*RLCP)[I.left] > (*RLCP)[I.right+1]) ) ? I.left : (I.right+1);
				// depth RLCP[k]
				return Node ((*rprev)[k],(*rnext)[k]-1, (*RLCP)[k]);
			}
			
			template<typename sa_elem_type>
			struct OffsetStringComparator
			{
				sa_elem_type const * RSA;
				char_type const * data;
				uint64_t const n;
				uint64_t const offset;
				uint64_t const complen;
				
				OffsetStringComparator(
					sa_elem_type const * rRSA,
					char_type const * rdata,
					uint64_t const rn,
					uint64_t const roffset,
					uint64_t const rcomplen)
				: RSA(rRSA), data(rdata), n(rn), offset(roffset), complen(rcomplen) 
				{
					// std::cerr << "complen " << complen << std::endl;
				}
				
				int compare(uint64_t const r0, uint64_t const r1) const
				{
					uint64_t p0 = RSA[r0]+offset;
					uint64_t p1 = RSA[r1]+offset;
					
					// std::cerr << "comparing p0=" << p0 << " and p1=" << p1 << std::endl;
					
					if ( p0 == p1 )
						return 0;
					
					if ( std::max(p0,p1) >= n )
					{
						if ( p0 > p1 )
							return -1;
						else
							return 1;
					}
					else
					{	
						uint64_t l = 0;
						while ( l < complen && data[p0] == data[p1]  )
							p0++, p1++, ++l;
							
						if ( l == complen )
							return 0;
							
						if ( data[p0] < data[p1] )
							return -1;
						else
							return 1;
					}
				}
			};

			template<typename comparator>
			static inline uint64_t smallestLargerEqualBinary(
				uint64_t const st1, 
				uint64_t const ed1,
				comparator const & A,
				uint64_t const st2
				)
			{
				uint64_t left = st1;
				uint64_t right = ed1;
				
				while ( right-left > 2 )
				{
					uint64_t const m = (right+left)/2;
					
					// searched value is right of m
					if ( A.compare ( m, st2 ) < 0 )
						left = m+1;
					// v >= st2, search value is either this one or to the left
					else
						right = m+1;				
				}

				for ( uint64_t i = left; i < right; ++i )
					if ( A.compare(i,st2) >= 0 )
						return i;
				
				return ed1;
			}

			template<typename comparator>
			static inline uint64_t smallestLargerBinary(
				uint64_t const st1, 
				uint64_t const ed1,
				comparator const & A,
				uint64_t const st2
				)
			{
				uint64_t left = st1;
				uint64_t right = ed1;
				
				while ( right-left > 2 )
				{
					uint64_t const m = (right+left)/2;
					
					// m <= st2
					if ( A.compare ( m, st2 ) <= 0 )
						left = m+1;
					// m > st2, search value is either this one or to the left
					else
						right = m+1;				
				}

				for ( uint64_t i = left; i < right; ++i )
					if ( A.compare(i,st2) > 0 )
						return i;
				
				return ed1;
			}

			static unsigned int getAlBits()
			{
				unsigned int albits = 0;
				uint64_t talsize = sort_type::ALPHABET_SIZE;
				unsigned int numbits = 0;
				
				while ( talsize )
				{
					if ( talsize & 1 )
						numbits++;
					albits++;
					talsize >>= 1;
				}
				albits--;
				
				assert ( numbits == 1 );
			
				return albits;
			}
			
			UniqueIndex(::libmaus2::util::ArgInfo const & arginfo)
			{
				::libmaus2::util::TempFileNameGenerator tmpgen("tmp",2);

				std::cerr << "Counting characters...";
				std::pair<uint64_t,uint64_t> NC = countChars(arginfo.restargs);
				numc = NC.first;
				numsep = NC.second;
				std::cerr << "done, numc="<<numc<<", numsep=" << numsep << std::endl;
				
				if ( numsep+5+1 > sort_type::ALPHABET_SIZE )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "too many fragments: " << numsep;
					se.finish();
					throw se;
				}
				
				std::cerr << "Reading data...";
				data = readFiles(arginfo.restargs, numc);
				n = data.size();
				std::cerr << "done, got " << n << " symbols." << std::endl;
				
				rdata = data.clone();
				std::reverse(rdata.begin(),rdata.end()-1);
				
				assert ( n );

				std::string const fsafilename = "fbwt.sa";
				std::string const rsafilename = "rbwt.sa";
				std::string const fbwtname = "fbwt";
				std::string const rbwtname = "rbwt";
				std::string const rlcpfilename = "rbwt.lcp";		
				std::string const flcpfilename = "fbwt.lcp";		

				#if defined(_OPENMP)
				#pragma omp parallel sections
				#endif
				{
					#if defined(_OPENMP)
					#pragma omp section
					#endif
					{
						std::cerr << "Computing forward suffix array...";
						if ( ::libmaus2::util::GetFileSize::fileExists(fsafilename) &&
							(::libmaus2::util::GetFileSize::getFileSize(fsafilename) == (1*sizeof(uint64_t)+n*sizeof(uint32_t))) )
						{
							std::cerr << "(load)";
							::libmaus2::aio::InputStreamInstance istr(fsafilename);
							SA.deserialize(istr);
							assert ( istr );
							istr.close();

							// sort_util_post_type::sufcheck(data.get(),SA.get(),n,1);
						}
						else
						{
							std::cerr << "(comp)";

							::libmaus2::autoarray::AutoArray<int64_t> TSA(n,false);
							// SAIS::SA_IS(data.get(), TSA.get(), n, ALPHABET_SIZE, 1);
							sort_type::divsufsort(data.get(),TSA.get(),n);
							// sort_util_type::sufcheck(data.get(),TSA.get(),n,1);
							
							SA = ::libmaus2::autoarray::AutoArray<uint32_t>(n,false);
							std::copy ( TSA.begin(), TSA.end(), SA.begin() );
							// sort_util_post_type::sufcheck(data.get(),SA.get(),n,1);
							
							libmaus2::aio::OutputStreamInstance ostr(fsafilename);
							SA.serialize(ostr);
							ostr.flush();
						}
						assert ( SA.size() == n );
						std::cerr << "done." << std::endl;


						std::cerr << "Computing LCP array...";
						if ( ::libmaus2::util::GetFileSize::fileExists(flcpfilename) &&
							(::libmaus2::util::GetFileSize::getFileSize(flcpfilename) == (1*sizeof(uint64_t)+(n+1)*sizeof(uint32_t))) )
						{
							std::cerr << "(load)";
							::libmaus2::aio::InputStreamInstance istr(flcpfilename);
							::libmaus2::autoarray::AutoArray<uint32_t> LLCP;
							LLCP.deserialize(istr);
							assert ( istr );
							istr.close();
							
							LCP = ::libmaus2::util::Array832::unique_ptr_type(new ::libmaus2::util::Array832(LLCP.begin(),LLCP.end()));
						}
						else
						{
							std::cerr << "(comp)";
							::libmaus2::autoarray::AutoArray<uint32_t> LLCP = ::libmaus2::suffixsort::SkewSuffixSort<char_type,uint32_t>::lcpByPlcp(data.get(), n, SA.get(), 1);
							LLCP[n] = 0;
							{
							libmaus2::aio::OutputStreamInstance ostr(flcpfilename);
							LLCP.serialize(ostr);
							ostr.flush();
							}

							LCP = ::libmaus2::util::Array832::unique_ptr_type(new ::libmaus2::util::Array832(LLCP.begin(),LLCP.end()));
						}
						std::cerr << "done." << std::endl;
					}
					
					#if defined(_OPENMP)
					#pragma omp section
					#endif
					{
						std::cerr << "Computing reverse suffix array...";
						if ( ::libmaus2::util::GetFileSize::fileExists(rsafilename) &&
							(::libmaus2::util::GetFileSize::getFileSize(rsafilename) == (1*sizeof(uint64_t)+n*sizeof(uint32_t))) )
						{
							std::cerr << "(load)";
							::libmaus2::aio::InputStreamInstance istr(rsafilename);
							RSA.deserialize(istr);
							assert ( istr );
							istr.close();
							// sort_util_post_type::sufcheck(rdata.get(),RSA.get(),n,1);
						}
						else
						{
							std::cerr << "(comp)";

							::libmaus2::autoarray::AutoArray<int64_t> TSA(n,false);
							// SAIS::SA_IS(rdata.get(), TSA.get(), n, ALPHABET_SIZE, 1);
							sort_type::divsufsort(rdata.get(),TSA.get(),n);
							// sort_util_type::sufcheck(rdata.get(),TSA.get(),n,1);
							
							RSA = ::libmaus2::autoarray::AutoArray<uint32_t>(n,false);
							std::copy ( TSA.begin(), TSA.end(), RSA.begin() );
							// sort_util_post_type::sufcheck(rdata.get(),RSA.get(),n,1);
							
							{
							libmaus2::aio::OutputStreamInstance ostr(rsafilename);
							RSA.serialize(ostr);
							ostr.flush();
							}
						}
						std::cerr << "done." << std::endl;

						std::cerr << "Computing reverse LCP array...";
						if ( ::libmaus2::util::GetFileSize::fileExists(rlcpfilename) &&
							(::libmaus2::util::GetFileSize::getFileSize(rlcpfilename) == (1*sizeof(uint64_t)+(n+1)*sizeof(uint32_t))) )
						{
							std::cerr << "(load)";
							::libmaus2::aio::InputStreamInstance istr(rlcpfilename);
							::libmaus2::autoarray::AutoArray<uint32_t> LRLCP;
							LRLCP.deserialize(istr);
							assert ( istr );
							istr.close();
							
							RLCP = ::libmaus2::util::Array832::unique_ptr_type(new ::libmaus2::util::Array832(LRLCP.begin(),LRLCP.end()));
						}
						else
						{
							std::cerr << "(comp)";
							::libmaus2::autoarray::AutoArray<uint32_t> LRLCP = ::libmaus2::suffixsort::SkewSuffixSort<char_type,uint32_t>::lcpByPlcp(rdata.get(), n, RSA.get(), 1);
							LRLCP[n] = 0;
							{
							libmaus2::aio::OutputStreamInstance ostr(rlcpfilename);
							LRLCP.serialize(ostr);
							ostr.flush();
							}

							RLCP = ::libmaus2::util::Array832::unique_ptr_type(new ::libmaus2::util::Array832(LRLCP.begin(),LRLCP.end()));
						}
						std::cerr << "done." << std::endl;
					}
				}
				
				std::cerr << "Computing forward ISA...";
				ISA = ::libmaus2::autoarray::AutoArray<uint32_t>(n,false);
				#if defined(_OPENMP)
				#pragma omp parallel for
				#endif
				for ( int64_t r = 0; r < static_cast<int64_t>(n); ++r )
					ISA[SA[r]] = r;
				std::cerr << "done." << std::endl;

				unsigned int const albits = getAlBits();
				std::cerr << "Alphabet size " << sort_type::ALPHABET_SIZE << " bits " << albits << std::endl;

				std::cerr << "Computing reverse ISA...";
				RISA = ::libmaus2::autoarray::AutoArray<uint32_t>(n,false);
				#if defined(_OPENMP)
				#pragma omp parallel for
				#endif
				for ( int64_t r = 0; r < static_cast<int64_t>(n); ++r )
					RISA[RSA[r]] = r;
				std::cerr << "done." << std::endl;

				#define WTEXTERNAL

				if ( ! ::libmaus2::util::GetFileSize::fileExists(fbwtname) )
				{
					#if defined(WTEXTERNAL)
					::libmaus2::wavelet::ExternalWaveletGenerator ewg(albits,tmpgen);
					#else
					::libmaus2::autoarray::AutoArray<char_type> BWT(n,false);
					char_type * ppp = BWT.begin();
					#endif
					
					std::cerr << "Creating forward BWT...";
					
					uint64_t p0rank = 0;
					for ( uint64_t r = 0; r < n; ++r )
					{
						int64_t p = SA[r];
						
						if ( p == 0 )
						{
							#if defined(WTEXTERNAL)
							ewg.putSymbol( data[ data.size()-1] );
							#else
							*(ppp++) = data[ data.size()-1];
							#endif
							p0rank = r;
						}
						else
						{
							#if defined(WTEXTERNAL)
							ewg.putSymbol( data[p-1] );				
							#else
							*(ppp++) = data[p-1];
							#endif
						}
					}
					this->p0rank = p0rank;
					std::cerr << "done." << std::endl;
					
					#if defined(WTEXTERNAL)
					std::cerr << "Creating final stream...";
					ewg.createFinalStream(fbwtname);
					std::cerr << "done." << std::endl;
					#else
					::libmaus2::wavelet::WaveletTree< rank_type, char_type > WT(BWT.begin(),n);
					{
					libmaus2::aio::OutputStreamInstance wtostr(fbwtname);
					WT.serialize(wtostr);
					wtostr.flush();
					assert ( wtostr );
					}
					#endif
				}
				else
				{
					uint64_t p0rank = 0;
					for ( uint64_t r = 0; r < n; ++r )
						if ( SA[r] == 0 )
							p0rank = r;
					this->p0rank = p0rank;		
				}

				if ( ! ::libmaus2::util::GetFileSize::fileExists(rbwtname) )
				{
					#if defined(WTEXTERNAL)
					::libmaus2::wavelet::ExternalWaveletGenerator ewg(albits,tmpgen);
					#else
					::libmaus2::autoarray::AutoArray<char_type> BWT(n,false);
					char_type * ppp = BWT.begin();
					#endif
					
					std::cerr << "Creating reverse BWT...";
					uint64_t p0rank = 0;
					for ( uint64_t r = 0; r < n; ++r )
					{
						int64_t p = RSA[r];
						
						if ( p == 0 )
						{
							#if defined(WTEXTERNAL)
							ewg.putSymbol( rdata[ rdata.size()-1] );
							#else
							*(ppp++) = rdata[ rdata.size()-1];
							#endif
							p0rank = r;
						}
						else
						{
							#if defined(WTEXTERNAL)
							ewg.putSymbol( rdata[p-1] );
							#else
							*(ppp++) = rdata[p-1];
							#endif
						}
					}
					this->rp0rank = p0rank;
					std::cerr << "done." << std::endl;
					
					#if defined(WTEXTERNAL)
					std::cerr << "Creating final stream...";
					ewg.createFinalStream(rbwtname);
					std::cerr << "done." << std::endl;
					#else
					::libmaus2::wavelet::WaveletTree< rank_type , char_type > WT(BWT.begin(),n);
					{
					libmaus2::aio::OutputStreamInstance ostr(rbwtname);
					WT.serialize(ostr);
					ostr.flush();
					assert ( ostr );
					}
					#endif
				}
				else
				{
					uint64_t p0rank = 0;
					for ( uint64_t r = 0; r < n; ++r )
						if ( RSA[r] == 0 )
							p0rank = r;
					this->rp0rank = p0rank;		
				}

				std::cerr << "Loading BWT...";
				::libmaus2::aio::InputStreamInstance bwtin(fbwtname);
				qwt = UNIQUE_PTR_MOVE(wt_ptr_type(new wt_type(bwtin)));
				bwtin.close();
				std::cerr << "done." << std::endl;
				
				std::cerr << "Setting up LF...";
				lf = UNIQUE_PTR_MOVE(lf_ptr_type(new lf_type(qwt)));
				std::cerr << "done." << std::endl;
				
				std::cerr << "Loading reverse BWT...";
				::libmaus2::aio::InputStreamInstance rbwtin("rbwt");
				rqwt = UNIQUE_PTR_MOVE(wt_ptr_type(new wt_type(rbwtin)));
				rbwtin.close();
				std::cerr << "done." << std::endl;

				std::cerr << "Setting up reverse LF...";
				rlf = UNIQUE_PTR_MOVE(lf_ptr_type(new lf_type(rqwt)));
				std::cerr << "done." << std::endl;

				std::cerr << "Computing prev/next...";
				
				std::string const prevname = "prev";
				std::string const nextname = "next";
				std::string const rprevname = "rprev";
				std::string const rnextname = "rnext";

				#if defined(_OPENMP)
				#pragma omp parallel sections
				#endif
				{
					#if defined(_OPENMP)
					#pragma omp section
					#endif
					{
						if ( ! ::libmaus2::util::GetFileSize::fileExists(prevname) )
						{
							prev = psv832(*LCP, n);
							libmaus2::aio::OutputStreamInstance ostr(prevname);
							prev->serialise(ostr);
							ostr.flush();
							assert ( ostr );
						}
						else
						{
							libmaus2::aio::InputStreamInstance istr(prevname);
							prev = ::libmaus2::util::NegativeDifferenceArray32::unique_ptr_type(new ::libmaus2::util::NegativeDifferenceArray32(istr));
							assert ( istr );
							istr.close();
				 
						}
					}
					#if defined(_OPENMP)
					#pragma omp section
					#endif
					{
						if ( ! ::libmaus2::util::GetFileSize::fileExists(nextname) )
						{
							next = nsv832(*LCP, n);
							libmaus2::aio::OutputStreamInstance ostr(nextname);
							next->serialise(ostr);
							ostr.flush();
							assert ( ostr );
						}
						else
						{
							libmaus2::aio::InputStreamInstance istr(nextname);
							next = ::libmaus2::util::PositiveDifferenceArray32::unique_ptr_type(new ::libmaus2::util::PositiveDifferenceArray32(istr));
							assert ( istr );
							istr.close();
						}
					}
					#if defined(_OPENMP)
					#pragma omp section
					#endif
					{
						if ( ! ::libmaus2::util::GetFileSize::fileExists(rprevname) )
						{
							rprev = psv832(*RLCP, n);
							libmaus2::aio::OutputStreamInstance ostr(rprevname);
							rprev->serialise(ostr);
							ostr.flush();
							assert ( ostr );
						}
						else
						{
							libmaus2::aio::InputStreamInstance istr(rprevname);
							rprev = ::libmaus2::util::NegativeDifferenceArray32::unique_ptr_type(new ::libmaus2::util::NegativeDifferenceArray32(istr));
							assert ( istr );
							istr.close();
				 
						}
					}
					#if defined(_OPENMP)
					#pragma omp section
					#endif
					{
						if ( ! ::libmaus2::util::GetFileSize::fileExists(rnextname) )
						{
							rnext = nsv832(*RLCP, n);
							libmaus2::aio::OutputStreamInstance ostr(rnextname);
							rnext->serialise(ostr);
							ostr.flush();
							assert ( ostr );
						}
						else
						{
							libmaus2::aio::InputStreamInstance istr(rnextname);
							rnext = ::libmaus2::util::PositiveDifferenceArray32::unique_ptr_type(new ::libmaus2::util::PositiveDifferenceArray32(istr));
							assert ( istr );
							istr.close();
						}
					}
				}

				std::cerr << "done." << std::endl;
			}
			
			struct TraversalContext
			{
				UniqueIndex const * index;
				uint64_t spf;
				uint64_t epf;
				uint64_t spr;
				uint64_t epr;
				uint64_t l;
				
				TraversalContext(UniqueIndex const * rindex)
				: index(rindex)
				{
					reset();
				}
				
				void reset()
				{
					spf = 0; epf = index->n;
					spr = 0; epr = index->n;
					l = 0;
				}
				
				void contextSuffixLink()
				{
					spf = index->suffixLink(spf);
					epf = index->suffixLink(epf-1)+1;
					l -= 1;
				
					/* call parent until we have reduced the string depth to at most l */
					/* so we include all ranks with matching prefixes */
					Node par = index->parent(Node(spf,epf-1,0));
					while ( par.depth > l )
						par = index->parent(par);
					/* compute how far below the desired depth we are now */
					uint64_t const lcpred = l-par.depth;	
						
					/* set new l value */
					l = par.depth;
					
					/* copy back values from node structure */
					spf = par.left;
					epf = par.right+1;

					/* call suffix link operation lcpred times to reduce length of match */
					for ( uint64_t i = 0; i < lcpred; ++i )
					{
						spr = index->reverseSuffixLink(spr);
						epr = index->reverseSuffixLink(epr-1)+1;
						assert ( epr > spr );
					}

					/* use parent operations until string depth of interval has fallen to at most l */
					Node rpar = index->rparent(Node(spr,epr-1,0));
					
					while ( rpar.depth > l )
						rpar = index->rparent(rpar);
					
					/* copy back interval from node structure */
					spr = rpar.left;
					epr = rpar.right+1;

					/* remaining string depth of reverse interval */
					uint64_t const rlcp = rpar.depth;

					/* extend reverse if we reduced below l */
					if ( l-rlcp )
					{
						// uint64_t const revoff = (n-1)-(p+l);
						uint64_t const revoff = (index->n-1)-(index->SA[spf]+l);
						uint64_t const offsetlow = rlcp;

						OffsetStringComparator<uint32_t> OSC(index->RSA.get(),index->rdata.get(),index->n,offsetlow,l-rlcp);
						spr = smallestLargerEqualBinary(spr,epr,OSC,index->RISA[revoff]);
						epr = smallestLargerBinary(spr,epr,OSC,index->RISA[revoff]);
					}
				}
				
				void extendRight(uint64_t const c)
				{
					index->extendRight( c , spf, epf, spr, epr );
					l++;
				}
			};
			
			static uint64_t const maxl = 1000;

			void process(std::string const outfilename) const
			{
				::libmaus2::util::TempFileNameGenerator tmpgen("uni_out_tmp",4);
				
				uint64_t const pblocksize = 64*1024;
				// uint64_t const pblocksize = 8ull * 1024ull * 1024ull * 1024ull; // 64*1024;
				uint64_t const numblocks = (n + pblocksize-1)/pblocksize;
				::libmaus2::parallel::OMPLock lock;
				
				std::vector < std::string > unifilenames;
				for ( uint64_t pp = 0; pp < numblocks; ++pp )
					unifilenames . push_back (tmpgen.getFileName() );
				
				#if 1
				#if defined(_OPENMP)
				#pragma omp parallel for schedule(dynamic,1)
				#endif
				#endif
				for ( int64_t pp = 0 ; pp < static_cast<int64_t>(numblocks); ++pp )
				{
					std::string const unioutname = unifilenames[pp];			
					::libmaus2::aio::SynchronousGenericOutput<uint32_t> uniout(unioutname,64*1024);

					uint64_t p = std::min(pp*pblocksize,n);
					uint64_t const pn = std::min(p+pblocksize,n);
				
					TraversalContext TI(this);
					
					while ( p < pn )
					{
						if ( (p & (8*1024-1)) == 0 )
						{
							lock.lock();
							std::cerr << "pp=" << static_cast<double>(pp)/numblocks << " p=" << p << " pn=" << pn << " l=" << TI.l << std::endl;
							lock.unlock();
						}

						while ( TI.epf-TI.spf > 1 )
							TI.extendRight(data[p+TI.l]);
						
						assert ( SA[TI.spf] == p );

						if ( TI.l <= maxl )
						{
							uint64_t rspf = 0, repf = n;
							uint64_t rl = 0;
							
							while ( 
								(p+rl) < n 
								&& 
								(repf > rspf)
								#if defined(PALIN)
								&&
								(
									(repf-rspf > 1) || ((repf-rspf==1)&&rspf!=TI.spf)
								)
								#endif
								&&
								(rl <= maxl)
							)
							{
								#if defined(PALIN)
								// check for palindrome
								if ( repf-rspf == 1 && rspf == TI.spf )
									break;
								#endif
							
								char_type const forw = data[p+rl];
								char_type const reve = rc(forw);
								rspf = lf->step(reve,rspf);
								repf = lf->step(reve,repf);
								
								rl++;
							}
							
							#if defined(PALIN)
							if ( repf-rspf == 1 && rspf == TI.spf )
							{
								lock.lock();
								std::cerr << "found palindrome" << std::endl;
								
								for ( uint64_t i = 0; i < rl; ++i )
									std::cerr << rv(data[ p + i ]);
								std::cerr << std::endl;
								lock.unlock();
							}
							#endif
							
							if ( (rl > maxl) || (p+rl==n) )
							{
								uniout.put( std::numeric_limits<uint32_t>::max() );
							}
							else
							{
								assert ( rl <= maxl );
								assert ( TI.l <= maxl );
								uniout.put( std::max(TI.l,rl) );
							}
											
							TI.contextSuffixLink();
							
							p += 1;
						}
						else
						{
							assert ( TI.l > maxl );
							uint64_t const ldif = TI.l-maxl;

							#if 1 // defined(UNIQUE_DEBUG)
							std::cerr << "Skipping " << ldif << " characters as LCP is too high" << std::endl;
							#endif
							
							for ( uint64_t z = 0; z < ldif && p < pn; ++z )
							{
								uniout.put(std::numeric_limits<uint32_t>::max());
								++p;
							}

							TI.reset();
						}
					}
					
					uniout.flush();
				}

				{		
					::libmaus2::aio::SynchronousGenericOutput<uint32_t> uniout(outfilename,64*1024);
					for ( uint64_t pp = 0; pp < numblocks; ++pp )
					{
						std::string const & filename = unifilenames[pp];

						{
						::libmaus2::aio::SynchronousGenericInput<uint32_t> uniin(filename,64*1024);
						uint32_t v;
						
						while ( uniin.getNext(v) )
							uniout.put(v);
						}
							
						libmaus2::aio::FileRemoval::removeFile ( filename );
					}
					uniout.flush();
				}		
			}
			
			std::ostream & printSuffix(std::ostream & out, uint64_t const r) const
			{
				uint64_t p = SA[r];
				
				out << r << "\t" << SA[r] << "\t" << (*LCP)[r] << "\t" << (*prev)[r] << "\t" << (*next)[r] << "\t";
				
				Node I(r);
				Node P = parent(I);
				
				out << P.toString() << "\t";
						
				for ( uint64_t i = p; i < n; ++i )
					out << rv(data[i]);
				out << std::endl;	
				
				return out;
			}
			std::ostream & printReverseSuffix(std::ostream & out, uint64_t const r) const
			{
				uint64_t p = RSA[r];
				
				out << r << "\t" << RSA[r] << "\t" << (*RLCP)[r] << "\t" << (*rprev)[r] << "\t" << (*rnext)[r] << "\t";

				Node I(r);
				Node P = rparent(I);
				
				out << P.toString() << "\t";

				for ( uint64_t i = p; i < n; ++i )
					out << rv(rdata[i]);
				out << std::endl;	
				
				return out;
			}
		};
	}
}
#endif
