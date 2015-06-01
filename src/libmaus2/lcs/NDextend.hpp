/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#if ! defined(LIBMAUS2_LCS_NDEXTEND_HPP)
#define LIBMAUS2_LCS_NDEXTEND_HPP

#include <vector>
#include <utility>
#include <deque>
#include <iostream>
#include <algorithm>
#include <cassert>
#include <sstream>

#include <libmaus2/lcs/EditDistanceTraceContainer.hpp>
#include <libmaus2/util/SimpleHashMap.hpp>
#include <libmaus2/rank/popcnt.hpp>

namespace libmaus2
{
	namespace lcs
	{
		struct NDextend : public EditDistanceTraceContainer
		{
			static uint64_t const evecmask = 0x7FFFFFFFFFFFFFFFull;

			private:
			struct QueueElement
			{
				uint32_t pa;
				uint32_t pb;
				
				uint32_t mat;
				uint32_t mis;
				uint32_t ins;
				uint32_t del;
				
				uint64_t evec;
				
				QueueElement() {}
				QueueElement(
					uint32_t const rpa, 
					uint32_t const rpb,
					uint32_t const rmat,
					uint32_t const rmis,
					uint32_t const rins,
					uint32_t const rdel,
					uint64_t const revec
				)
				: pa(rpa), pb(rpb), mat(rmat), mis(rmis), ins(rins), del(rdel), evec(revec)
				{
				
				}
				
				int64_t getScore() const
				{
					return	
						static_cast<int64_t>(mat) -
						static_cast<int64_t>(mis) -
						static_cast<int64_t>(ins) -
						static_cast<int64_t>(del);
				}
			};

			// id of diagonal
			static inline uint64_t diagid_f(uint64_t x, uint64_t y)
			{
				int64_t const dif = static_cast<int64_t>(x)-static_cast<int64_t>(y);
				
				if ( dif < 0 )
				{
					return ((-dif)<<1);
				}
				else if ( dif > 0 )
				{
					return ((dif-1)<<1)|1;
				}
				else
				{
					return 0;
				}	
			}

			// offset on diagonal
			static inline uint64_t diagoffset_f(uint64_t const x, uint64_t const y)
			{
				return (x<y)?x:y;
			}

			static inline uint64_t diagptr_f(uint64_t const x, uint64_t const y, uint64_t const diaglen)
			{
				uint64_t const diagid = diagid_f(x,y);
				uint64_t const diagoffset = diagoffset_f(x,y);
				uint64_t diagptr = diagid * diaglen + diagoffset;
				
				return diagptr;
			}
			
			static inline std::pair<uint64_t,uint64_t> rdiagptr_f(uint64_t ptr, uint64_t const diaglen)
			{
				uint64_t diagoffset = ptr % diaglen;
				uint64_t diagid = ptr / diaglen;
				
				if ( diagid == 0 )
				{
					return std::pair<uint64_t,uint64_t>(diagoffset,diagoffset);
				}
				else
				{
					if ( (diagid) % 2 == 0 )
					{
						return std::pair<uint64_t,uint64_t>(diagoffset,diagoffset+(diagid>>1));
					}
					else
					{
						return std::pair<uint64_t,uint64_t>(diagoffset + (((diagid-1)/2)+1),diagoffset);
					}
				}
			}
			
			#if 0
			static void inline enumDiag()
			{
				uint64_t const diaglen = 6;
				for ( uint64_t i = 0; i < 5; ++i )
					for ( uint64_t j = 0; j < 5; ++j )
					{
						std::pair<uint64_t,uint64_t> P = rdiagptr_f(diagptr_f(i,j,diaglen),diaglen);
						std::cerr << i << "," << j << " " << P.first << "," << P.second << std::endl;
					}
			}
			#endif

			// edit operations per byte
			enum edit_op { step_none = 0, step_diag = 1, step_ins  = 2, step_del  = 3 };

			static inline edit_op diagaccess_get_f(
				libmaus2::util::SimpleHashMap<uint64_t,uint64_t> const & H, uint64_t const ptr
			)
			{
				uint64_t v;
				if ( H.contains(ptr >> 5,v) )
				{
					switch ( (v >> ((ptr & 31) << 1)) & 3 )
					{
						case step_none: return step_none;
						case step_diag: return step_diag;
						case step_ins: return step_ins;
						case step_del: default: return step_del;
					}
				}
				else
				{
					return step_none;
				}
			}

