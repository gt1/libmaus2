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

#if ! defined(ERANK222BAPPEND_HPP)
#define ERANK222BAPPEND_HPP

#include <libmaus/rank/ERankBase.hpp>
#include <libmaus/rank/ERank222BBase.hpp>
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
		 * This class starts with an empty bit vector of predefined
		 * maximal length. Bits can be appended up to the predefined
		 * size.
		 **/
		struct ERank222BAppend : public ERankBase, public ERank222BBase
		{
			public:
			typedef ERank222BAppend this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			private:
			uint64_t * const UUUUUUUU;
			uint64_t const n;
			uint64_t const numsuper;
			uint64_t const nummini;
			uint64_t activesuper;
			uint64_t activemini;
			uint64_t * activepointer;
			uint64_t activemask;
			uint64_t nc;
			uint64_t nr;
			
			::libmaus::autoarray::AutoArray<uint64_t> S; // n / 2^16 * 64 bits = n / 2^10 = n/1024 bits
			::libmaus::autoarray::AutoArray<unsigned short> M; // n / 2^16 * 2^16 / 64 * 16 = n/4 bits

			/**
			 * return superblock containing i th 1 bit,
			 * i.e. largest j such that S[j] < ii
			 **/
			uint64_t selectSuper(uint64_t const ii) const
			{
				// search largest superblock index s such that ii < S[s]
				uint64_t left = 0, right = activesuper;

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
			 * return miniblock containing i th 1 bit,
			 * i.e. largest j such that M[j] < ii
			 **/
			uint64_t selectMini(uint64_t const s, uint64_t const iii) const
			{
				uint64_t const ii = iii - S[s];
				uint64_t left = (s << sbbitwidth) >>  mbbitwidth;
				uint64_t right = ::std::min( activemini, ((s+1) << sbbitwidth) >>  mbbitwidth);
			
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
			 * @param rUUUUUUUU bit vector
			 * @param rn number of bits in vector (has to be a multiple of 64)
			 **/
			ERank222BAppend(uint64_t * const rUUUUUUUU, uint64_t const rn) 
			: UUUUUUUU(rUUUUUUUU), n(rn),
			  numsuper( divUp(n,sbsize) ), nummini( divUp(n,mbsize) ),
			  activesuper(0), activemini(0), activepointer(UUUUUUUU-1), activemask(0),
			  nc(0), nr(0),
			  S( numsuper , false ), M( nummini, false)
			{
			}
			
			void appendOneRun(uint64_t j)
			{
				while ( j-- )
					appendBit(true);
			}

			void appendZeroRun(uint64_t j)
			{
				while ( j-- )
					appendBit(false);
			}
			
			void appendBit(bool const b)
			{
				// start new word if activemask is null
				if ( ! activemask )
				{
					*(++activepointer) = 0;
					activemask = (1ull<<63);
				}
				// add bit if it is one
				if ( b )
					*activepointer |= activemask;
				// shift active mask to next position
				activemask >>= 1;
				// update miniblock dictionary if necessary
				if ( !(nc & mbmask) )
				{
					// update superblock dictionary if necessary
					//if ( !(nc & sbmask) ) 
					if ( ! (activemini & sbmbmask) )
						S [ activesuper++ ] = nr;

					M [ activemini++ ] = nr - S [ activesuper-1 ];
				}
				// update rank accu
				if ( b )
					++nr;
				// update position
				++nc;
			}
						
			bool operator[](uint64_t const i) const
			{
				return ::libmaus::bitio::getBit(UUUUUUUU,i);
			}
			
			/**
			 * @return estimated space in bytes
			 **/		
			uint64_t byteSize() const
			{
				return 
					sizeof(uint64_t *) + 
					3*sizeof(uint64_t) +
					S.byteSize() + 
					M.byteSize();
			}
			
			/**
			 * return number of 1 bits up to (and including) index i
			 * @param i
			 * @return population count
			 **/
			uint64_t rank1(uint64_t i) const
			{
				uint64_t const mi = i >> mbbitwidth;
				return S[i >> sbbitwidth] + M[mi] + popcount8( (UUUUUUUU[mi]), i - (mi<<mbbitwidth));
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
			 * Return the position of the ii'th 1 bit. This function is implemented using a 
			 * binary search on the rank1 function.
			 **/
			uint64_t select1(uint64_t const ii) const
			{
				uint64_t i = ii+1;

				uint64_t const s = selectSuper(i);
				uint64_t const m = selectMini(s,i);
				i -= S[s]; i -= M[m];
				uint64_t const v = (UUUUUUUU[m]);
				
				uint64_t left = 0, right = 1u<<mbbitwidth;
				while ( right-left )
				{
					uint64_t const d = right-left;
					uint64_t const d2 = d>>1;
					uint64_t const mid = left + d2;
					uint64_t const rmid = popcount8( v, mid );

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
				
				return nc;
			}
			/**
			 * Return the position of the ii'th 0 bit. This function is implemented using a 
			 * binary search on the rank1 function.
			 **/
			uint64_t select0(uint64_t const ii) const
			{
				uint64_t const i = ii+1;

				uint64_t left = 0, right = nc;
				
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
				
				return nc;		
			}
		};
	}
}
#endif
