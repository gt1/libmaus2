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

#if ! defined(ERANK2_HPP)
#define ERANK2_HPP

#include <libmaus2/rank/ERankBase.hpp>
#include <libmaus2/bitio/BitWriter.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/util/unique_ptr.hpp>
#include <cassert>
#include <stdexcept>

namespace libmaus2
{
	namespace rank
	{
		/**
		 * two level rank directory using approximately n/2 bits for index.
		 * the super block size is 2^16 bits, the miniblock size is 32 bits.
		 * rank queries are answered using the dictionary and call to a 16
		 * bit population count dictionary
		 **/
		struct ERank2 : public ERankBase
		{
			public:
			typedef ::libmaus2::bitio::BitWriter writer_type;

			typedef ERank2 this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
		
			private:
			// super block size 2^16 bits
			static unsigned int const sbbitwidth = 16;
			// mini block size 2^5 = 32 bits
			static unsigned int const mbbitwidth = 5;
			static unsigned int const sbsize = 1u << sbbitwidth;
			static unsigned int const mbsize = 1u << mbbitwidth;
			static unsigned int const mps = ERANK2DIVUP(sbsize,mbsize);
			static unsigned int const sbmask = (1u<<sbbitwidth)-1;
			static unsigned int const mbmask = (1u<<mbbitwidth)-1;

			uint16_t const * const UU;
			unsigned int const n;
			
			::libmaus2::autoarray::AutoArray<unsigned int> S; // n / 2^16 * 32 bits = n / 2^11 bits
			::libmaus2::autoarray::AutoArray<unsigned short> M; // n / 2^16 * 2^16 / 32 * 16 = n/2 bits

			static inline unsigned int divUp(unsigned int a, unsigned int b)
			{
				return ERANK2DIVUP(a,b);
			}

			public:
			/**
			 * @param U bit vector
			 * @param rn number of bits in vector (has to be a multiple of 16)
			 **/
			ERank2(uint8_t const * const U, unsigned int const rn) 
			: UU(reinterpret_cast<uint16_t const *>(U)), n(rn), S( divUp(n,sbsize) , false ), M( divUp(n,mbsize), false )
			{
				if ( n & mbmask )
					throw ::std::runtime_error("::libmaus2::rank::ERank2: n is not multiple of 16.");
					
				unsigned int c = 0;

				// superblock counter
				int s = -1;
				int m = -1;
				
				for ( unsigned int i = 0; i < n; ++i )
				{
					if ( (i & sbmask) == 0 )
					{
						S[ ++s ] = c;
					}
					if ( (i & mbmask) == 0 )
					{
						M[ ++m ] = c - S[s];
						assert( S[s] + M[m] == c );
					}
					
					if ( ::libmaus2::bitio::getBit1(U,i) )
						c++;
				}
			}
			/**
			 * Return the position of the ii'th 0 bit. This function is implemented using a 
			 * binary search on the rank1 function.
			 **/
			unsigned int select0(unsigned int const ii) const
			{
				unsigned int const i = ii+1;

				unsigned int left = 0, right = n;
				
				while ( (right-left) )
				{
					unsigned int const d = right-left;
					unsigned int const d2 = d>>1;
					unsigned int const mid = left + d2;

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

			/**
			 * Return the position of the ii'th 1 bit. This function is implemented using a 
			 * binary search on the rank1 function.
			 **/
			unsigned int select1(unsigned int const ii) const
			{
				unsigned int const i = ii+1;

				unsigned int left = 0, right = n;
				
				while ( (right-left) )
				{
					unsigned int const d = right-left;
					unsigned int const d2 = d>>1;
					unsigned int const mid = left + d2;

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

			/**
			 * return number of 1 bits up to (and including) index i
			 * @param i
			 * @return population count
			 **/
			unsigned int rank1(unsigned int i) const
			{
				unsigned int const mi = i >> mbbitwidth; 
				unsigned int const ri = i - ( mi << mbbitwidth );

				if ( ri < 16 )
					return S[i >> sbbitwidth] + M[mi] + popcount2( reorder2( UU[mi<<1] ), ri);
				else
					return S[i >> sbbitwidth] + M[mi] + popcount2 ( UU[mi<<1] ) + popcount2( reorder2( UU[(mi<<1)|1]) , ri-16 );
			}
			/**
			 * return number of 0 bits up to (and including) index i
			 * @param i
			 * @return inverse population count
			 **/
			unsigned int rank0(unsigned int i) const
			{
				return (i+1) - rank1(i);
			}

			/**
			 * @return estimated space in bytes
			 **/
			unsigned int byteSize() const
			{
				return 
					sizeof(uint16_t *) + 
					sizeof(unsigned int) +
					S.byteSize() + 
					M.byteSize();
			}
		};
	}
}
#endif
