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

#if ! defined(ERANK_HPP)
#define ERANK_HPP

#include <libmaus/rank/ERankBase.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/bitio/BitWriter.hpp>
#include <libmaus/util/unique_ptr.hpp>
#include <stdexcept>

namespace libmaus
{
	namespace rank
	{
		/**
		 * single level rank dictionary using approximately n bits for index.
		 * the block size is 32 bits. rank queries are answered using the
		 * dictionary and calls to a 16 bit population count function.
		 **/
		struct ERank : public ERankBase
		{
			public:
			typedef ::libmaus::bitio::BitWriter writer_type;

			typedef ERank this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
		
			private:
			typedef uint32_t wordtype;

			static unsigned int const bs = (sizeof(wordtype) << 3);
		
			uint8_t const * const U;
			::libmaus::autoarray::AutoArray<wordtype> S; // n bits
			unsigned int const n;

			/**
			 * return superblock containing i th 1 bit,
			 * i.e. largest j such that S[j] < ii
			 **/
			unsigned int selectSuper(unsigned int const ii) const
			{
				// search largest superblock index s such that ii < S[s]
				unsigned int left = 0, right = (n + bs - 1) / bs;

				while ( right-left > 1 )
				{
					unsigned int const d = right-left;
					unsigned int const d2 = d>>1;
					unsigned int const mid = left + d2;

					// number of 1s is too large
					if ( S[mid] < ii )
						left = mid;
					else
						right = mid;
				}

				return left;
			}

			public:
			/**
			 * @param rU bit vector
			 * @param rn number of bits in vector (has to be a multiple of 32)
			 **/
			ERank(uint8_t const * const rU, unsigned int const rn) 
			: U(rU), S( (rn + bs - 1) / bs, false ), n(rn)
			{
				if ( n % 16 )
					throw ::std::runtime_error("::libmaus::rank::ERank: n is not multiple of 16.");
			
				unsigned int c = 0;
				for ( unsigned int i = 0; i < n; ++i )
				{
					if ( (i % bs) == 0 )
						S[ i / bs ] = c;
						
					if ( ::libmaus::bitio::getBit1(U,i) )
						c++;
				}
			}

			/**
			 * @return estimated space in bytes
			 **/
			unsigned int byteSize() const
			{
				return 
					sizeof(uint8_t *) + 
					S.byteSize() + 
					sizeof(unsigned int);
			}

			/**
			 * return number of 1 bits up to (and including) index i
			 * @param i
			 * @return population count
			 **/
			unsigned int rank1(unsigned int i) const
			{
				unsigned int const b = i / bs;
				unsigned int r = S[ i / bs ];
			
				unsigned int restbits = i - b * bs;
			
				uint16_t const * UU = reinterpret_cast<uint16_t const *>(U) + ((b*bs) >> 4);
				while ( restbits >= 16 )
				{
					r += popcount2(*(UU++));
					restbits -= 16;
				}
				r += popcount2( reorder2(*UU) , restbits );

				return r;
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
			 * Return the position of the ii'th 1 bit. This function is implemented using a 
			 * binary search on the rank1 function.
			 **/
			unsigned int select1(unsigned int const ii) const
			{
				unsigned int const i = ii+1;

				unsigned int const s = selectSuper(i);
				unsigned int left = s * bs, right = ::std::min(n, (s+1)*bs);
			
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
		};
	}
}
#endif
