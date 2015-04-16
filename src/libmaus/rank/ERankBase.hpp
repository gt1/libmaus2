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
#if ! defined(ERANKBASE_HPP)
#define ERANKBASE_HPP

#include <libmaus/rank/BSwapBase.hpp>
#include <libmaus/rank/RankTable.hpp>
#include <libmaus/rank/EncodeCache.hpp>
#include <libmaus/rank/DecodeCache.hpp>
#include <libmaus/rank/popcnt.hpp>
#include <libmaus/util/unique_ptr.hpp>

#define ERANK2DIVUP(a,b) ((a+(b-1))/b)

/**
 * \mainpage
 * This package contains some rank dictionaries.
 **/
namespace libmaus
{
	namespace rank
	{
		/**
		 * rank base functions
		 **/
		struct ERankBase : public BSwapBase
		{
			typedef ERankBase this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			private:
			#if defined(RANKTABLES)
			//! lookup table for popcount function
			static RankTable const R;
			static SimpleRankTable const S;
			#endif
			protected:
			/**
			 * cache for encoding enumerative code
			 **/
			static EncodeCache<16,uint16_t> EC16;
			/**
			 * cache for decoding enumerative code
			 **/
			static DecodeCache<16,uint16_t> DC16;
			
			protected:
			
			// #define RTABLE
			// #define HAVE_SSE4
			
