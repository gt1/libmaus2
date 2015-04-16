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

#if ! defined(ERANK222BAPPENDDYNAMIC_HPP)
#define ERANK222BAPPENDDYNAMIC_HPP

#include <libmaus/rank/ERankBase.hpp>
#include <libmaus/rank/ERank222BBase.hpp>
#include <libmaus/bitio/getBit.hpp>
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
		 * This class starts with an empty bit vector. Bits can be appended.
		 **/
		struct ERank222BAppendDynamic : public ERankBase, public ERank222BBase
		{
			public:
			typedef ERank222BAppendDynamic this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			private:
			//! bit vector
			std::vector<uint64_t> UUUUUUUU;
			uint64_t activemask;
			//! total number of bits inserted
			uint64_t nc;
			//! number of 1 bits inserted
			uint64_t nr;
			
			//! super block dictionary
			std::vector<uint64_t> S;
			//! mini block dictionary
			std::vector<uint16_t> M;

			/**
			 * return superblock containing i th 1 bit,
			 * i.e. largest j such that S[j] < ii
			 **/
			uint64_t selectSuper(uint64_t const ii) const
			{
				// search largest superblock index s such that ii < S[s]
				uint64_t left = 0, right = S.size();

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
				uint64_t right = ::std::min( static_cast<uint64_t>(M.size()), ((s+1) << sbbitwidth) >>  mbbitwidth);
			
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
			 *
			 **/
			ERank222BAppendDynamic() : activemask(0), nc(0), nr(0)
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
					// update superblock dictionary if necessary
					if ( ! (M.size() & sbmbmask) )
						S.push_back(nr);
					// update miniblock dictionary
					M.push_back ( nr - S.back() );
					
					UUUUUUUU.push_back(0);
					activemask = (1ull<<63);
				}
				// add bit if it is one
				if ( b )
					UUUUUUUU.back() |= activemask;
				// shift active mask to next position
				activemask >>= 1;
				// update rank accu
				if ( b )
					++nr;
				// update position
				++nc;
			}
						
			bool operator[](uint64_t const i) const
			{
				return ::libmaus::bitio::getBit(&(UUUUUUUU[i >> mbbitwidth]),i & mbmask);
			}
			
			/**
			 * @return length of vector in bits
			 **/
			uint64_t size() const
			{
				return nc;
			}
			
			/**
			 * @return estimated space in bytes
			 **/		
			uint64_t byteSize() const
			{
				return 
					sizeof(uint64_t *) + 
					3*sizeof(uint64_t) +
					S.size() * sizeof(uint64_t) + 
					M.size() * sizeof(uint16_t);
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
