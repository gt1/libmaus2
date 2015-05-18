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
#if ! defined(LIBMAUS2_LCS_ND_HPP)
#define LIBMAUS2_LCS_ND_HPP

#include <vector>
#include <utility>
#include <deque>
#include <iostream>
#include <algorithm>
#include <cassert>
#include <sstream>

#include <libmaus2/lcs/EditDistanceTraceContainer.hpp>

namespace libmaus2
{
	namespace lcs
	{
		struct ND : public EditDistanceTraceContainer
		{
			private:
			typedef std::pair<unsigned int, unsigned int> upair;

			// id of diagonal
			static inline unsigned int diagid_f(unsigned int x, unsigned int y)
			{
				int const dif = static_cast<int>(x)-static_cast<int>(y);
				
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
			static inline unsigned int diagoffset_f(unsigned int const x, unsigned int const y)
			{
				return (x<y)?x:y;
			}

			static inline unsigned int diagptr_f(unsigned int const x, unsigned int const y, unsigned int const diaglen)
			{
				unsigned int const diagid = diagid_f(x,y);
				unsigned int const diagoffset = diagoffset_f(x,y);
				unsigned int diagptr = diagid * diaglen + diagoffset;
				return diagptr;
			}

			// edit operations per byte
			static unsigned int const elperbyte = 4;

			enum edit_op { step_none = 0, step_diag = 1, step_ins  = 2, step_del  = 3 };

			static inline edit_op diagaccess_get_f(std::vector<unsigned char> const & V, unsigned int const ptr)
			{
				switch ( (V.at(ptr>>2) >> ((ptr & 3)<<1)) & 3 )
				{
					case 0:  return step_none;
					case 1:  return step_diag;
					case 2:  return step_ins;
					default: return step_del;
				}
			}

			static inline void diagaccess_set_f(std::vector<unsigned char> & V, unsigned int ptr, edit_op const e)
			{
				assert ( diagaccess_get_f(V,ptr) == step_none );
				V.at(ptr>>2) |= static_cast<int>(e) << ((ptr&3)<<1);
			}
			
			std::vector<unsigned char> editops;
			
			public:
			ND()
			{
			}
			
			template<typename iterator_a, typename iterator_b>
			bool process(
				iterator_a a,
				size_t const na,
				iterator_b b,
				size_t const nb,
				uint64_t const d = std::numeric_limits<uint64_t>::max()
			)
			{
				// min size
				unsigned int const nmin = na < nb ? na : nb;

				// maximum length of diagonals (extend to multiple of elperbyte)
				unsigned int const diaglen = ( ((nmin + 1) + elperbyte - 1) / elperbyte ) * elperbyte;
				// todo queue	
				std::deque<upair> Q;
				// insert origin
				Q.push_back(upair(0,0));
				// current distance
				unsigned int curd = 0;
				// points to be processed with this distance 
				unsigned int dc = 1;
				// points with distance curd+1
				unsigned int nextdc = 0;

				// trace data (4 operations per byte)
				size_t const basesize = ((2+1) * diaglen ) / elperbyte;
				if ( editops.size() < basesize )
					editops.resize(basesize);
				std::fill(editops.begin(),editops.begin()+basesize,0);
				bool aligned = false;

				while ( Q.size() )
				{
					upair P = Q.front();
					Q.pop_front();

					#if 0				
					std::cerr << "P.first=" << P.first << " P.second=" << P.second << std::endl;
					#endif
					
					// start point on a	
					unsigned int pa = P.first;
					// start point on b
					unsigned int pb = P.second;

					unsigned int diagptr = diagptr_f(pa,pb,diaglen);
					
					// steps we can go along a		
					unsigned int const sa = na-pa;
					// steps we can go along b
					unsigned int const sb = nb-pb;
					// minimum of the two
					unsigned int s = sa < sb ? sa : sb;
					
					// slide
					bool ok = true;
					while ( s && a[pa] == b[pb] && (ok=(diagaccess_get_f(editops,diagptr+1) == step_none)) )
					{
						// assert ( diagaccess_get_f(editops,diagptr+1) == step_none );
						diagaccess_set_f(editops,++diagptr,step_diag);
						--s, ++pa, ++pb;
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
											diagaccess_set_f(editops,diagptr_f(pa,pb+1,diaglen),step_ins);
											Q.push_back(upair(pa,pb+1));
											nextdc += 1;
										}
										if ( diagaccess_get_f(editops,diagptr_f(pa+1,pb,diaglen)) == step_none )
										{
											diagaccess_set_f(editops,diagptr_f(pa+1,pb,diaglen),step_del);
											Q.push_back(upair(pa+1,pb));
											nextdc += 1;
										}
										if ( diagaccess_get_f(editops,diagptr_f(pa+1,pb+1,diaglen)) == step_none )
										{
											diagaccess_set_f(editops,diagptr_f(pa+1,pb+1,diaglen),step_diag);
											Q.push_back(upair(pa+1,pb+1));
											nextdc += 1;
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
											diagaccess_set_f(editops,diagptr_f(pa+1,pb,diaglen),step_del);
											Q.push_back(upair(pa+1,pb));
											nextdc += 1;
										}
									}	
								}
								else // na == pa
								{
									assert ( nb != pb );
									if ( diagaccess_get_f(editops,diagptr_f(pa,pb+1,diaglen)) == step_none )
									{
										diagaccess_set_f(editops,diagptr_f(pa,pb+1,diaglen),step_ins);
										Q.push_back(upair(pa,pb+1));
										nextdc++;
									}
								}
							}
						}
						// both at end, we have found an alignment
						else
						{
							aligned = true;
							// std::cerr << "! curd=" << curd << std::endl;
							break;
						}
					}
					
					// no more points for this distance?
					if ( ! --dc )
					{
						dc = nextdc;
						nextdc = 0;
						curd++;
						
						// resize editops vector
						if ( curd < d )
						{
							size_t const prevsize = (((2*(curd+0)) + 1) * diaglen ) / elperbyte;
							// assert ( editops.size() == prevsize );
							size_t const nextsize = (((2*(curd+1)) + 1) * diaglen ) / elperbyte;
							if ( editops.size() < nextsize )
								editops.resize( nextsize );
							std::fill(editops.begin()+prevsize,editops.begin()+nextsize,0);
						}
					}
				}
				
				// did we reach the bottom right corner?
				if ( 
					// diagaccess_get_f(editops,diagptr_f(na,nb,diaglen)) != step_none 
					aligned
				)
				{
					EditDistanceTraceContainer::reset();
					if ( EditDistanceTraceContainer::capacity() < na+nb )
						EditDistanceTraceContainer::resize(na+nb);
					
					EditDistanceTraceContainer::te = EditDistanceTraceContainer::ta = EditDistanceTraceContainer::trace.end();
					
					#if 0
					traceend = trace.end();
					tracebegin = trace.end();
					#endif
					
					unsigned int pa = na;
					unsigned int pb = nb;
					
					while ( pa != 0 || pb != 0 )
					{
						switch ( diagaccess_get_f(editops,diagptr_f(pa,pb,diaglen)) )
						{
							case step_diag:
								if ( a[--pa] == b[--pb] )
								{
									*(--EditDistanceTraceContainer::ta) = STEP_MATCH;
								}
								else
								{
									*(--EditDistanceTraceContainer::ta) = STEP_MISMATCH;									
								}
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
					return false;
				}
				
			}
		};
	}
}
#endif
