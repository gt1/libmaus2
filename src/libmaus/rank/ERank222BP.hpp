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

#if ! defined(ERANK222BP_HPP)
#define ERANK222BP_HPP

#include <libmaus/bitio/BitWriter.hpp>
#include <libmaus/rank/ERankBase.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/util/unique_ptr.hpp>
#include <cassert>
#include <stdexcept>

namespace libmaus
{
	namespace rank
	{
		/**
		 * two level rank dictionary using approximately n/4 bits for index.
		 * the superblock size is 2^16 bits, the miniblock size is 64 bits.
		 * rank queries are answered using the dictionary and a 64 bit
		 * population count function. if the machine instruction set
		 * does not provide a 64 bit popcount function, these calls
		 * are simulated by using a precomputed 16 bit lookup table.
		 **/
		template<typename proc01>
		struct ERank222BP : public ERankBase
		{
			public:
			typedef ::libmaus::bitio::BitWriter8 writer_type;

			typedef ERank222BP this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
		
			private:
			// super block size 2^16 bits
			static unsigned int const sbbitwidth = 16;
			// mini block size 2^6 = 64 bits
			static unsigned int const mbbitwidth = 6;
			static unsigned int const sbsize = 1u << sbbitwidth;
			static unsigned int const mbsize = 1u << mbbitwidth;
			static unsigned int const mps = ERANK2DIVUP(sbsize,mbsize);
			static unsigned int const sbmask = (1u<<sbbitwidth)-1;
			static unsigned int const mbmask = (1u<<mbbitwidth)-1;

			uint64_t const * const UUUUUUUU;
			unsigned int const n;
			unsigned int const numsuper;
			unsigned int const nummini;
			
			::libmaus::autoarray::AutoArray<unsigned int> S; // n / 2^16 * 32 bits = n / 2^11 = n/2048 bits
			::libmaus::autoarray::AutoArray<unsigned short> M; // n / 2^16 * 2^16 / 64 * 16 = n/4 bits

			static inline unsigned int divUp(unsigned int a, unsigned int b)
			{
				return ERANK2DIVUP(a,b);
			}

			/**
			 * return superblock containing i th 1 bit,
			 * i.e. largest j such that S[j] < ii
			 **/
			unsigned int selectSuper(unsigned int const ii) const
			{
				// search largest superblock index s such that ii < S[s]
				unsigned int left = 0, right = numsuper;

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
			/**
			 * return miniblock containing i th 1 bit,
			 * i.e. largest j such that M[j] < ii
			 **/
			unsigned int selectMini(unsigned int const s, unsigned int const iii) const
			{
				unsigned int const ii = iii - S[s];
				unsigned int left = (s << sbbitwidth) >>  mbbitwidth;
				unsigned int right = ::std::min( nummini, ((s+1) << sbbitwidth) >>  mbbitwidth);
			
				while ( right-left > 1 )
				{
					unsigned int const d = right-left;
					unsigned int const d2 = d>>1;
					unsigned int const mid = left + d2;

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
			 * @param rUUUUUUUU bit vector
			 * @param rn number of bits in vector (has to be a multiple of 64)
			 **/
			ERank222BP(uint64_t const * const rUUUUUUUU, unsigned int const rn) 
			: UUUUUUUU(rUUUUUUUU), n(rn),
			  numsuper((n + (sbsize-1)) >> sbbitwidth), nummini((n + (mbsize-1)) >> mbbitwidth),
			  S( divUp(n,sbsize) , false ), M( divUp(n,mbsize), false)
			{
				if ( n & mbmask )
					throw ::std::runtime_error("::libmaus::rank::ERank222BP: n is not multiple of miniblock size 64.");
			
				unsigned int c = 0;

				// superblock counter
				int s = -1;
				// miniblock counter
				int m = -1;

				unsigned int i = 0;
				for ( unsigned int mi = 0; mi < nummini; ++mi, i += mbsize )
				{
					if ( (i & sbmask) == 0 )
					{
						S[ ++s ] = c;
						assert ( S[s] == c);
					}

					M[ ++m ] = c - S[s];
					assert( S[s] + M[m] == c );
				
					c += popcount8(proc01::proc01(UUUUUUUU[mi],UUUUUUUU[mi+1]));
				}
			}

			/**
			 * @return estimated space in bytes
			 **/		
			unsigned int byteSize() const
			{
				return 
					sizeof(uint64_t *) + 
					3*sizeof(unsigned int) +
					S.byteSize() + 
					M.byteSize();
			}

			
			/**
			 * return number of 1 bits up to (and including) index i
			 * @param i
			 * @return population count
			 **/
			unsigned int rank1(unsigned int i) const
			{
				unsigned int const mi = i >> mbbitwidth;
				return S[i >> sbbitwidth] + M[mi] + popcount8( proc01::proc01(UUUUUUUU[mi],UUUUUUUU[mi+1]), i - (mi<<mbbitwidth));
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
				unsigned int i = ii+1;

				unsigned int const s = selectSuper(i);
				unsigned int const m = selectMini(s,i);
				i -= S[s]; i -= M[m];
				uint64_t const v = proc01::proc01(UUUUUUUU[m],UUUUUUUU[m+1]);
				
				unsigned int left = 0, right = 1u<<mbbitwidth;
				while ( right-left )
				{
					unsigned int const d = right-left;
					unsigned int const d2 = d>>1;
					unsigned int const mid = left + d2;
					unsigned int const rmid = popcount8( v, mid );

					// number of ones is too small
					if ( rmid < i )
						left = mid+1;
					// number of ones is too large
					else if ( rmid > i )
						right = mid;
					// if this is the leftmost occurence in the interval, return it
					else if ( (!mid) || (popcount8( v, mid-1 ) != i) )
					{
						return (m<<mbbitwidth)+mid;
					}
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
			#endif
		};
	}
}
#endif
