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


#if ! defined(PUTBIT_HPP)
#define PUTBIT_HPP

#include <iterator>
#include <libmaus2/types/types.hpp>

namespace libmaus2
{
	namespace bitio
	{
		/**
		 * put one bit
		 * @param A bit vector
		 * @param offset
		 * @param bit
		 **/
		template<typename iterator>
		inline void putBit1(iterator A, uint64_t const offset, uint8_t v)
		{
			static const uint8_t maskone[] = { 
				static_cast<uint8_t>(~(static_cast<uint8_t>(1)<<7)), static_cast<uint8_t>(~(static_cast<uint8_t>(1)<<6)), 
				static_cast<uint8_t>(~(static_cast<uint8_t>(1)<<5)), static_cast<uint8_t>(~(static_cast<uint8_t>(1)<<4)), 
				static_cast<uint8_t>(~(static_cast<uint8_t>(1)<<3)), static_cast<uint8_t>(~(static_cast<uint8_t>(1)<<2)), 
				static_cast<uint8_t>(~(static_cast<uint8_t>(1)<<1)), static_cast<uint8_t>(~(static_cast<uint8_t>(1)<<0))
			};
			static const uint8_t insone[] = { 0u, 1u<<7,0u,1u<<6,0u,1u<<5,0u,1u<<4,0u,1u<<3,0u,1u<<2,0u,1u<<1,0u,1u<<0 };
			uint64_t const wordoffset = offset>>3;
			uint64_t const bitoffset = offset&0x7u;
			A[wordoffset] = (A[wordoffset] & maskone[bitoffset]) | insone[(bitoffset<<1)|v];
		}
		#if defined(LIBMAUS2_HAVE_SYNC_OPS)
		/**
		 * put one bit synchronous
		 * @param A bit vector
		 * @param offset
		 * @param bit
		 **/
		template<typename iterator>
		inline void putBit1Sync(iterator A, uint64_t const offset, uint8_t v)
		{
			static const uint8_t maskone[] = { 
				static_cast<uint8_t>(~(static_cast<uint8_t>(1)<<7)), static_cast<uint8_t>(~(static_cast<uint8_t>(1)<<6)), 
				static_cast<uint8_t>(~(static_cast<uint8_t>(1)<<5)), static_cast<uint8_t>(~(static_cast<uint8_t>(1)<<4)), 
				static_cast<uint8_t>(~(static_cast<uint8_t>(1)<<3)), static_cast<uint8_t>(~(static_cast<uint8_t>(1)<<2)), 
				static_cast<uint8_t>(~(static_cast<uint8_t>(1)<<1)), static_cast<uint8_t>(~(static_cast<uint8_t>(1)<<0))
			};
			static const uint8_t insone[] = { 0u, 1u<<7,0u,1u<<6,0u,1u<<5,0u,1u<<4,0u,1u<<3,0u,1u<<2,0u,1u<<1,0u,1u<<0 };
			uint64_t const wordoffset = offset>>3;
			uint64_t const bitoffset = offset&0x7u;
			__sync_fetch_and_and(A+wordoffset,maskone[bitoffset]);
			__sync_fetch_and_or (A+wordoffset,insone[(bitoffset<<1)|v]);
		}
		/**
		 * set one bit synchronous
		 * @param A bit vector
		 * @param offset
		 * @param bit
		 **/
		template<typename iterator>
		inline bool setBit1Sync(iterator A, uint64_t const offset)
		{
			static const uint8_t setone[] = { 
				static_cast<uint8_t>(1ull<<7), 
				static_cast<uint8_t>(1ull<<6), 
				static_cast<uint8_t>(1ull<<5), 
				static_cast<uint8_t>(1ull<<4), 
				static_cast<uint8_t>(1ull<<3), 
				static_cast<uint8_t>(1ull<<2), 
				static_cast<uint8_t>(1ull<<1), 
				static_cast<uint8_t>(1ull<<0) };
			uint64_t const wordoffset = offset>>3;
			uint64_t const bitoffset = offset&0x7u;
			return __sync_fetch_and_or (A+wordoffset,setone[bitoffset]) & setone[bitoffset];
		}
		/**
		 * erase one bit synchronous
		 * @param A bit vector
		 * @param offset
		 * @param bit
		 **/
		template<typename iterator>
		inline bool eraseBit1Sync(iterator A, uint64_t const offset)
		{
			static const uint8_t setone[] = { 
				static_cast<uint8_t>(1ull<<7), 
				static_cast<uint8_t>(1ull<<6), 
				static_cast<uint8_t>(1ull<<5), 
				static_cast<uint8_t>(1ull<<4), 
				static_cast<uint8_t>(1ull<<3), 
				static_cast<uint8_t>(1ull<<2), 
				static_cast<uint8_t>(1ull<<1), 
				static_cast<uint8_t>(1ull<<0) 
			};
			static const uint8_t maskone[] = { 
				static_cast<uint8_t>(~(1ull<<7)), 
				static_cast<uint8_t>(~(1ull<<6)), 
				static_cast<uint8_t>(~(1ull<<5)), 
				static_cast<uint8_t>(~(1ull<<4)), 
				static_cast<uint8_t>(~(1ull<<3)), 
				static_cast<uint8_t>(~(1ull<<2)), 
				static_cast<uint8_t>(~(1ull<<1)), 
				static_cast<uint8_t>(~(1ull<<0)) 
			};
			uint64_t const wordoffset = offset>>3;
			uint64_t const bitoffset = offset&0x7u;
			return __sync_fetch_and_and (A+wordoffset,maskone[bitoffset]) & setone[bitoffset];
		}
		#endif
		/**
		 * put one bit
		 * @param A bit vector
		 * @param offset
		 * @param bit
		 **/
		template<typename iterator>
		inline void putBit2(iterator A, uint64_t const offset, uint16_t v)
		{
			static const uint16_t maskone[] = { 
				static_cast<uint16_t>(~(static_cast<uint16_t>(1)<<15)), static_cast<uint16_t>(~(static_cast<uint16_t>(1)<<14)), static_cast<uint16_t>(~(static_cast<uint16_t>(1)<<13)), 
				static_cast<uint16_t>(~(static_cast<uint16_t>(1)<<12)), static_cast<uint16_t>(~(static_cast<uint16_t>(1)<<11)), static_cast<uint16_t>(~(static_cast<uint16_t>(1)<<10)), 
				static_cast<uint16_t>(~(static_cast<uint16_t>(1)<<9)),  static_cast<uint16_t>(~(static_cast<uint16_t>(1)<<8)),  static_cast<uint16_t>(~(static_cast<uint16_t>(1)<<7)), 
				static_cast<uint16_t>(~(static_cast<uint16_t>(1)<<6)),  static_cast<uint16_t>(~(static_cast<uint16_t>(1)<<5)),  static_cast<uint16_t>(~(static_cast<uint16_t>(1)<<4)), 
				static_cast<uint16_t>(~(static_cast<uint16_t>(1)<<3)),  static_cast<uint16_t>(~(static_cast<uint16_t>(1)<<2)),  static_cast<uint16_t>(~(static_cast<uint16_t>(1)<<1)), 
				static_cast<uint16_t>(~(static_cast<uint16_t>(1)<<0))
			};
			static const uint16_t insone[] = { 
				0u, (static_cast<uint16_t>(1)<<15), 0u, (static_cast<uint16_t>(1)<<14), 
				0u, (static_cast<uint16_t>(1)<<13), 0u, (static_cast<uint16_t>(1)<<12), 
				0u, (static_cast<uint16_t>(1)<<11), 0u, (static_cast<uint16_t>(1)<<10), 
				0u, (static_cast<uint16_t>(1)<<9),  0u, (static_cast<uint16_t>(1)<<8), 
				0u, (static_cast<uint16_t>(1)<<7), 0u, (static_cast<uint16_t>(1)<<6),
				0u, (static_cast<uint16_t>(1)<<5), 0u, (static_cast<uint16_t>(1)<<4),
				0u, (static_cast<uint16_t>(1)<<3), 0u, (static_cast<uint16_t>(1)<<2),
				0u, (static_cast<uint16_t>(1)<<1), 0u, (static_cast<uint16_t>(1)<<0)
			};
			uint64_t const wordoffset = offset>>4;
			uint64_t const bitoffset = offset&0xfu;
			A[wordoffset] = (A[wordoffset] & maskone[bitoffset]) | insone[(bitoffset<<1)|v];
		}
		#if defined(LIBMAUS2_HAVE_SYNC_OPS)
		/**
		 * put one bit
		 * @param A bit vector
		 * @param offset
		 * @param bit
		 **/
		template<typename iterator>
		inline void putBit2Sync(iterator A, uint64_t const offset, uint16_t v)
		{
			static const uint16_t maskone[] = { 
				static_cast<uint16_t>(~(static_cast<uint16_t>(1)<<15)), static_cast<uint16_t>(~(static_cast<uint16_t>(1)<<14)), static_cast<uint16_t>(~(static_cast<uint16_t>(1)<<13)), 
				static_cast<uint16_t>(~(static_cast<uint16_t>(1)<<12)), static_cast<uint16_t>(~(static_cast<uint16_t>(1)<<11)), static_cast<uint16_t>(~(static_cast<uint16_t>(1)<<10)), 
				static_cast<uint16_t>(~(static_cast<uint16_t>(1)<<9)),  static_cast<uint16_t>(~(static_cast<uint16_t>(1)<<8)),  static_cast<uint16_t>(~(static_cast<uint16_t>(1)<<7)), 
				static_cast<uint16_t>(~(static_cast<uint16_t>(1)<<6)),  static_cast<uint16_t>(~(static_cast<uint16_t>(1)<<5)),  static_cast<uint16_t>(~(static_cast<uint16_t>(1)<<4)), 
				static_cast<uint16_t>(~(static_cast<uint16_t>(1)<<3)),  static_cast<uint16_t>(~(static_cast<uint16_t>(1)<<2)),  static_cast<uint16_t>(~(static_cast<uint16_t>(1)<<1)), 
				static_cast<uint16_t>(~(static_cast<uint16_t>(1)<<0))
			};
			static const uint16_t insone[] = { 
				0u, (static_cast<uint16_t>(1)<<15), 0u, (static_cast<uint16_t>(1)<<14), 
				0u, (static_cast<uint16_t>(1)<<13), 0u, (static_cast<uint16_t>(1)<<12), 
				0u, (static_cast<uint16_t>(1)<<11), 0u, (static_cast<uint16_t>(1)<<10), 
				0u, (static_cast<uint16_t>(1)<<9),  0u, (static_cast<uint16_t>(1)<<8), 
				0u, (static_cast<uint16_t>(1)<<7), 0u, (static_cast<uint16_t>(1)<<6),
				0u, (static_cast<uint16_t>(1)<<5), 0u, (static_cast<uint16_t>(1)<<4),
				0u, (static_cast<uint16_t>(1)<<3), 0u, (static_cast<uint16_t>(1)<<2),
				0u, (static_cast<uint16_t>(1)<<1), 0u, (static_cast<uint16_t>(1)<<0)
			};
			uint64_t const wordoffset = offset>>4;
			uint64_t const bitoffset = offset&0xfu;
			__sync_fetch_and_and(A+wordoffset,maskone[bitoffset]);
			__sync_fetch_and_or (A+wordoffset,insone[(bitoffset<<1)|v]);
		}		