			/**
			 * compute population count (number of 1 bits) of 16 bit number val
			 * @param val
			 * @return population count of val
			 **/
			static inline uint16_t popcount2(uint16_t val)
			{
				#if ! defined(RANKTABLES)
				return PopCnt4<sizeof(unsigned int)>::popcnt4(val);
				#else
				return S(val);
				#endif
			}
			/**
			 * compute population count (number of 1 bits) of the i+1 most significant bits of the 16 bit number val
			 * @param val
			 * @param i index of last bit to consider (starting from MSB)
			 * @return population count
			 **/
			static inline uint16_t popcount2(uint16_t val, unsigned int const i)
			{
				#if ! defined(RANKTABLES)
				return PopCnt4<sizeof(unsigned int)>::popcnt4(val >> (15-i));
				#else
				return R(val,i);
				#endif
			}
			/**
			 * compute population count (number of 1 bits) of 32 bit number val
			 * @param val
			 * @return population count of val
			 **/
			static inline uint32_t popcount4(uint32_t val)
			{
				#if ! defined(RANKTABLES)
				return PopCnt4<sizeof(unsigned int)>::popcnt4(val);
				#else
				#if defined(RTABLE)
				return R(val >> 16,15) + R(val & 0xFFFF,15);
				#else
				return S(val >> 16) + S(val & 0xFFFF);
				#endif
				#endif
			}
			/**
			 * compute population count (number of 1 bits) of the i+1 most significant bits of the 32 bit number val
			 * @param val
			 * @param i index of last bit to consider (starting from MSB)
			 * @return population count
			 **/
			static inline uint32_t popcount4(uint32_t val, unsigned int const i)
			{
				#if ! defined(RANKTABLES)
				val >>= (31-i);
				return PopCnt4<sizeof(unsigned int)>::popcnt4(val);
				#else
				#if defined(RTABLE)
				if ( i < 16 )
					return R(val>>16,i);
				else
					return R(val>>16,15) + R(val & 0xFFFF,i-16);
				#else
				return popcount4( val >> (31-i) );
				#endif
				#endif
			}
			/**
			 * compute population count (number of 1 bits) of the i most significant bits of the 32 bit number val
			 * @param val
			 * @param i index of first bit not to consider (starting from MSB)
			 * @return population count
			 **/
			static inline uint32_t popcount4m1(uint32_t val, unsigned int const i)
			{
				static uint32_t const shiftmask[32] =
				{
					// 0-8
					0,
					0xFFFFFFFFUL,
					0xFFFFFFFFUL,
					0xFFFFFFFFUL,
					0xFFFFFFFFUL,
					0xFFFFFFFFUL,
					0xFFFFFFFFUL,
					0xFFFFFFFFUL,

					// 8-16
					0xFFFFFFFFUL,
					0xFFFFFFFFUL,
					0xFFFFFFFFUL,
					0xFFFFFFFFUL,
					0xFFFFFFFFUL,
					0xFFFFFFFFUL,
					0xFFFFFFFFUL,
					0xFFFFFFFFUL,

					// 16-24
					0xFFFFFFFFUL,
					0xFFFFFFFFUL,
					0xFFFFFFFFUL,
					0xFFFFFFFFUL,
					0xFFFFFFFFUL,
					0xFFFFFFFFUL,
					0xFFFFFFFFUL,
					0xFFFFFFFFUL,

					// 24-32
					0xFFFFFFFFUL,
					0xFFFFFFFFUL,
					0xFFFFFFFFUL,
					0xFFFFFFFFUL,
					0xFFFFFFFFUL,
					0xFFFFFFFFUL,
					0xFFFFFFFFUL,
					0xFFFFFFFFUL
				};
			
				#if ! defined(RANKTABLES)
				val >>= (32-i);
				val &= shiftmask[i];
				return PopCnt4<sizeof(unsigned int)>::popcnt4(val);
				#else
				return popcount4( (val >> (32-i)) & shiftmask[i] );
				#endif
			}
			/**
			 * compute population count of the 64 bit number val
			 * @param val
			 * @return population count
			 **/
			static inline uint64_t popcount8(uint64_t val)
			{
				#if ! defined(RANKTABLES)
				return PopCnt8<sizeof(unsigned long)>::popcnt8(val);
				#else
				return popcount4( val >> 32 ) + popcount4( val & 0xFFFFFFFFu );
				#endif
			}
			/**
			 * compute population count (number of 1 bits) of the i+1 most significant bits of the 64 bit number val
			 * @param val
			 * @param i index of last bit to consider (starting from MSB)
			 * @return population count
			 **/
			static inline uint64_t popcount8(uint64_t val, unsigned int const i)
			{
				#if ! defined(RANKTABLES)
				val >>= (63-i);
				return PopCnt8<sizeof(unsigned long)>::popcnt8(val);
				#elif defined(RTABLE)
				if ( i < 32 )
					return popcount4( val >> 32, i);
				else
					return popcount4( val >> 32, 31 ) + popcount4( val & 0xFFFFFFFFu, i - 32);
				#else
				return popcount8( val >> (63-i) );
				#endif
			}
			/**
			 * compute population count (number of 1 bits) of the i most significant bits of the 64 bit number val
			 * @param val
			 * @param i index of first bit not to consider (starting from MSB)
			 * @return population count
			 **/
			static inline uint64_t popcount8m1(uint64_t val, unsigned int const i)
			{
				static uint64_t const shiftmask[64] = 
				{
					// 0-8
					0,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					// 0-16
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					// 16-24
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					// 24-32
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					// 32-40
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					// 40-48
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					// 48-56
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					// 56-64
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
					0xFFFFFFFFFFFFFFFFULL,
				};
			
				#if ! defined(RANKTABLES)
				val >>= (64-i);
				val &= shiftmask[i];
				return PopCnt8<sizeof(unsigned long)>::popcnt8(val);
				#else
				return popcount8( (val >> (64-i)) & shiftmask[i] );
				#endif
			}
			/**
			 * Reorder bytes in 16 bit number val for population count. Inverts byte order on little endian machines, on big endian machines returns val as is.
			 * @param val
			 * @return val reordered
			 **/
			inline static uint16_t reorder2(uint16_t val)
			{
	#if defined(BYTE_ORDER_BIG_ENDIAN)
				return val;
	#else
				return bswap2(val);
	#endif
			}
			/**
			 * Reorder bytes in 32 bit number val for population count. Inverts byte order on little endian machines, on big endian machines returns val as is.
			 * @param val
			 * @return val reordered
			 **/
			inline static uint32_t reorder4(uint32_t val)
			{
	#if defined(BYTE_ORDER_BIG_ENDIAN)
				return val;
	#else
				return bswap4(val);
	#endif
			}
			/**
			 * Reorder bytes in 64 bit number val for population count. Inverts byte order on little endian machines, on big endian machines returns val as is.
			 * @param val
			 * @return val reordered
			 **/
			inline static uint64_t reorder8(uint64_t val)
			{
	#if defined(BYTE_ORDER_BIG_ENDIAN)
				return val;
	#else
				return bswap8(val);
	#endif
			}

			public:
			/**
			 * count number of non-zero bitpairs (0,1) (2,3) ... (30,31)
			 * @param n value
			 * @return number of non-zero bit pairs
			 **/
			inline static unsigned int bitcountpair(uint32_t const n)
			{
				return popcount8(((n >> 1) | n) & 0x55555555ul);
			}
			/**
			 * count number of non-zero bitpairs (0,1) (2,3) ... (62,63)
			 * @param n value
			 * @return number of non-zero bit pairs
			 **/
			inline static unsigned int bitcountpair(uint64_t const n)
			{
				return popcount8(((n >> 1) | n) & 0x5555555555555555ull);
			}
			/**
			 * count number of differing bitpairs (0,1) (2,3) ... (30,31)
			 * @param a first value 
			 * @param b first value 
			 * @return number of differing bit pairs
			 **/
			inline static unsigned int diffcountpair(uint32_t const a, uint32_t const b)
			{
				return bitcountpair( a ^ b );
			}
			/**
			 * count number of differing bitpairs (0,1) (2,3) ... (62,63)
			 * @param a first value 
			 * @param b first value 
			 * @return number of differing bit pairs
			 **/
			inline static unsigned int diffcountpair(uint64_t const a, uint64_t const b)
			{
				return bitcountpair( a ^ b );
			}
		};
	}
}
#endif
