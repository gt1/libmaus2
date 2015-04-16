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

#if ! defined(ERANK3_HPP)
#define ERANK3_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/rank/ERankBase.hpp>
#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/bitio/BitWriter.hpp>

#include <cassert>
#include <iostream>

namespace libmaus2
{
	namespace rank
	{
		/**
		 * three level (sbsize 2^16, lbsize 2^8, mbsize 2^4) rank index using ~.56 bits
		 **/
		struct ERank3 : public ERankBase
		{
			public:
			typedef ::libmaus2::bitio::BitWriter2 writer_type;

			typedef ERank3 this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
		
			private:
			static unsigned int const sbbitwidth = 16;
			static unsigned int const lbbitwidth = 8;
			static unsigned int const mbbitwidth = 4;
			static unsigned int const sbsize = 1u<<sbbitwidth;
			static unsigned int const lbsize = 1u<<lbbitwidth;
			static unsigned int const mbsize = 1u<<mbbitwidth;
			static unsigned int const sbmask = sbsize-1;
			static unsigned int const lbmask = lbsize-1;
			static unsigned int const mbmask = mbsize-1;
		
			uint64_t const n;
			uint16_t const * const U;
			
			uint64_t const nums;
			::libmaus2::autoarray::AutoArray<uint64_t> S;
			uint64_t const numl;
			::libmaus2::autoarray::AutoArray<uint16_t> L;
			uint64_t const numm;
			::libmaus2::autoarray::AutoArray<uint8_t> M;

			public:
			uint64_t byteSize() const
			{
				return
					4*sizeof(uint64_t) +
					1*sizeof(uint16_t const *) +
					S.byteSize() +
					L.byteSize() +
					M.byteSize();
			}
			
			private:

			/**
			 * return superblock containing i th 1 bit,
			 * i.e. largest j such that S[j] < ii
			 **/
			uint64_t selectSuper(uint64_t const ii) const
			{
				// search largest superblock index s such that ii < S[s]
				uint64_t left = 0, right = nums;

				while ( right-left > 1 )
				{
					uint64_t const d = right-left;
					uint64_t const d2 = d>>1;
					uint64_t const mid = left + d2;

					// number of 1s is too large
					if ( S[mid] < ii )
						left = mid;
					else
						right = mid;
				}

				return left;
			}
			/**
			 * return large block containing i th 1 bit,
			 * i.e. largest j such that L[j] < ii
			 **/
			uint64_t selectLarge(uint64_t const s, uint64_t const iii) const
			{
				uint64_t const ii = iii - S[s];
				
				uint64_t left = (s << sbbitwidth) >>  lbbitwidth;
				uint64_t right = ::std::min( numl, ((s+1) << sbbitwidth) >>  lbbitwidth);

				while ( right-left > 1 )
				{
					uint64_t const d = right-left;
					uint64_t const d2 = d>>1;
					uint64_t const mid = left + d2;

					// number of 1s is too large
					if ( L[mid] < ii )
						left = mid;
					else
						right = mid;
				}

				return left;
			}
			/**
			 * return mini block containing i th 1 bit,
			 * i.e. largest j such that M[j] < ii
			 **/
			uint64_t selectMini(uint64_t const s, uint64_t const l, uint64_t const iii) const
			{
				uint64_t const ii = iii - S[s] - L[l];
				
				uint64_t left = (l << lbbitwidth) >>  mbbitwidth;
				uint64_t right = ::std::min( numm, ((l+1) << lbbitwidth) >>  mbbitwidth);

				while ( right-left > 1 )
				{
					uint64_t const d = right-left;
					uint64_t const d2 = d>>1;
					uint64_t const mid = left + d2;

					// number of 1s is too large
					if ( M[mid] < ii )
						left = mid;
					else
						right = mid;
				}

				return left;
			}

			
			public:
			/**
			 * constructor
			 * @param rU bit vector
			 * @param rn length of bit vector
			 **/	
			ERank3(uint16_t const * const rU, uint64_t const rn) 
			: n(rn), 
			  U(rU),
			  nums( ERANK2DIVUP(n,sbsize) ),
			  S( nums ),
			  numl( ERANK2DIVUP(n,lbsize) ),
			  L( numl ),
			  numm( ERANK2DIVUP(n,mbsize) ),
			  M( numm )
			{
				uint64_t c = 0;
			
				uint64_t l = 0, m = 0;
				for ( uint64_t s = 0 ; s < nums; ++s )
				{
					S[s] = c;
					assert ( S[s] == c );
					
					for ( uint64_t tl = 0; tl < sbsize/lbsize && l < numl; ++l, ++tl )
					{
						L[l] = c-S[s];
						assert ( L[l] == c-S[s] );
						
						for ( uint64_t tm = 0; tm < (lbsize/mbsize) && m < numm; ++m, ++tm )
						{
							M[m] = c-S[s]-L[l];
							assert ( M[m] == c-S[s]-L[l] );
						
							c += popcount2(U[m]);
						} 
					} 
				} 
			}
			