		/**
		 * set one bit synchronous
		 * @param A bit vector
		 * @param offset
		 * @param bit
		 **/
		template<typename iterator>
		inline bool setBit2Sync(iterator A, uint64_t const offset)
		{
			static const uint16_t setone[] = { 
				static_cast<uint16_t>(1ull<<15), 
				static_cast<uint16_t>(1ull<<14), 
				static_cast<uint16_t>(1ull<<13), 
				static_cast<uint16_t>(1ull<<12), 
				static_cast<uint16_t>(1ull<<11), 
				static_cast<uint16_t>(1ull<<10), 
				static_cast<uint16_t>(1ull<<9), 
				static_cast<uint16_t>(1ull<<8),
				static_cast<uint16_t>(1ull<<7), 
				static_cast<uint16_t>(1ull<<6), 
				static_cast<uint16_t>(1ull<<5), 
				static_cast<uint16_t>(1ull<<4), 
				static_cast<uint16_t>(1ull<<3), 
				static_cast<uint16_t>(1ull<<2), 
				static_cast<uint16_t>(1ull<<1), 
				static_cast<uint16_t>(1ull<<0) 
			};
			uint64_t const wordoffset = offset>>4;
			uint64_t const bitoffset = offset&0xfu;
			return __sync_fetch_and_or (A+wordoffset,setone[bitoffset]) & setone[bitoffset];
		}
		/**
		 * erase one bit synchronous
		 * @param A bit vector
		 * @param offset
		 * @param bit
		 **/
		template<typename iterator>
		inline bool eraseBit2Sync(iterator A, uint64_t const offset)
		{
			static const uint16_t setone[] = { 
				static_cast<uint16_t>(1ull<<15), 
				static_cast<uint16_t>(1ull<<14), 
				static_cast<uint16_t>(1ull<<13), 
				static_cast<uint16_t>(1ull<<12), 
				static_cast<uint16_t>(1ull<<11), 
				static_cast<uint16_t>(1ull<<10), 
				static_cast<uint16_t>(1ull<<9), 
				static_cast<uint16_t>(1ull<<8), 
				static_cast<uint16_t>(1ull<<7), 
				static_cast<uint16_t>(1ull<<6), 
				static_cast<uint16_t>(1ull<<5), 
				static_cast<uint16_t>(1ull<<4), 
				static_cast<uint16_t>(1ull<<3), 
				static_cast<uint16_t>(1ull<<2), 
				static_cast<uint16_t>(1ull<<1), 
				static_cast<uint16_t>(1ull<<0) 
			};
			static const uint16_t maskone[] = { 
				static_cast<uint16_t>(~(1ull<<15)), 
				static_cast<uint16_t>(~(1ull<<14)), 
				static_cast<uint16_t>(~(1ull<<13)), 
				static_cast<uint16_t>(~(1ull<<12)), 
				static_cast<uint16_t>(~(1ull<<11)), 
				static_cast<uint16_t>(~(1ull<<10)), 
				static_cast<uint16_t>(~(1ull<<9)), 
				static_cast<uint16_t>(~(1ull<<8)),
				static_cast<uint16_t>(~(1ull<<7)), 
				static_cast<uint16_t>(~(1ull<<6)), 
				static_cast<uint16_t>(~(1ull<<5)), 
				static_cast<uint16_t>(~(1ull<<4)), 
				static_cast<uint16_t>(~(1ull<<3)), 
				static_cast<uint16_t>(~(1ull<<2)), 
				static_cast<uint16_t>(~(1ull<<1)), 
				static_cast<uint16_t>(~(1ull<<0)) 
			};
			uint64_t const wordoffset = offset>>4;
			uint64_t const bitoffset = offset&0xFu;
			return __sync_fetch_and_and (A+wordoffset,maskone[bitoffset]) & setone[bitoffset];
		}
		#endif
		/**
		 * put one bit
		 * @param A bit vector
		 * @param offset
		 * @param bit
		 **/
		template<typename iterator>
		inline void putBit4(iterator A, uint64_t const offset, uint32_t v)
		{
			static const uint32_t maskone[] = { 
				~(static_cast<uint32_t>(1)<<31), ~(static_cast<uint32_t>(1)<<30), ~(static_cast<uint32_t>(1)<<29), ~(static_cast<uint32_t>(1)<<28), ~(static_cast<uint32_t>(1)<<27), ~(static_cast<uint32_t>(1)<<26), ~(static_cast<uint32_t>(1)<<25), ~(static_cast<uint32_t>(1)<<24),
				~(static_cast<uint32_t>(1)<<23), ~(static_cast<uint32_t>(1)<<22), ~(static_cast<uint32_t>(1)<<21), ~(static_cast<uint32_t>(1)<<20), ~(static_cast<uint32_t>(1)<<19), ~(static_cast<uint32_t>(1)<<18), ~(static_cast<uint32_t>(1)<<17), ~(static_cast<uint32_t>(1)<<16),
				~(static_cast<uint32_t>(1)<<15), ~(static_cast<uint32_t>(1)<<14), ~(static_cast<uint32_t>(1)<<13), ~(static_cast<uint32_t>(1)<<12), ~(static_cast<uint32_t>(1)<<11), ~(static_cast<uint32_t>(1)<<10), ~(static_cast<uint32_t>(1)<<9), ~(static_cast<uint32_t>(1)<<8),
				~(static_cast<uint32_t>(1)<<7), ~(static_cast<uint32_t>(1)<<6), ~(static_cast<uint32_t>(1)<<5), ~(static_cast<uint32_t>(1)<<4), ~(static_cast<uint32_t>(1)<<3), ~(static_cast<uint32_t>(1)<<2), ~(static_cast<uint32_t>(1)<<1), ~(static_cast<uint32_t>(1)<<0) 
			};
			static const uint32_t insone[] = { 
				0u, (static_cast<uint32_t>(1)<<31), 0u, (static_cast<uint32_t>(1)<<30), 0u, (static_cast<uint32_t>(1)<<29), 0u, (static_cast<uint32_t>(1)<<28), 0u, (static_cast<uint32_t>(1)<<27), 0u, (static_cast<uint32_t>(1)<<26), 0u, (static_cast<uint32_t>(1)<<25), 0u, (static_cast<uint32_t>(1)<<24), 
				0u, (static_cast<uint32_t>(1)<<23), 0u, (static_cast<uint32_t>(1)<<22), 0u, (static_cast<uint32_t>(1)<<21), 0u, (static_cast<uint32_t>(1)<<20), 0u, (static_cast<uint32_t>(1)<<19), 0u, (static_cast<uint32_t>(1)<<18), 0u, (static_cast<uint32_t>(1)<<17), 0u, (static_cast<uint32_t>(1)<<16), 
				0u, (static_cast<uint32_t>(1)<<15), 0u, (static_cast<uint32_t>(1)<<14), 0u, (static_cast<uint32_t>(1)<<13), 0u, (static_cast<uint32_t>(1)<<12), 0u, (static_cast<uint32_t>(1)<<11), 0u, (static_cast<uint32_t>(1)<<10), 0u, (static_cast<uint32_t>(1)<<9), 0u, (static_cast<uint32_t>(1)<<8), 
				0u, (static_cast<uint32_t>(1)<<7),0u,(static_cast<uint32_t>(1)<<6),0u,(static_cast<uint32_t>(1)<<5),0u,(static_cast<uint32_t>(1)<<4),0u,(static_cast<uint32_t>(1)<<3),0u,(static_cast<uint32_t>(1)<<2),0u,(static_cast<uint32_t>(1)<<1),0u,(static_cast<uint32_t>(1)<<0)
			};
			uint64_t const wordoffset = offset>>5;
			uint64_t const bitoffset = offset&0x1fu;
			A[wordoffset] = (A[wordoffset] & maskone[bitoffset]) | insone[(bitoffset<<1)|v];
		}
		#if defined(LIBMAUS2_HAVE_SYNC_OPS)
		/**
		 * put one bit
		 * @param A bit vector
		 * @param offset
		 * @param bit
		 **/
		template<typename iterator>
		inline void putBit4Sync(iterator A, uint64_t const offset, uint32_t v)
		{
			static const uint32_t maskone[] = { 
				~(static_cast<uint32_t>(1)<<31), ~(static_cast<uint32_t>(1)<<30), ~(static_cast<uint32_t>(1)<<29), ~(static_cast<uint32_t>(1)<<28), ~(static_cast<uint32_t>(1)<<27), ~(static_cast<uint32_t>(1)<<26), ~(static_cast<uint32_t>(1)<<25), ~(static_cast<uint32_t>(1)<<24),
				~(static_cast<uint32_t>(1)<<23), ~(static_cast<uint32_t>(1)<<22), ~(static_cast<uint32_t>(1)<<21), ~(static_cast<uint32_t>(1)<<20), ~(static_cast<uint32_t>(1)<<19), ~(static_cast<uint32_t>(1)<<18), ~(static_cast<uint32_t>(1)<<17), ~(static_cast<uint32_t>(1)<<16),
				~(static_cast<uint32_t>(1)<<15), ~(static_cast<uint32_t>(1)<<14), ~(static_cast<uint32_t>(1)<<13), ~(static_cast<uint32_t>(1)<<12), ~(static_cast<uint32_t>(1)<<11), ~(static_cast<uint32_t>(1)<<10), ~(static_cast<uint32_t>(1)<<9), ~(static_cast<uint32_t>(1)<<8),
				~(static_cast<uint32_t>(1)<<7), ~(static_cast<uint32_t>(1)<<6), ~(static_cast<uint32_t>(1)<<5), ~(static_cast<uint32_t>(1)<<4), ~(static_cast<uint32_t>(1)<<3), ~(static_cast<uint32_t>(1)<<2), ~(static_cast<uint32_t>(1)<<1), ~(static_cast<uint32_t>(1)<<0) 
			};
			static const uint32_t insone[] = { 
				0u, (static_cast<uint32_t>(1)<<31), 0u, (static_cast<uint32_t>(1)<<30), 0u, (static_cast<uint32_t>(1)<<29), 0u, (static_cast<uint32_t>(1)<<28), 0u, (static_cast<uint32_t>(1)<<27), 0u, (static_cast<uint32_t>(1)<<26), 0u, (static_cast<uint32_t>(1)<<25), 0u, (static_cast<uint32_t>(1)<<24), 
				0u, (static_cast<uint32_t>(1)<<23), 0u, (static_cast<uint32_t>(1)<<22), 0u, (static_cast<uint32_t>(1)<<21), 0u, (static_cast<uint32_t>(1)<<20), 0u, (static_cast<uint32_t>(1)<<19), 0u, (static_cast<uint32_t>(1)<<18), 0u, (static_cast<uint32_t>(1)<<17), 0u, (static_cast<uint32_t>(1)<<16), 
				0u, (static_cast<uint32_t>(1)<<15), 0u, (static_cast<uint32_t>(1)<<14), 0u, (static_cast<uint32_t>(1)<<13), 0u, (static_cast<uint32_t>(1)<<12), 0u, (static_cast<uint32_t>(1)<<11), 0u, (static_cast<uint32_t>(1)<<10), 0u, (static_cast<uint32_t>(1)<<9), 0u, (static_cast<uint32_t>(1)<<8), 
				0u, (static_cast<uint32_t>(1)<<7),0u,(static_cast<uint32_t>(1)<<6),0u,(static_cast<uint32_t>(1)<<5),0u,(static_cast<uint32_t>(1)<<4),0u,(static_cast<uint32_t>(1)<<3),0u,(static_cast<uint32_t>(1)<<2),0u,(static_cast<uint32_t>(1)<<1),0u,(static_cast<uint32_t>(1)<<0)
			};
			uint64_t const wordoffset = offset>>5;
			uint64_t const bitoffset = offset&0x1fu;
			__sync_fetch_and_and(A+wordoffset,maskone[bitoffset]);
			__sync_fetch_and_or (A+wordoffset,insone[(bitoffset<<1)|v]);
		}

