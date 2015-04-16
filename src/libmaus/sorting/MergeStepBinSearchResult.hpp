/*
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
*/
#if ! defined(LIBMAUS_SORTING_MERGESTEPBINSEARCHRESULT_HPP)
#define LIBMAUS_SORTING_MERGESTEPBINSEARCHRESULT_HPP

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <vector>
#include <map>

#include <libmaus/types/types.hpp>

namespace libmaus
{
	namespace sorting
	{
		struct MergeStepBinSearchResult
		{
			uint64_t l0;
			uint64_t l1;
			uint64_t r0;
			uint64_t r1;
			int64_t nbest;
			
			MergeStepBinSearchResult() : l0(0), l1(0), r0(0), r1(0), nbest(std::numeric_limits<int64_t>::max()) {}
			MergeStepBinSearchResult(
				uint64_t const rl0,
				uint64_t const rl1,
				uint64_t const rr0,
				uint64_t const rr1,
				int64_t const rnbest
			) : l0(rl0), l1(rl1), r0(rr0), r1(rr1), nbest(rnbest) {}
			
			MergeStepBinSearchResult sideswap() const
			{
				return MergeStepBinSearchResult(r0,r1,l0,l1,nbest);
			}

			template<typename iterator, typename order_type>
			static MergeStepBinSearchResult mergestepbinsearch(
				iterator const aa, 
				iterator const ae, 
				iterator const ba, 
				iterator const be, 
				order_type order,
				uint64_t const xc = 1, // split counter
				uint64_t const xd = 2  // split denominator
			)
			{
				typedef typename ::std::iterator_traits<iterator>::value_type value_type;		

				// size of left block
				uint64_t const s = ae-aa;
				// size of right block
				uint64_t const t = be-ba;
				// split
				uint64_t const x = (xc*(s+t))/xd;

				// pointers on left block
				uint64_t l = 0;
				uint64_t r = s;
				
				while ( r-l > 2 )
				{
					uint64_t const m = (l+r) >> 1;
					value_type const & v = aa[m];

					iterator bm = std::lower_bound(ba,be,v,order);

					int64_t n = static_cast<int64_t>((bm-ba) + m) - x;
					
					// if we do not have enough elements to reach the split
					// but we could add more equal elements from the rhs block
					if ( 
						n < 0 && 
						(bm != be) && 
						// v == *bm
						(!(order(*bm,v))) && (!(order(v,*bm)))
					)
					{
						// count number of elements equalling v
						std::pair<iterator,iterator> const eqr = ::std::equal_range(ba,be,v,order);
						// add
						n += std::min(-n,static_cast<int64_t>(eqr.second-eqr.first));
					}
								
					if ( n < 0 )
						l = m+1; // l excluded
					else // n >= 0
						r = m+1; // r included
				}
				
				uint64_t lbest = l;
				int64_t nbest = std::numeric_limits<int64_t>::max();
				iterator bmbest = ba;
				
				for ( uint64_t m = (l ? (l-1):l); m < r; ++m )
				{
					value_type const v = aa[m];
				
					iterator bm = std::lower_bound(ba,be,v,order);

					int64_t n = static_cast<int64_t>((bm-ba) + m) - x;
					
					if ( n < 0 && bm != be && (!(order(*bm,v))) && (!(order(v,*bm))) )
					{
						std::pair<iterator,iterator> const eqr = ::std::equal_range(ba,be,v,order);
						uint64_t const add = std::min(-n,static_cast<int64_t>(eqr.second-eqr.first));
						n += add;
						bm += add;
					}
					
					if ( iabs(n) < iabs(nbest) )
					{
						lbest = m;
						nbest = n;
						bmbest = bm;
					}
				}

				uint64_t l0 = lbest;
				uint64_t l1 = s-l0;
					
				uint64_t r0 = bmbest-ba;
				uint64_t r1 = t-r0;
				
				// make sure equal elements are on the left
				if ( 
					l1 && r0 &&
					(!order(aa[l0],ba[r0-1])) &&
					(!order(ba[r0-1],aa[l0]))
				)
				{
					std::pair<iterator,iterator> const eqrl = ::std::equal_range(aa,ae,aa[l0],order);
					std::pair<iterator,iterator> const eqrr = ::std::equal_range(ba,be,aa[l0],order);
					
					int64_t const lp = eqrl.second - (aa + l0);
					assert ( lp > 0 );
					assert ( 
						(!order(aa[l0+lp-1],aa[l0]))
						&&
						(!order(aa[l0],aa[l0+lp-1]))
					);
					int64_t const rm = (ba + r0) - eqrr.first;
					assert ( rm > 0 );
					assert ( 
						(!order(ba[r0-rm],aa[l0]))
						&&
						(!order(aa[l0],ba[r0-rm]))
					);
					
					uint64_t const sh = std::min(lp,rm);
												
					l0 += sh;
					l1 -= sh;
					r0 -= sh;
					r1 += sh;
				}
				
				assert ( (!l1) || (!r0) || order(aa[l0],ba[r0-1]) || order(ba[r0-1],aa[l0]) );

				return MergeStepBinSearchResult(l0,l1,r0,r1,nbest);
			}

			static int64_t iabs(int64_t const v)
			{
				if ( v < 0 )
					return -v;
				else
					return v;
			}

			template<typename iterator, typename order_type>
			static MergeStepBinSearchResult mergestepbinsearchOpt(
				iterator const aa, 
				iterator const ae, 
				iterator const ba, 
				iterator const be, 
				order_type order,
				uint64_t const xc = 1, // split counter
				uint64_t const xd = 2  // split denominator
			)
			{
				MergeStepBinSearchResult const msbsr_l = MergeStepBinSearchResult::mergestepbinsearch(aa,ae,ba,be,order,xc,xd);
				MergeStepBinSearchResult const msbsr_r = MergeStepBinSearchResult::mergestepbinsearch(ba,be,aa,ae,order,xc,xd).sideswap();
				MergeStepBinSearchResult const msbsr = (iabs(msbsr_l.nbest) <= iabs(msbsr_r.nbest)) ? msbsr_l : msbsr_r;
				return msbsr;
			}
			
			template<typename iterator, typename order_type>
			static std::vector< std::pair<uint64_t,uint64_t> > mergeSplitVector(
				iterator aa,
				iterator ae,
				iterator ba,
				iterator be,
				order_type order,
				uint64_t p
			)
			{
				std::vector< std::pair<uint64_t,uint64_t> > V;
			
				while ( p > 1 )
				{
					MergeStepBinSearchResult const binres = mergestepbinsearch(
						aa,ae,ba,be,order,1,p
					);
					
					V.push_back(std::pair<uint64_t,uint64_t>(binres.l0,binres.r0));
					
					aa += binres.l0;
					ba += binres.r0;
					p  -= 1;
					
					assert ( ae-aa == static_cast<ptrdiff_t>(binres.l1) );
					assert ( be-ba == static_cast<ptrdiff_t>(binres.r1) );
				}
				
				if ( p )
				{
					V.push_back(std::pair<uint64_t,uint64_t>(ae-aa,be-ba));
					p -= 1;
				}
				
				assert ( ! p );
				
				std::vector< std::pair<uint64_t,uint64_t> > VA;
				uint64_t sa = 0;
				uint64_t sb = 0;
				for ( uint64_t i = 0; i < V.size(); ++i )
				{
					VA.push_back(std::pair<uint64_t,uint64_t>(sa,sb));
					sa += V[i].first;
					sb += V[i].second;
				}
				VA.push_back(std::pair<uint64_t,uint64_t>(sa,sb));
				
				return VA;
			}
		};
	}
}
#endif