			/**
			 * return number of 1 bits up to (and including) index i
			 * @param i
			 * @return population count
			 **/
			uint64_t rank1(uint64_t i) const
			{
				uint64_t const m = i>>mbbitwidth;
				return S[i>>sbbitwidth] + L[i>>lbbitwidth] + M[m] + popcount2(U[m],i - (m<<mbbitwidth));
			} 
			/**
			 * return number of 0 bits up to (and including) index i
			 * @param i
			 * @return inverse population count
			 **/
			uint64_t rank0(uint64_t i) const
			{
				return (i+1) - rank1(i);
			}
			/**
			 * Return the position of the ii'th 0 bit. This function is implemented using a 
			 * binary search on the rank1 function.
			 **/
			uint64_t select0(uint64_t const ii) const
			{
				uint64_t const i = ii+1;

				uint64_t left = 0, right = n;
				
				while ( (right-left) )
				{
					uint64_t const d = right-left;
					uint64_t const d2 = d>>1;
					uint64_t const mid = left + d2;

					// number of ones is too small
					if ( rank0(mid) < i )
						left = mid+1;
					// number of ones is too large
					else if ( rank0(mid) > i )
						right = mid;
					// if this is the leftmost occurence in the interval, return it
					else if ( (!mid) || (rank0(mid-1) != i) )
						return mid;
					// otherwise, go on and search to the left
					else
						right = mid;
				}
				
				return n;		
			}
			#if 0
			/**
			 * Return the position of the ii'th 0 bit. This function is implemented using a 
			 * binary search on the rank1 function.
			 **/
			uint64_t select1(uint64_t const ii) const
			{
				uint64_t const i = ii+1;

				uint64_t left = 0, right = n;
				
				while ( (right-left) )
				{
					uint64_t const d = right-left;
					uint64_t const d2 = d>>1;
					uint64_t const mid = left + d2;

					// number of ones is too small
					if ( rank1(mid) < i )
						left = mid+1;
					// number of ones is too large
					else if ( rank1(mid) > i )
						right = mid;
					// if this is the leftmost occurence in the interval, return it
					else if ( (!mid) || (rank1(mid-1) != i) )
						return mid;
					// otherwise, go on and search to the left
					else
						right = mid;
				}
				
				return n;		
			}
			#else		
			/**
			 * Return the position of the ii'th 1 bit. This function is implemented using a 
			 * binary search on the rank1 function.
			 **/
			uint64_t select1(uint64_t const ii) const
			{
				uint64_t i = ii+1;

				// ::std::cerr << "in.";
				uint64_t const s = selectSuper(i);
				uint64_t const l = selectLarge(s,i);
				uint64_t const m = selectMini(s,l,i);
				i -= S[s]; 
				i -= L[l];
				i -= M[m];
				// ::std::cerr << "out." << ::std::endl;
				
				uint16_t const v = U[m];
				
				uint64_t left = 0, right = 1u<<mbbitwidth;
				while ( right-left )
				{
					uint64_t const d = right-left;
					uint64_t const d2 = d>>1;
					uint64_t const mid = left + d2;
					uint64_t const rmid = popcount2( v, mid );

					// number of ones is too small
					if ( rmid < i )
						left = mid+1;
					// number of ones is too large
					else if ( rmid > i )
						right = mid;
					// if this is the leftmost occurence in the interval, return it
					else if ( (!mid) || (popcount2( v, mid-1 ) != i) )
					{
						return (m<<mbbitwidth)+mid;
					}
					// otherwise, go on and search to the left
					else
						right = mid;
				}
				
				return n;
			}
			#endif

		};
	}
}
#endif