		/**
		 * set one bit synchronous
		 * @param A bit vector
		 * @param offset
		 * @param bit
		 **/
		template<typename iterator>
		inline bool setBit4Sync(iterator A, uint64_t const offset)
		{
			static const uint32_t setone[] = { 
				static_cast<uint32_t>(1ull<<31), 
				static_cast<uint32_t>(1ull<<30), 
				static_cast<uint32_t>(1ull<<29), 
				static_cast<uint32_t>(1ull<<28), 
				static_cast<uint32_t>(1ull<<27), 
				static_cast<uint32_t>(1ull<<26), 
				static_cast<uint32_t>(1ull<<25), 
				static_cast<uint32_t>(1ull<<24),
				static_cast<uint32_t>(1ull<<23), 
				static_cast<uint32_t>(1ull<<22), 
				static_cast<uint32_t>(1ull<<21), 
				static_cast<uint32_t>(1ull<<20), 
				static_cast<uint32_t>(1ull<<19), 
				static_cast<uint32_t>(1ull<<18), 
				static_cast<uint32_t>(1ull<<17), 
				static_cast<uint32_t>(1ull<<16),
				static_cast<uint32_t>(1ull<<15), 
				static_cast<uint32_t>(1ull<<14), 
				static_cast<uint32_t>(1ull<<13), 
				static_cast<uint32_t>(1ull<<12), 
				static_cast<uint32_t>(1ull<<11), 
				static_cast<uint32_t>(1ull<<10), 
				static_cast<uint32_t>(1ull<<9), 
				static_cast<uint32_t>(1ull<<8),
				static_cast<uint32_t>(1ull<<7), 
				static_cast<uint32_t>(1ull<<6), 
				static_cast<uint32_t>(1ull<<5), 
				static_cast<uint32_t>(1ull<<4), 
				static_cast<uint32_t>(1ull<<3), 
				static_cast<uint32_t>(1ull<<2), 
				static_cast<uint32_t>(1ull<<1), 
				static_cast<uint32_t>(1ull<<0) 
			};
			uint64_t const wordoffset = offset>>5;
			uint64_t const bitoffset = offset&0x1fu;
			return __sync_fetch_and_or (A+wordoffset,setone[bitoffset]) & setone[bitoffset];
		}
		/**
		 * erase one bit synchronous
		 * @param A bit vector
		 * @param offset
		 * @param bit
		 **/
		template<typename iterator>
		inline bool eraseBit4Sync(iterator A, uint64_t const offset)
		{
			static const uint32_t setone[] = { 
				static_cast<uint32_t>(1ull<<31), 
				static_cast<uint32_t>(1ull<<30), 
				static_cast<uint32_t>(1ull<<29), 
				static_cast<uint32_t>(1ull<<28), 
				static_cast<uint32_t>(1ull<<27), 
				static_cast<uint32_t>(1ull<<26), 
				static_cast<uint32_t>(1ull<<25), 
				static_cast<uint32_t>(1ull<<24), 
				static_cast<uint32_t>(1ull<<23), 
				static_cast<uint32_t>(1ull<<22), 
				static_cast<uint32_t>(1ull<<21), 
				static_cast<uint32_t>(1ull<<20), 
				static_cast<uint32_t>(1ull<<19), 
				static_cast<uint32_t>(1ull<<18), 
				static_cast<uint32_t>(1ull<<17), 
				static_cast<uint32_t>(1ull<<16), 
				static_cast<uint32_t>(1ull<<15), 
				static_cast<uint32_t>(1ull<<14), 
				static_cast<uint32_t>(1ull<<13), 
				static_cast<uint32_t>(1ull<<12), 
				static_cast<uint32_t>(1ull<<11), 
				static_cast<uint32_t>(1ull<<10), 
				static_cast<uint32_t>(1ull<<9), 
				static_cast<uint32_t>(1ull<<8), 
				static_cast<uint32_t>(1ull<<7), 
				static_cast<uint32_t>(1ull<<6), 
				static_cast<uint32_t>(1ull<<5), 
				static_cast<uint32_t>(1ull<<4), 
				static_cast<uint32_t>(1ull<<3), 
				static_cast<uint32_t>(1ull<<2), 
				static_cast<uint32_t>(1ull<<1), 
				static_cast<uint32_t>(1ull<<0) 
			};
			static const uint32_t maskone[] = { 
				static_cast<uint32_t>(~(1ull<<31)), 
				static_cast<uint32_t>(~(1ull<<30)), 
				static_cast<uint32_t>(~(1ull<<29)), 
				static_cast<uint32_t>(~(1ull<<28)), 
				static_cast<uint32_t>(~(1ull<<27)), 
				static_cast<uint32_t>(~(1ull<<26)), 
				static_cast<uint32_t>(~(1ull<<25)), 
				static_cast<uint32_t>(~(1ull<<24)),
				static_cast<uint32_t>(~(1ull<<23)), 
				static_cast<uint32_t>(~(1ull<<22)), 
				static_cast<uint32_t>(~(1ull<<21)), 
				static_cast<uint32_t>(~(1ull<<20)), 
				static_cast<uint32_t>(~(1ull<<19)), 
				static_cast<uint32_t>(~(1ull<<18)), 
				static_cast<uint32_t>(~(1ull<<17)), 
				static_cast<uint32_t>(~(1ull<<16)),
				static_cast<uint32_t>(~(1ull<<15)), 
				static_cast<uint32_t>(~(1ull<<14)), 
				static_cast<uint32_t>(~(1ull<<13)), 
				static_cast<uint32_t>(~(1ull<<12)), 
				static_cast<uint32_t>(~(1ull<<11)), 
				static_cast<uint32_t>(~(1ull<<10)), 
				static_cast<uint32_t>(~(1ull<<9)), 
				static_cast<uint32_t>(~(1ull<<8)),
				static_cast<uint32_t>(~(1ull<<7)), 
				static_cast<uint32_t>(~(1ull<<6)), 
				static_cast<uint32_t>(~(1ull<<5)), 
				static_cast<uint32_t>(~(1ull<<4)), 
				static_cast<uint32_t>(~(1ull<<3)), 
				static_cast<uint32_t>(~(1ull<<2)), 
				static_cast<uint32_t>(~(1ull<<1)), 
				static_cast<uint32_t>(~(1ull<<0)) 
			};
			uint64_t const wordoffset = offset>>5;
			uint64_t const bitoffset = offset&0x1Fu;
			return __sync_fetch_and_and (A+wordoffset,maskone[bitoffset]) & setone[bitoffset];
		}
		#endif
		/**
		 * put one bit
		 * @param A bit vector
		 * @param offset
		 * @param bit
		 **/
		template<typename iterator>
		inline void putBit8(iterator A, uint64_t const offset, uint64_t v)
		{
			static const uint64_t maskone[] = { 
				~(1ULL<<63), ~(1ULL<<62), ~(1ULL<<61), ~(1ULL<<60), ~(1ULL<<59), ~(1ULL<<58), ~(1ULL<<57), ~(1ULL<<56),
				~(1ULL<<55), ~(1ULL<<54), ~(1ULL<<53), ~(1ULL<<52), ~(1ULL<<51), ~(1ULL<<50), ~(1ULL<<49), ~(1ULL<<48),
				~(1ULL<<47), ~(1ULL<<46), ~(1ULL<<45), ~(1ULL<<44), ~(1ULL<<43), ~(1ULL<<42), ~(1ULL<<41), ~(1ULL<<40),
				~(1ULL<<39), ~(1ULL<<38), ~(1ULL<<37), ~(1ULL<<36), ~(1ULL<<35), ~(1ULL<<34), ~(1ULL<<33), ~(1ULL<<32),
				~(1ULL<<31), ~(1ULL<<30), ~(1ULL<<29), ~(1ULL<<28), ~(1ULL<<27), ~(1ULL<<26), ~(1ULL<<25), ~(1ULL<<24),
				~(1ULL<<23), ~(1ULL<<22), ~(1ULL<<21), ~(1ULL<<20), ~(1ULL<<19), ~(1ULL<<18), ~(1ULL<<17), ~(1ULL<<16),
				~(1ULL<<15), ~(1ULL<<14), ~(1ULL<<13), ~(1ULL<<12), ~(1ULL<<11), ~(1ULL<<10), ~(1ULL<<9), ~(1ULL<<8),
				~(1ULL<<7), ~(1ULL<<6), ~(1ULL<<5), ~(1ULL<<4), ~(1ULL<<3), ~(1ULL<<2), ~(1ULL<<1), ~(1ULL<<0) 
			};
			static const uint64_t insone[] = { 
				0ULL, (1ULL<<63), 0ULL, (1ULL<<62), 0ULL, (1ULL<<61), 0ULL, (1ULL<<60), 0ULL, (1ULL<<59), 0ULL, (1ULL<<58), 0ULL, (1ULL<<57), 0ULL, (1ULL<<56), 
				0ULL, (1ULL<<55), 0ULL, (1ULL<<54), 0ULL, (1ULL<<53), 0ULL, (1ULL<<52), 0ULL, (1ULL<<51), 0ULL, (1ULL<<50), 0ULL, (1ULL<<49), 0ULL, (1ULL<<48), 
				0ULL, (1ULL<<47), 0ULL, (1ULL<<46), 0ULL, (1ULL<<45), 0ULL, (1ULL<<44), 0ULL, (1ULL<<43), 0ULL, (1ULL<<42), 0ULL, (1ULL<<41), 0ULL, (1ULL<<40),
				0ULL, (1ULL<<39), 0ULL, (1ULL<<38), 0ULL, (1ULL<<37), 0ULL, (1ULL<<36), 0ULL, (1ULL<<35), 0ULL, (1ULL<<34), 0ULL, (1ULL<<33), 0ULL, (1ULL<<32), 
				0ULL, (1ULL<<31), 0ULL, (1ULL<<30), 0ULL, (1ULL<<29), 0ULL, (1ULL<<28), 0ULL, (1ULL<<27), 0ULL, (1ULL<<26), 0ULL, (1ULL<<25), 0ULL, (1ULL<<24), 
				0ULL, (1ULL<<23), 0ULL, (1ULL<<22), 0ULL, (1ULL<<21), 0ULL, (1ULL<<20), 0ULL, (1ULL<<19), 0ULL, (1ULL<<18), 0ULL, (1ULL<<17), 0ULL, (1ULL<<16), 
				0ULL, (1ULL<<15), 0ULL, (1ULL<<14), 0ULL, (1ULL<<13), 0ULL, (1ULL<<12), 0ULL, (1ULL<<11), 0ULL, (1ULL<<10), 0ULL, (1ULL<<9), 0ULL, (1ULL<<8),
				0ULL, 1ULL<<7,0ULL,1ULL<<6,0ULL,1ULL<<5,0ULL,1ULL<<4,0ULL,1ULL<<3,0ULL,1ULL<<2,0ULL,1ULL<<1,0ULL,1ULL<<0,0ULL 
			};
			
			uint64_t const wordoffset = offset>>6;
			uint64_t const bitoffset = offset&0x3fu;
			
			A[wordoffset] = (A[wordoffset] & maskone[bitoffset]) | insone[(bitoffset<<1)|v];
		}
		#if defined(LIBMAUS2_HAVE_SYNC_OPS)
		/**
		 * put one bit
		 * @param A bit vector
		 * @param offset
		 * @param bit
		 **/
		template<typename iterator>
		inline void putBit8Sync(iterator A, uint64_t const offset, uint64_t v)
		{
			static const uint64_t maskone[] = { 
				~(1ULL<<63), ~(1ULL<<62), ~(1ULL<<61), ~(1ULL<<60), ~(1ULL<<59), ~(1ULL<<58), ~(1ULL<<57), ~(1ULL<<56),
				~(1ULL<<55), ~(1ULL<<54), ~(1ULL<<53), ~(1ULL<<52), ~(1ULL<<51), ~(1ULL<<50), ~(1ULL<<49), ~(1ULL<<48),
				~(1ULL<<47), ~(1ULL<<46), ~(1ULL<<45), ~(1ULL<<44), ~(1ULL<<43), ~(1ULL<<42), ~(1ULL<<41), ~(1ULL<<40),
				~(1ULL<<39), ~(1ULL<<38), ~(1ULL<<37), ~(1ULL<<36), ~(1ULL<<35), ~(1ULL<<34), ~(1ULL<<33), ~(1ULL<<32),
				~(1ULL<<31), ~(1ULL<<30), ~(1ULL<<29), ~(1ULL<<28), ~(1ULL<<27), ~(1ULL<<26), ~(1ULL<<25), ~(1ULL<<24),
				~(1ULL<<23), ~(1ULL<<22), ~(1ULL<<21), ~(1ULL<<20), ~(1ULL<<19), ~(1ULL<<18), ~(1ULL<<17), ~(1ULL<<16),
				~(1ULL<<15), ~(1ULL<<14), ~(1ULL<<13), ~(1ULL<<12), ~(1ULL<<11), ~(1ULL<<10), ~(1ULL<<9), ~(1ULL<<8),
				~(1ULL<<7), ~(1ULL<<6), ~(1ULL<<5), ~(1ULL<<4), ~(1ULL<<3), ~(1ULL<<2), ~(1ULL<<1), ~(1ULL<<0) 
			};
			static const uint64_t insone[] = { 
				0ULL, (1ULL<<63), 0ULL, (1ULL<<62), 0ULL, (1ULL<<61), 0ULL, (1ULL<<60), 0ULL, (1ULL<<59), 0ULL, (1ULL<<58), 0ULL, (1ULL<<57), 0ULL, (1ULL<<56), 
				0ULL, (1ULL<<55), 0ULL, (1ULL<<54), 0ULL, (1ULL<<53), 0ULL, (1ULL<<52), 0ULL, (1ULL<<51), 0ULL, (1ULL<<50), 0ULL, (1ULL<<49), 0ULL, (1ULL<<48), 
				0ULL, (1ULL<<47), 0ULL, (1ULL<<46), 0ULL, (1ULL<<45), 0ULL, (1ULL<<44), 0ULL, (1ULL<<43), 0ULL, (1ULL<<42), 0ULL, (1ULL<<41), 0ULL, (1ULL<<40),
				0ULL, (1ULL<<39), 0ULL, (1ULL<<38), 0ULL, (1ULL<<37), 0ULL, (1ULL<<36), 0ULL, (1ULL<<35), 0ULL, (1ULL<<34), 0ULL, (1ULL<<33), 0ULL, (1ULL<<32), 
				0ULL, (1ULL<<31), 0ULL, (1ULL<<30), 0ULL, (1ULL<<29), 0ULL, (1ULL<<28), 0ULL, (1ULL<<27), 0ULL, (1ULL<<26), 0ULL, (1ULL<<25), 0ULL, (1ULL<<24), 
				0ULL, (1ULL<<23), 0ULL, (1ULL<<22), 0ULL, (1ULL<<21), 0ULL, (1ULL<<20), 0ULL, (1ULL<<19), 0ULL, (1ULL<<18), 0ULL, (1ULL<<17), 0ULL, (1ULL<<16), 
				0ULL, (1ULL<<15), 0ULL, (1ULL<<14), 0ULL, (1ULL<<13), 0ULL, (1ULL<<12), 0ULL, (1ULL<<11), 0ULL, (1ULL<<10), 0ULL, (1ULL<<9), 0ULL, (1ULL<<8),
				0ULL, 1ULL<<7,0ULL,1ULL<<6,0ULL,1ULL<<5,0ULL,1ULL<<4,0ULL,1ULL<<3,0ULL,1ULL<<2,0ULL,1ULL<<1,0ULL,1ULL<<0,0ULL 
			};
			
			uint64_t const wordoffset = offset>>6;
			uint64_t const bitoffset = offset&0x3fu;
			
			__sync_fetch_and_and(A+wordoffset,maskone[bitoffset]);
			__sync_fetch_and_or (A+wordoffset,insone[(bitoffset<<1)|v]);
		}
		/**
		 * set one bit synchronous
		 * @param A bit vector
		 * @param offset
		 * @param bit
		 **/
		template<typename iterator>
		inline bool setBit8Sync(iterator A, uint64_t const offset)
		{
			static const uint64_t setone[] = { 
				static_cast<uint64_t>(1ull<<63), 
				static_cast<uint64_t>(1ull<<62), 
				static_cast<uint64_t>(1ull<<61), 
				static_cast<uint64_t>(1ull<<60), 
				static_cast<uint64_t>(1ull<<59), 
				static_cast<uint64_t>(1ull<<58), 
				static_cast<uint64_t>(1ull<<57), 
				static_cast<uint64_t>(1ull<<56),
				static_cast<uint64_t>(1ull<<55), 
				static_cast<uint64_t>(1ull<<54), 
				static_cast<uint64_t>(1ull<<53), 
				static_cast<uint64_t>(1ull<<52), 
				static_cast<uint64_t>(1ull<<51), 
				static_cast<uint64_t>(1ull<<50), 
				static_cast<uint64_t>(1ull<<49), 
				static_cast<uint64_t>(1ull<<48),
				static_cast<uint64_t>(1ull<<47), 
				static_cast<uint64_t>(1ull<<46), 
				static_cast<uint64_t>(1ull<<45), 
				static_cast<uint64_t>(1ull<<44), 
				static_cast<uint64_t>(1ull<<43), 
				static_cast<uint64_t>(1ull<<42), 
				static_cast<uint64_t>(1ull<<41), 
				static_cast<uint64_t>(1ull<<40),
				static_cast<uint64_t>(1ull<<39), 
				static_cast<uint64_t>(1ull<<38), 
				static_cast<uint64_t>(1ull<<37), 
				static_cast<uint64_t>(1ull<<36), 
				static_cast<uint64_t>(1ull<<35), 
				static_cast<uint64_t>(1ull<<34), 
				static_cast<uint64_t>(1ull<<33), 
				static_cast<uint64_t>(1ull<<32),
				static_cast<uint64_t>(1ull<<31), 
				static_cast<uint64_t>(1ull<<30), 
				static_cast<uint64_t>(1ull<<29), 
				static_cast<uint64_t>(1ull<<28), 
				static_cast<uint64_t>(1ull<<27), 
				static_cast<uint64_t>(1ull<<26), 
				static_cast<uint64_t>(1ull<<25), 
				static_cast<uint64_t>(1ull<<24),
				static_cast<uint64_t>(1ull<<23), 
				static_cast<uint64_t>(1ull<<22), 
				static_cast<uint64_t>(1ull<<21), 
				static_cast<uint64_t>(1ull<<20), 
				static_cast<uint64_t>(1ull<<19), 
				static_cast<uint64_t>(1ull<<18), 
				static_cast<uint64_t>(1ull<<17), 
				static_cast<uint64_t>(1ull<<16),
				static_cast<uint64_t>(1ull<<15), 
				static_cast<uint64_t>(1ull<<14), 
				static_cast<uint64_t>(1ull<<13), 
				static_cast<uint64_t>(1ull<<12), 
				static_cast<uint64_t>(1ull<<11), 
				static_cast<uint64_t>(1ull<<10), 
				static_cast<uint64_t>(1ull<<9), 
				static_cast<uint64_t>(1ull<<8),
				static_cast<uint64_t>(1ull<<7), 
				static_cast<uint64_t>(1ull<<6), 
				static_cast<uint64_t>(1ull<<5), 
				static_cast<uint64_t>(1ull<<4), 
				static_cast<uint64_t>(1ull<<3), 
				static_cast<uint64_t>(1ull<<2), 
				static_cast<uint64_t>(1ull<<1), 
				static_cast<uint64_t>(1ull<<0) 
			};
			uint64_t const wordoffset = offset>>6;
			uint64_t const bitoffset = offset&0x3fu;
			return __sync_fetch_and_or (A+wordoffset,setone[bitoffset]) & setone[bitoffset];
		}
		/**
		 * erase one bit synchronous
		 * @param A bit vector
		 * @param offset
		 * @param bit
		 **/
		template<typename iterator>
		inline bool eraseBit8Sync(iterator A, uint64_t const offset)
		{
			static const uint64_t setone[] = { 
				static_cast<uint64_t>(1ull<<63), 
				static_cast<uint64_t>(1ull<<62), 
				static_cast<uint64_t>(1ull<<61), 
				static_cast<uint64_t>(1ull<<60), 
				static_cast<uint64_t>(1ull<<59), 
				static_cast<uint64_t>(1ull<<58), 
				static_cast<uint64_t>(1ull<<57), 
				static_cast<uint64_t>(1ull<<56),
				static_cast<uint64_t>(1ull<<55), 
				static_cast<uint64_t>(1ull<<54), 
				static_cast<uint64_t>(1ull<<53), 
				static_cast<uint64_t>(1ull<<52), 
				static_cast<uint64_t>(1ull<<51), 
				static_cast<uint64_t>(1ull<<50), 
				static_cast<uint64_t>(1ull<<49), 
				static_cast<uint64_t>(1ull<<48),
				static_cast<uint64_t>(1ull<<47), 
				static_cast<uint64_t>(1ull<<46), 
				static_cast<uint64_t>(1ull<<45), 
				static_cast<uint64_t>(1ull<<44), 
				static_cast<uint64_t>(1ull<<43), 
				static_cast<uint64_t>(1ull<<42), 
				static_cast<uint64_t>(1ull<<41), 
				static_cast<uint64_t>(1ull<<40),
				static_cast<uint64_t>(1ull<<39), 
				static_cast<uint64_t>(1ull<<38), 
				static_cast<uint64_t>(1ull<<37), 
				static_cast<uint64_t>(1ull<<36), 
				static_cast<uint64_t>(1ull<<35), 
				static_cast<uint64_t>(1ull<<34), 
				static_cast<uint64_t>(1ull<<33), 
				static_cast<uint64_t>(1ull<<32),
				static_cast<uint64_t>(1ull<<31), 
				static_cast<uint64_t>(1ull<<30), 
				static_cast<uint64_t>(1ull<<29), 
				static_cast<uint64_t>(1ull<<28), 
				static_cast<uint64_t>(1ull<<27), 
				static_cast<uint64_t>(1ull<<26), 
				static_cast<uint64_t>(1ull<<25), 
				static_cast<uint64_t>(1ull<<24),
				static_cast<uint64_t>(1ull<<23), 
				static_cast<uint64_t>(1ull<<22), 
				static_cast<uint64_t>(1ull<<21), 
				static_cast<uint64_t>(1ull<<20), 
				static_cast<uint64_t>(1ull<<19), 
				static_cast<uint64_t>(1ull<<18), 
				static_cast<uint64_t>(1ull<<17), 
				static_cast<uint64_t>(1ull<<16),
				static_cast<uint64_t>(1ull<<15), 
				static_cast<uint64_t>(1ull<<14), 
				static_cast<uint64_t>(1ull<<13), 
				static_cast<uint64_t>(1ull<<12), 
				static_cast<uint64_t>(1ull<<11), 
				static_cast<uint64_t>(1ull<<10), 
				static_cast<uint64_t>(1ull<<9), 
				static_cast<uint64_t>(1ull<<8),
				static_cast<uint64_t>(1ull<<7), 
				static_cast<uint64_t>(1ull<<6), 
				static_cast<uint64_t>(1ull<<5), 
				static_cast<uint64_t>(1ull<<4), 
				static_cast<uint64_t>(1ull<<3), 
				static_cast<uint64_t>(1ull<<2), 
				static_cast<uint64_t>(1ull<<1), 
				static_cast<uint64_t>(1ull<<0) 
			};
			static const uint64_t maskone[] = { 
				static_cast<uint64_t>(~(1ull<<63)), 
				static_cast<uint64_t>(~(1ull<<62)), 
				static_cast<uint64_t>(~(1ull<<61)), 
				static_cast<uint64_t>(~(1ull<<60)), 
				static_cast<uint64_t>(~(1ull<<59)), 
				static_cast<uint64_t>(~(1ull<<58)), 
				static_cast<uint64_t>(~(1ull<<57)), 
				static_cast<uint64_t>(~(1ull<<56)),
				static_cast<uint64_t>(~(1ull<<55)), 
				static_cast<uint64_t>(~(1ull<<54)), 
				static_cast<uint64_t>(~(1ull<<53)), 
				static_cast<uint64_t>(~(1ull<<52)), 
				static_cast<uint64_t>(~(1ull<<51)), 
				static_cast<uint64_t>(~(1ull<<50)), 
				static_cast<uint64_t>(~(1ull<<49)), 
				static_cast<uint64_t>(~(1ull<<48)),
				static_cast<uint64_t>(~(1ull<<47)), 
				static_cast<uint64_t>(~(1ull<<46)), 
				static_cast<uint64_t>(~(1ull<<45)), 
				static_cast<uint64_t>(~(1ull<<44)), 
				static_cast<uint64_t>(~(1ull<<43)), 
				static_cast<uint64_t>(~(1ull<<42)), 
				static_cast<uint64_t>(~(1ull<<41)), 
				static_cast<uint64_t>(~(1ull<<40)),
				static_cast<uint64_t>(~(1ull<<39)), 
				static_cast<uint64_t>(~(1ull<<38)), 
				static_cast<uint64_t>(~(1ull<<37)), 
				static_cast<uint64_t>(~(1ull<<36)), 
				static_cast<uint64_t>(~(1ull<<35)), 
				static_cast<uint64_t>(~(1ull<<34)), 
				static_cast<uint64_t>(~(1ull<<33)), 
				static_cast<uint64_t>(~(1ull<<32)),
				static_cast<uint64_t>(~(1ull<<31)), 
				static_cast<uint64_t>(~(1ull<<30)), 
				static_cast<uint64_t>(~(1ull<<29)), 
				static_cast<uint64_t>(~(1ull<<28)), 
				static_cast<uint64_t>(~(1ull<<27)), 
				static_cast<uint64_t>(~(1ull<<26)), 
				static_cast<uint64_t>(~(1ull<<25)), 
				static_cast<uint64_t>(~(1ull<<24)),
				static_cast<uint64_t>(~(1ull<<23)), 
				static_cast<uint64_t>(~(1ull<<22)), 
				static_cast<uint64_t>(~(1ull<<21)), 
				static_cast<uint64_t>(~(1ull<<20)), 
				static_cast<uint64_t>(~(1ull<<19)), 
				static_cast<uint64_t>(~(1ull<<18)), 
				static_cast<uint64_t>(~(1ull<<17)), 
				static_cast<uint64_t>(~(1ull<<16)),
				static_cast<uint64_t>(~(1ull<<15)), 
				static_cast<uint64_t>(~(1ull<<14)), 
				static_cast<uint64_t>(~(1ull<<13)), 
				static_cast<uint64_t>(~(1ull<<12)), 
				static_cast<uint64_t>(~(1ull<<11)), 
				static_cast<uint64_t>(~(1ull<<10)), 
				static_cast<uint64_t>(~(1ull<<9)), 
				static_cast<uint64_t>(~(1ull<<8)),
				static_cast<uint64_t>(~(1ull<<7)), 
				static_cast<uint64_t>(~(1ull<<6)), 
				static_cast<uint64_t>(~(1ull<<5)), 
				static_cast<uint64_t>(~(1ull<<4)), 
				static_cast<uint64_t>(~(1ull<<3)), 
				static_cast<uint64_t>(~(1ull<<2)), 
				static_cast<uint64_t>(~(1ull<<1)), 
				static_cast<uint64_t>(~(1ull<<0)) 
			};
			uint64_t const wordoffset = offset>>6;
			uint64_t const bitoffset = offset&0x3Fu;
			return __sync_fetch_and_and (A+wordoffset,maskone[bitoffset]) & setone[bitoffset];
		}
		
