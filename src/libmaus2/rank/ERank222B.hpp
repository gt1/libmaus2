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

#if ! defined(ERANK222B_HPP)
#define ERANK222B_HPP

#include <libmaus2/bitio/BitWriter.hpp>
#include <libmaus2/rank/ERankBase.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/util/unique_ptr.hpp>
#include <cassert>
#include <stdexcept>


namespace libmaus2
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
		struct ERank222B : public ERankBase
		{
			public:
			typedef ::libmaus2::bitio::BitWriter8 writer_type;

			typedef ERank222B this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			private:
			ERank222B & operator=(ERank222B const &);
			ERank222B(ERank222B const &);

			private:
			// super block size 2^16 bits
			static unsigned int const sbbitwidth = 16;
			// mini block size 2^6 = 64 bits
			static unsigned int const mbbitwidth = 6;

			// derived numbers
			// actual block sizes
			static uint64_t const sbsize = 1u << sbbitwidth;
			static uint64_t const mbsize = 1u << mbbitwidth;

			// miniblocks per superblock
			static uint64_t const mps = ERANK2DIVUP(sbsize,mbsize);
			// superblock mask
			static uint64_t const sbmask = (1u<<sbbitwidth)-1;
			// miniblock mask
			static uint64_t const mbmask = (1u<<mbbitwidth)-1;
			// superblock/miniblock mask
			static uint64_t const sbmbmask = (1ull << (sbbitwidth-mbbitwidth))-1;
			// superblock/miniblock shift
			static unsigned int const sbmbshift = sbbitwidth-mbbitwidth;

			uint64_t const * const UUUUUUUU;
			uint64_t const n;
			uint64_t const numsuper;
			uint64_t const nummini;

			::libmaus2::autoarray::AutoArray<uint64_t> S; // n / 2^16 * 64 bits = n / 2^10 = n/1024 bits
			::libmaus2::autoarray::AutoArray<unsigned short> M; // n / 2^16 * 2^16 / 64 * 16 = n/4 bits

			static inline uint64_t divUp(uint64_t a, uint64_t b)
			{
				return ERANK2DIVUP(a,b);
			}

			inline uint64_t S0(uint64_t i) const
			{
				return ((i<<sbbitwidth))-S[i];
			}

			/**
			 * return superblock containing i th 1 bit,
			 * i.e. largest j such that S[j] < ii
			 **/
			uint64_t selectSuper1(uint64_t const ii) const
			{
				// search largest superblock index s such that ii < S[s]
				uint64_t left = 0, right = numsuper;

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
			 * return superblock containing 0 th 1 bit,
			 * i.e. largest j such that S0(j) < ii
			 **/
			uint64_t selectSuper0(uint64_t const ii) const
			{
				// search largest superblock index s such that ii < S[s]
				uint64_t left = 0, right = numsuper;

				while ( right-left > 1 )
				{
					uint64_t const d = right-left;
					uint64_t const d2 = d>>1;
					uint64_t const mid = left + d2;

					// number of 1s is too large
					if ( S0(mid) < ii )
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
			uint64_t selectMini1(uint64_t const s, uint64_t const iii) const
			{
				uint64_t const ii = iii - S[s];
				uint64_t left = (s << sbbitwidth) >>  mbbitwidth;
				uint64_t right = ::std::min( nummini, ((s+1) << sbbitwidth) >>  mbbitwidth);

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
			/**
			 * return miniblock containing i th 0 bit,
			 * i.e. largest j such that M0[j] < ii
			 **/
			uint64_t selectMini0(uint64_t const s, uint64_t const iii) const
			{
				uint64_t const ii = iii - S0(s);
				uint64_t left = (s << sbbitwidth) >>  mbbitwidth;
				uint64_t const rleft = left;
				uint64_t right = ::std::min( nummini, ((s+1) << sbbitwidth) >>  mbbitwidth);

				while ( right-left > 1 )
				{
					uint64_t const d = right-left;
					uint64_t const d2 = d>>1;
					uint64_t const mid = left + d2;

					// number of 1s is too large
					if ( ((((mid-rleft) << mbbitwidth)) -  M[mid]) < ii )
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
			ERank222B(uint64_t const * const rUUUUUUUU, uint64_t const rn)
			: UUUUUUUU(rUUUUUUUU), n(rn),
			  numsuper((n + (sbsize-1)) >> sbbitwidth), nummini((n + (mbsize-1)) >> mbbitwidth),
			  S( divUp(n,sbsize) , false ), M( divUp(n,mbsize), false)
			{
				if ( n & mbmask )
					throw ::std::runtime_error("Rank::ERank222B: n is not multiple of miniblock size 64.");

				// miniblock popcnt
				uint64_t c = 0;
				// superblock popcnt
				uint64_t sc = 0;
				// superblock counter
				uint64_t s = 0;

				for ( uint64_t mi = 0; mi != nummini; ++mi )
				{
					uint64_t b = UUUUUUUU[mi];

					if ( (mi & sbmbmask) == 0 )
					{
						assert ( s == (mi >> sbmbshift) );
						sc = c;
						S[ s++ ] = sc;
					}

					M[ mi ] = c - sc;

					assert( sc + M[mi] == c );

					c += popcount8(b);
				}

				// ::std::cerr << "Construction done." << ::std::endl;
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
			uint64_t rank1(uint64_t const i) const
			{
				uint64_t const mi = i >> mbbitwidth;
				return S[i >> sbbitwidth] + M[mi] + popcount8( (UUUUUUUU[mi]), i - (mi<<mbbitwidth));
			}
			/**
			 * return number of 1 bits up to (and excluding) index i
			 * @param i
			 * @return population count
			 **/
			uint64_t rankm1(uint64_t const i) const
			{
				uint64_t const mi = i >> mbbitwidth;
				return S[i >> sbbitwidth] + M[mi] + popcount8m1( (UUUUUUUU[mi]), i - (mi<<mbbitwidth));
			}
			/**
			 * return number of 0 bits up to (and including) index i
			 * @param i
			 * @return inverse population count
			 **/
			uint64_t rank0(uint64_t const i) const
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

				uint64_t const s = selectSuper1(i);
				uint64_t const m = selectMini1(s,i);
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

				return n;
			}
			/**
			 * Return the position of the ii'th 1 bit. This function is implemented using a
			 * binary search on the rank0 function.
			 **/
			uint64_t select0(uint64_t const ii) const
			{
				uint64_t i = ii+1;

				uint64_t const s = selectSuper0(i);
				uint64_t const m = selectMini0(s,i);
				i -= S0(s);
				i -= ((m-s*mps)<<mbbitwidth) - M[m];
				uint64_t const v = ~(UUUUUUUU[m]);

				uint64_t left = 0, right = 1u<<mbbitwidth;
				while ( right-left )
				{
					uint64_t const d = right-left;
					uint64_t const d2 = d>>1;
					uint64_t const mid = left + d2;
					uint64_t const rmid = popcount8( v, mid );

					// number of zeros is too small
					if ( rmid < i )
						left = mid+1;
					// number of zeros is too large
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
			/**
			 * Return the position of the ii'th 0 bit. This function is implemented using a
			 * binary search on the rank0 function.
			 **/
			uint64_t select0slow(uint64_t const ii) const
			{
				uint64_t const i = ii+1;

				uint64_t const s = selectSuper0(i);
				uint64_t const m = selectMini0(s,i);
				uint64_t left = (m << mbbitwidth), right = ::std::min( (m+1)<<mbbitwidth, n);

				while ( (right-left) )
				{
					uint64_t const d = right-left;
					uint64_t const d2 = d>>1;
					uint64_t const mid = left + d2;

					// number of zeros is too small
					if ( rank0(mid) < i )
						left = mid+1;
					// number of zeros is too large
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
