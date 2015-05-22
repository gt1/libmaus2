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
				{
					H.updateOffset(offset,H.atOffset(offset) | (static_cast<uint64_t>(e) << shift));
				}
				else
				{
					H.insertNonSyncExtend(ptr>>5,static_cast<uint64_t>(e) << shift,0.8);
				}
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
				// todo queue	
				std::deque<QueueElement> Q;
				// insert origin
				Q.push_back(QueueElement(0,0,0,0,0,0,0));
				// diag len
				uint64_t const diaglen = std::min(na,nb)+1;
				// current distance
				unsigned int curd = 0;
				// points to be processed with this distance 
				unsigned int dc = 1;
				// points with distance curd+1
				unsigned int nextdc = 0;
				// current maximum antidiagonal
				uint64_t curmaxantidiag = 0;
				// next d maximum antidiagonal
				uint64_t nextmaxantidiag = 0;

				// trace data
				editops.clear();
				bool aligned = false;

				uint64_t const evecmask = 0x7FFFFFFFFFFFFFFFull;

				while ( Q.size() )
				{
					QueueElement P = Q.front();
					Q.pop_front();
					
					// start point on a	
					uint64_t pa = P.pa;
					// start point on b
					uint64_t pb = P.pb;
					// anti diagonal id
					uint64_t const antidiag = pa + pb;
					
					assert ( antidiag <= curmaxantidiag );
					
					if ( curmaxantidiag - antidiag <= maxantidiagdiff )
					{
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
						bool ok = true;
						uint64_t diagptr = diagptr_f(pa,pb,diaglen)+1;
						while ( s && a[pa] == b[pb] && (ok=(diagaccess_get_f(editops,diagptr) == step_none)) )
						{
							diagaccess_set_f(editops,diagptr,step_diag);
							++mat, ++pa, ++pb, ++diagptr, --s;
							evec = (evec & evecmask) << 1;
						}

						if ( ok )
						{
							// at least one not at end
							if ( (na-pa) | (nb-pb) )
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

												if ( (libmaus2::rank::PopCnt8<sizeof(unsigned long)>::popcnt8(qel.evec) <= maxevecpopcnt) )
												{
													nextmaxantidiag = std::max(nextmaxantidiag,static_cast<uint64_t>(qel.pa+qel.pb));
													Q.push_back(qel);
													diagaccess_set_f(editops,diagptr_f(pa,pb+1,diaglen),step_ins);
													nextdc += 1;
												}
											}
											if ( diagaccess_get_f(editops,diagptr_f(pa+1,pb,diaglen)) == step_none )
											{
												QueueElement qel(pa+1,pb,mat,mis,ins,del+1,((evec & evecmask) << 1) | 1ull);

												if ( (libmaus2::rank::PopCnt8<sizeof(unsigned long)>::popcnt8(qel.evec) <= maxevecpopcnt) )
												{
													nextmaxantidiag = std::max(nextmaxantidiag,static_cast<uint64_t>(qel.pa+qel.pb));
													Q.push_back(qel);
													diagaccess_set_f(editops,diagptr_f(pa+1,pb,diaglen),step_del);
													nextdc += 1;
												}
											}
											if ( diagaccess_get_f(editops,diagptr_f(pa+1,pb+1,diaglen)) == step_none )
											{
												QueueElement qel(pa+1,pb+1,mat,mis+1,ins,del,((evec & evecmask) << 1) | 1ull);

												if ( (libmaus2::rank::PopCnt8<sizeof(unsigned long)>::popcnt8(qel.evec) <= maxevecpopcnt) )
												{
													nextmaxantidiag = std::max(nextmaxantidiag,static_cast<uint64_t>(qel.pa+qel.pb));
													Q.push_back(qel);
													diagaccess_set_f(editops,diagptr_f(pa+1,pb+1,diaglen),step_diag);
													nextdc += 1;
												}
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

												if ( (libmaus2::rank::PopCnt8<sizeof(unsigned long)>::popcnt8(qel.evec) <= maxevecpopcnt) )
												{
													nextmaxantidiag = std::max(nextmaxantidiag,static_cast<uint64_t>(qel.pa+qel.pb));
													Q.push_back(qel);
													diagaccess_set_f(editops,diagptr_f(pa+1,pb,diaglen),step_del);
													nextdc += 1;
												}
											}
										}
									}
									else // na == pa
									{
										assert ( nb != pb );
										if ( diagaccess_get_f(editops,diagptr_f(pa,pb+1,diaglen)) == step_none )
										{
											QueueElement qel(pa,pb+1,mat,mis,ins+1,del,((evec & evecmask) << 1) | 1ull);

											if ( (libmaus2::rank::PopCnt8<sizeof(unsigned long)>::popcnt8(qel.evec) <= maxevecpopcnt) )
											{
												nextmaxantidiag = std::max(nextmaxantidiag,static_cast<uint64_t>(qel.pa+qel.pb));
												Q.push_back(qel);
												diagaccess_set_f(editops,diagptr_f(pa,pb+1,diaglen),step_ins);
												nextdc++;
											}
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
					
					// no more points for this distance?
					if ( ! --dc )
					{
						dc = nextdc;
						nextdc = 0;
						curmaxantidiag = nextmaxantidiag;
						nextmaxantidiag = 0;
						curd++;						
					}
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