		#endif
		/**
		 * put one bit 
		 * @param A bit vector
		 * @param offset
		 * @param bit
		 **/
		template<typename iterator>
		inline void putBit(iterator A, uint64_t const offset, uint32_t const bit)
		{
			typedef typename std::iterator_traits<iterator>::value_type value_type;
			
			switch( sizeof(value_type) )
			{
				case 8:          return putBit8(A,offset,bit);
				case 4:          return putBit4(A,offset,bit);
				case 2:          return putBit2(A,offset,bit);
				case 1: default: return putBit1(A,offset,bit);
			}
		}
		#if defined(LIBMAUS2_HAVE_SYNC_OPS)
		/**
		 * put one bit 
		 * @param A bit vector
		 * @param offset
		 * @param bit
		 **/
		template<typename iterator>
		inline void putBitSync(iterator A, uint64_t const offset, uint32_t const bit)
		{
			typedef typename std::iterator_traits<iterator>::value_type value_type;
			
			switch( sizeof(value_type) )
			{
				case 8:          putBit8Sync(A,offset,bit); break;
				case 4:          putBit4Sync(A,offset,bit); break;
				case 2:          putBit2Sync(A,offset,bit); break;
				case 1: default: putBit1Sync(A,offset,bit); break;
			}
		}		
		/**
		 * set one bit 
		 * @param A bit vector
		 * @param offset
		 * @param bit
		 **/
		template<typename iterator>
		inline bool setBitSync(iterator A, uint64_t const offset)
		{
			typedef typename std::iterator_traits<iterator>::value_type value_type;
			
			switch( sizeof(value_type) )
			{
				case 8:          return setBit8Sync(A,offset);
				case 4:          return setBit4Sync(A,offset);
				case 2:          return setBit2Sync(A,offset);
				case 1: default: return setBit1Sync(A,offset);
			}
		}		
		/**
		 * erase one bit 
		 * @param A bit vector
		 * @param offset
		 * @param bit
		 **/
		template<typename iterator>
		inline bool eraseBitSync(iterator A, uint64_t const offset)
		{
			typedef typename std::iterator_traits<iterator>::value_type value_type;
			
			switch( sizeof(value_type) )
			{
				case 8:          return eraseBit8Sync(A,offset);
				case 4:          return eraseBit4Sync(A,offset);
				case 2:          return eraseBit2Sync(A,offset);
				case 1: default: return eraseBit1Sync(A,offset);
			}
		}		
		#endif
	}
}
#endif
