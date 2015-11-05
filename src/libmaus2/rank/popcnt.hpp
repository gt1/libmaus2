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

#if ! defined(POPCNT_HPP)
#define POPCNT_HPP

#include <libmaus2/types/types.hpp>
#include <string>
#include <sstream>
#include <iostream>

namespace libmaus2
{
	namespace rank
	{
		template<unsigned int intsize>
		struct PopCnt4Base
		{
			virtual ~PopCnt4Base() {}
			/**
			 * Population count (number of 1 bits)
			 * imported from http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
			 * @param x number
			 * @return number of 1 bits in x
			 **/
			static unsigned int popcnt4(uint32_t v)
			{
				v = v - ((v >> 1) & 0x55555555);                    // reuse input as temporary
				v = (v & 0x33333333) + ((v >> 2) & 0x33333333);     // temp
				return ( ( (v + (v >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24; // count
			}

		};
		template<unsigned int longsize>
		struct PopCnt8Base
		{
			virtual ~PopCnt8Base() {}
			/**
			 * 64 bit extension of code above
			 * @param x number
			 * @return number of 1 bits in x
			 **/
			static unsigned int popcnt8(uint64_t x)
			{
				x = x-( (x>>1) & 0x5555555555555555ull);
				x = (x & 0x3333333333333333ull) + ((x >> 2) & 0x3333333333333333ull);
				x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0full;
				return  (0x0101010101010101ull*x >> 56);
			}
		};


		#if defined(LIBMAUS2_USE_ASSEMBLY) && defined(HAVE_SSE4) && defined(LIBMAUS2_HAVE_x86_64)
		inline unsigned int sse4popcnt(uint32_t data)
		{
			asm volatile("popcnt %0,%0" : "+r" (data));
			return data;
		}
		inline unsigned int sse4popcnt(uint64_t data)
		{
			asm volatile("popcnt %0,%0" : "+r" (data));
			return data;
		}
		template<>
		struct PopCnt4Base<4>
		{
			virtual ~PopCnt4Base() {}

			static unsigned int popcnt4(uint32_t v)
			{
				return sse4popcnt(v);
			}
		};
		template<>
		struct PopCnt8Base<8>
		{
			virtual ~PopCnt8Base() {}

			static unsigned int popcnt8(uint64_t v)
			{
				return sse4popcnt(v);
			}
		};
		#elif defined(LIBMAUS2_USE_ASSEMBLY) && defined(LIBMAUS2_HAVE_SSSE3) && defined(LIBMAUS2_HAVE_x86_64)
		inline unsigned int ssse3popcnt(uint64_t const data)
		{
			// lookup table
			static const uint8_t POPCOUNT_4bit[16] __attribute__((aligned(16))) = {
				/* 0 */ 0,
				/* 1 */ 1,
				/* 2 */ 1,
				/* 3 */ 2,
				/* 4 */ 1,
				/* 5 */ 2,
				/* 6 */ 2,
				/* 7 */ 3,
				/* 8 */ 1,
				/* 9 */ 2,
				/* a */ 2,
				/* b */ 3,
				/* c */ 2,
				/* d */ 3,
				/* e */ 3,
				/* f */ 4
			};

			// mask for 4 lsb
			static const uint8_t MASK_4bit[16] __attribute__((aligned(16))) = {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf};

			uint32_t res;
			asm volatile(
				"movdqa (%1), %%xmm7\n"
				"movdqa (%2), %%xmm6\n"
				"movq %3,%%xmm0\n"
				"movq %%xmm0,%%xmm1\n"
				"psrlw $4, %%xmm1\n"
				"pand %%xmm6, %%xmm0\n"
				"pand %%xmm6, %%xmm1\n"
				"movdqa %%xmm7, %%xmm2\n"
				"movdqa %%xmm7, %%xmm3\n"
				"pshufb %%xmm0, %%xmm2\n" // popcnt lower 4 bits of bytes
				"pshufb %%xmm1, %%xmm3\n" // popcnt upper 4 bits of bytes
				"paddb %%xmm2, %%xmm3\n"  // add counts
				"pxor %%xmm0,%%xmm0\n"    // erase xmm0
				"psadbw %%xmm3, %%xmm0\n" // absolute sum of differences, stored in low 16 bits of xmm0 for both quads
				#if 0 //this is only need when we have two quad words in the input
				"movhlps %%xmm0, %%xmm1\n" // move high of xmm0 to low of xmm1
				"paddd %%xmm0, %%xmm1\n"   // add up
				"movd %%xmm1, %0"
				#else
				"movd %%xmm0, %0"
				#endif
				: "=r" (res) : "r" (&POPCOUNT_4bit[0]), "r" (&MASK_4bit[0]), "r" (data) : "%xmm0", "%xmm1", "%xmm2", "%xmm3", "%xmm6", "%xmm7"
				);

			return res;
		}
		template<>
		struct PopCnt4Base<4>
		{
			virtual ~PopCnt4Base() {}

			static unsigned int popcnt4(uint32_t v)
			{
				return ssse3popcnt(v);
			}
		};
		template<>
		struct PopCnt8Base<8>
		{
			virtual ~PopCnt8Base() {}

			static unsigned int popcnt8(uint64_t v)
			{
				return ssse3popcnt(v);
			}
		};
		#elif defined(__GNUC__)
		template<>
		struct PopCnt4Base<2>
		{
			virtual ~PopCnt4Base() {}

			/**
			 * Population count (number of 1 bits)
			 * using the gnu compilers builtin function for unsigned long types
			 * @param u number
			 * @return number of 1 bits in u
			 **/
			static unsigned int popcnt4(uint32_t const u)
			{
				return __builtin_popcountl(u);
			}
		};
		template<>
		struct PopCnt4Base<4>
		{
			virtual ~PopCnt4Base() {}

			/**
			 * Population count (number of 1 bits)
			 * using the gnu compilers builtin function for unsigned long types
			 * @param u number
			 * @return number of 1 bits in u
			 **/
			static unsigned int popcnt4(uint32_t const u)
			{
				return __builtin_popcount(u);
			}
		};

		template<>
		struct PopCnt8Base<4>
		{
			virtual ~PopCnt8Base() {}

			/**
			 * Population count (number of 1 bits)
			 * using the gnu compilers builtin function for unsigned long types
			 * @param u number
			 * @return number of 1 bits in u
			 **/
			static unsigned int popcnt8(uint64_t const u)
			{
				return (__builtin_popcountl(u>>32)) + (__builtin_popcountl(u&0x00000000FFFFFFFFULL));
			}
		};
		template<>
		struct PopCnt8Base<8>
		{
			virtual ~PopCnt8Base() {}

			/**
			 * Population count (number of 1 bits)
			 * using the gnu compilers builtin function for unsigned long types
			 * @param u number
			 * @return number of 1 bits in u
			 **/
			static unsigned int popcnt8(uint64_t const u)
			{
				return __builtin_popcountl(u);
			}
		};
		#endif

		template<unsigned int intsize>
		struct PopCnt4 : public PopCnt4Base<intsize>
		{
			typedef uint32_t input_type;

			virtual ~PopCnt4() {}

			static unsigned int dataBits()
			{
				return 8*sizeof(input_type);
			}

			static input_type msb()
			{
				return static_cast<input_type>(1) << (dataBits()-1);
			}

			static unsigned int popcnt4(input_type v)
			{
				return PopCnt4Base<intsize>::popcnt4(v);
			}
			static unsigned int popcnt4(input_type v, unsigned int bits)
			{
				return popcnt4( v >> (8*sizeof(input_type)-bits) );
			}

			static std::string printP(input_type v, unsigned int numbits = dataBits() )
			{
				input_type const mask = msb();
				std::string s(numbits,' ');

				for ( unsigned int i = 0; i < numbits; ++i )
				{
					if ( v&mask )
						s[i] = ')';
					else
						s[i] = '(';

					v &= ~mask;
					v <<= 1;
				}

				return s;
			}
		};

		template<unsigned int longsize>
		struct PopCnt8 : public PopCnt8Base<longsize>
		{
			virtual ~PopCnt8() {}

			static unsigned int popcnt8(uint64_t v)
			{
				return PopCnt8Base<longsize>::popcnt8(v);
			}
			static unsigned int popcnt8(uint64_t v, unsigned int bits)
			{
				return popcnt8( v >> (8*sizeof(uint64_t)-bits) );
			}
		};

		static inline unsigned int bitcountpair(uint32_t const n)
		{
			return PopCnt4<sizeof(unsigned int)>::popcnt4(((n >> 1) | n) & 0x55555555ul);
		}

		static inline unsigned int bitcountpair(uint64_t const n)
		{
			return PopCnt8<sizeof(unsigned long)>::popcnt8(((n >> 1) | n) & 0x5555555555555555ull);
		}

		static inline unsigned int diffcountpair(uint32_t const a, uint32_t const b)
		{
			return bitcountpair( a ^ b );
		}

		static inline unsigned int diffcountpair(uint64_t const a, uint64_t const b)
		{
			return bitcountpair( a ^ b );
		}
	}
}
#endif