			static inline void diagaccess_set_f(
				libmaus2::util::SimpleHashMap<uint64_t,uint64_t> & H, 
				uint64_t const ptr, 
				edit_op const e
			)
			{
				uint64_t offset;
				uint64_t const shift = (ptr & 31) << 1;
								
				if ( H.offset(ptr>>5,offset) )
					H.updateOffset(offset,H.atOffset(offset) | (static_cast<uint64_t>(e) << shift));
				else
					H.insertNonSyncExtend(ptr>>5,static_cast<uint64_t>(e) << shift,0.8);
			}
			
			libmaus2::util::SimpleHashMap<uint64_t,uint64_t> editops;
			
			public:
			NDextend() : editops(2)
			{
			}

			static std::string maskToString(uint64_t const v)
			{
				std::ostringstream ostr;
				for ( uint64_t i = 0; i < 64; ++i )
					ostr.put ( ((v & (1ull<<(63-i))) != 0) ? '1': '0' );
				return ostr.str();
			}

			template<typename iterator_a, typename iterator_b>
			void printTraceBack(std::ostream & out, iterator_a a, iterator_b b, uint64_t pa, uint64_t pb, uint64_t const diaglen, uint64_t const maxsteps = std::numeric_limits<uint64_t>::max()) const
			{
				std::deque<step_type> steps;
				uint64_t numsteps = 0;

				while ( (pa != 0 || pb != 0) && (numsteps++ < maxsteps) )
				{
					switch ( diagaccess_get_f(editops,diagptr_f(pa,pb,diaglen)) )
					{
						case step_diag:
							steps.push_front ( (a[--pa] == b[--pb]) ? STEP_MATCH : STEP_MISMATCH );
							break;
						case step_del:
							steps.push_front ( STEP_DEL );
							pa-=1;
							break;
						case step_ins:
							steps.push_front ( STEP_INS );
							pb-=1;
							break;
						default:
							assert ( false );
							break;
					}
				}

				for ( uint64_t i = 0; i < steps.size(); ++i )
				{
					switch ( steps[i] )
					{
						case STEP_MATCH: out.put('+'); break;
						case STEP_MISMATCH: out.put('-'); break;
						case STEP_INS: out.put('I'); break;
						case STEP_DEL: out.put('D'); break;
					}
				}
			}
			
			template<typename iterator_a, typename iterator_b>
			inline QueueElement slide(
				QueueElement const & P, 
				iterator_a a, size_t const na, 
				iterator_b b, size_t const nb, 
				uint64_t const diaglen,
				uint64_t & maxantidiag
			)
			{
				// start point on a	
				uint64_t pa = P.pa;
				// start point on b
				uint64_t pb = P.pb;

				uint32_t mat = P.mat;
				uint32_t mis = P.mis;
				uint32_t ins = P.ins;
				uint32_t del = P.del;
				uint64_t evec = P.evec;

				// steps we can go along a
				uint64_t const sa = na-pa;
				// steps we can go along b
				uint64_t const sb = nb-pb;
				// minimum of the two
				uint64_t s = sa < sb ? sa : sb;

				// slide
				uint64_t diagptr = diagptr_f(pa,pb,diaglen)+1;
				while ( s && a[pa] == b[pb] && (diagaccess_get_f(editops,diagptr) == step_none) )
				{
					diagaccess_set_f(editops,diagptr,step_diag);
					++mat, ++pa, ++pb, ++diagptr, --s;
					evec = (evec & evecmask) << 1;
				}

				// anti diagonal id
				uint64_t const antidiag = pa + pb;
				maxantidiag = std::max(maxantidiag,antidiag);

				return QueueElement(pa,pb,mat,mis,ins,del,evec);
			}

			void printMatrix(std::ostream & out, size_t const na, size_t const nb) const
			{
				// maximum length of diagonals (extend to multiple of elperbyte)
				unsigned int const diaglen = std::min(na,nb)+1;

				for ( size_t pa = 0; pa <= na; ++pa )
				{
					for ( size_t pb = 0; pb <= nb; ++pb )
					{
						switch ( 
							diagaccess_get_f(editops,diagptr_f(pa,pb,diaglen))
						)
						{
							case step_none: out.put('?'); break;
							case step_diag: out.put('\\'); break;
							case step_ins:  out.put('i'); break;
							case step_del:  out.put('d'); break;
						}
					}
					
					out << "\n";
				}
			}

			template<typename iterator_a, typename iterator_b, bool check_self = false>
			bool process(
				iterator_a a,
				size_t const na,
				iterator_b b,
				size_t const nb,
				uint64_t const d = std::numeric_limits<uint64_t>::max(),
				uint64_t const maxantidiagdiff = std::numeric_limits<uint64_t>::max(),
				uint64_t const maxevecpopcnt = std::numeric_limits<uint64_t>::max()
			)
			{
				editops.clear();
				
				// diag len
				uint64_t const diaglen = std::min(na,nb)+1;
				// todo queue	
				std::deque<QueueElement> Q, nextQ;
				// maximum antidiagonal
				uint64_t maxantidiag = 0;
				// insert origin
				Q.push_back(slide(QueueElement(0,0,0,0,0,0,0),a,na,b,nb,diaglen,maxantidiag));

				// trace data
				bool aligned = false;

				for ( unsigned int curd = 0; curd <= d; ++curd )
				{
					nextQ.clear();

					uint64_t nextmaxantidiag = 0;
					for ( uint64_t i = 0; i < Q.size(); ++i )
					{
						QueueElement const & P = Q[i];
					
						// start point on a	
						uint64_t pa = P.pa;
						// start point on b
						uint64_t pb = P.pb;
						
						if ( 
							check_self &&
							( a + pa == b + pb )
						)
						{
							EditDistanceTraceContainer::te = EditDistanceTraceContainer::ta = EditDistanceTraceContainer::trace.end();
							return false;		
						}

						// anti diagonal id
						uint64_t const antidiag = pa + pb;
						assert ( antidiag <= maxantidiag );

						if ( 
							maxantidiag - antidiag <= maxantidiagdiff 
							&&
							(libmaus2::rank::PopCnt8<sizeof(unsigned long)>::popcnt8(P.evec) <= maxevecpopcnt)
						)
						{
							uint32_t mat = P.mat;
							uint32_t mis = P.mis;
							uint32_t ins = P.ins;
							uint32_t del = P.del;
							uint64_t evec = P.evec;

							// at least one not at end
							if ( (na-pa) || (nb-pb) )
							{
								// one more error is still in range
								if ((curd+1)<=d)
								{
									// a not at end
									if ( na != pa )
									{
										// b not at end
										if ( nb != pb )
										{
											// sanity check
											assert ( na != pa && nb != pb );
											// enque three variants with one more error
											if ( diagaccess_get_f(editops,diagptr_f(pa,pb+1,diaglen)) == step_none )
											{
												QueueElement qel(pa,pb+1,mat,mis,ins+1,del,((evec & evecmask) << 1) | 1ull);
												diagaccess_set_f(editops,diagptr_f(pa,pb+1,diaglen),step_ins);
												nextQ.push_back(slide(qel,a,na,b,nb,diaglen,nextmaxantidiag));
											}
											if ( diagaccess_get_f(editops,diagptr_f(pa+1,pb,diaglen)) == step_none )
											{
												QueueElement qel(pa+1,pb,mat,mis,ins,del+1,((evec & evecmask) << 1) | 1ull);
												diagaccess_set_f(editops,diagptr_f(pa+1,pb,diaglen),step_del);
												nextQ.push_back(slide(qel,a,na,b,nb,diaglen,nextmaxantidiag));
											}
											if ( diagaccess_get_f(editops,diagptr_f(pa+1,pb+1,diaglen)) == step_none )
											{
												QueueElement qel(pa+1,pb+1,mat,mis+1,ins,del,((evec & evecmask) << 1) | 1ull);
												diagaccess_set_f(editops,diagptr_f(pa+1,pb+1,diaglen),step_diag);
												nextQ.push_back(slide(qel,a,na,b,nb,diaglen,nextmaxantidiag));
											}
										}
										// b is at end
										else
										{
											// sanity check
											assert ( nb == pb );
											// enque one variant with one more error
											if ( diagaccess_get_f(editops,diagptr_f(pa+1,pb,diaglen)) == step_none )
											{
												QueueElement qel(pa+1,pb,mat,mis,ins,del+1,((evec & evecmask) << 1) | 1ull);
												diagaccess_set_f(editops,diagptr_f(pa+1,pb,diaglen),step_del);
												nextQ.push_back(slide(qel,a,na,b,nb,diaglen,nextmaxantidiag));
											}
										}
									}
									else // na == pa
									{
										assert ( nb != pb );
										if ( diagaccess_get_f(editops,diagptr_f(pa,pb+1,diaglen)) == step_none )
										{
											QueueElement qel(pa,pb+1,mat,mis,ins+1,del,((evec & evecmask) << 1) | 1ull);
											diagaccess_set_f(editops,diagptr_f(pa,pb+1,diaglen),step_ins);
											nextQ.push_back(slide(qel,a,na,b,nb,diaglen,nextmaxantidiag));
										}
									}
								}
							}
							// both at end, we have found an alignment
							else
							{
								aligned = true;
								break;
							}
						}
					}

					maxantidiag = nextmaxantidiag;
					
					if ( aligned )
						break;
					else if ( nextQ.size() == 0 )
						break;
					else
						Q.swap(nextQ);
				}
				
				// did we reach the bottom right corner?
				if ( aligned )
				{
					EditDistanceTraceContainer::reset();
					if ( EditDistanceTraceContainer::capacity() < na+nb )
						EditDistanceTraceContainer::resize(na+nb);
					
					EditDistanceTraceContainer::te = EditDistanceTraceContainer::ta = EditDistanceTraceContainer::trace.end();
										
					uint64_t pa = na;
					uint64_t pb = nb;
					
					while ( pa != 0 || pb != 0 )
					{
						switch ( diagaccess_get_f(editops,diagptr_f(pa,pb,diaglen)) )
						{
							case step_diag:
								*(--EditDistanceTraceContainer::ta) = (a[--pa] == b[--pb]) ? STEP_MATCH : STEP_MISMATCH;
								break;
							case step_del:
								*(--EditDistanceTraceContainer::ta) = STEP_DEL;
								pa-=1;
								break;
							case step_ins:
								*(--EditDistanceTraceContainer::ta) = STEP_INS;
								pb-=1;
								break;
							default:
								assert ( false );
								break;
						}
					}
					
					return true;
				}
				else if ( Q.size() )
				{
					// find Q entry with maximum score
					int64_t maxscore = std::numeric_limits<int64_t>::min();
					uint64_t maxscoreindex = 0;
					
					for ( uint64_t i = 0; i < Q.size(); ++i )
					{
						int64_t const score = Q[i].getScore();
												
						if ( score > maxscore )
						{
							maxscore = score;
							maxscoreindex = i;
						}
							
					}
					
					QueueElement const & P = Q[maxscoreindex];
				
					uint64_t pa = P.pa;
					uint64_t pb = P.pb;

					EditDistanceTraceContainer::reset();
					if ( EditDistanceTraceContainer::capacity() < pa+pb )
						EditDistanceTraceContainer::resize(pa+pb);

					EditDistanceTraceContainer::te = EditDistanceTraceContainer::ta = EditDistanceTraceContainer::trace.end();
					
					while ( pa != 0 || pb != 0 )
					{
						switch ( diagaccess_get_f(editops,diagptr_f(pa,pb,diaglen)) )
						{
							case step_diag:
								*(--EditDistanceTraceContainer::ta) = (a[--pa] == b[--pb]) ? STEP_MATCH : STEP_MISMATCH;
								break;
							case step_del:
								*(--EditDistanceTraceContainer::ta) = STEP_DEL;
								pa-=1;
								break;
							case step_ins:
								*(--EditDistanceTraceContainer::ta) = STEP_INS;
								pb-=1;
								break;
							default:
								assert ( false );
								break;
						}
					}
					
					suffixPositive();

					return false;				
				}
				else
				{
					EditDistanceTraceContainer::te = EditDistanceTraceContainer::ta = EditDistanceTraceContainer::trace.end();
					return false;
				}
			}
		};
	}
}
#endif
