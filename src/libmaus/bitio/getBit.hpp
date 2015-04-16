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


#if ! defined(GETBIT_HPP)
#define GETBIT_HPP

#include <iterator>
#include <libmaus/types/types.hpp>

namespace libmaus
{
	namespace bitio
	{
		/**
		 * read one bit from a bit vector
		 * @param A bit vector
		 * @param offset
		 * @return bit
		 **/
		template<typename iterator>
		inline bool getBit1(iterator A, uint64_t const offset)
		{
			static const uint8_t maskone[] = { (1u<<7), (1u<<6), (1u<<5), (1u<<4), (1u<<3), (1u<<2), (1u<<1), (1u<<0) };
			return A[(offset >> 3)] & maskone[offset & 0x7u];
		}
		/**
		 * read one bit from a bit vector
		 * @param A bit vector
		 * @param offset
		 * @return bit
		 **/
		template<typename iterator>
		inline bool getBit2(iterator A, uint64_t const offset)
		{
			static const 
				uint16_t maskone[] = { 
					(1u<<15), (1u<<14), (1u<<13), (1u<<12), (1u<<11), (1u<<10), (1u<<9), (1u<<8),
					(1u<<7), (1u<<6), (1u<<5), (1u<<4), (1u<<3), (1u<<2), (1u<<1), (1u<<0) 
				};

			// std::cerr << "getBit2::Offset: " << (offset >> 4) << " num " << A[offset>>4] << std::endl;
			
			return A[(offset >> 4)] & maskone[offset & 0xfu];
		}
		/**
		 * read one bit from a bit vector
		 * @param A bit vector
		 * @param offset
		 * @return bit
		 **/
		template<typename iterator>
		inline bool getBit4(iterator A, uint64_t const offset)
		{
			static const 
				uint32_t maskone[] = { 
					(1u<<31), (1u<<30), (1u<<29), (1u<<28), (1u<<27), (1u<<26), (1u<<25), (1u<<24),
					(1u<<23), (1u<<22), (1u<<21), (1u<<20), (1u<<19), (1u<<18), (1u<<17), (1u<<16),
					(1u<<15), (1u<<14), (1u<<13), (1u<<12), (1u<<11), (1u<<10), (1u<<9), (1u<<8),
					(1u<<7), (1u<<6), (1u<<5), (1u<<4), (1u<<3), (1u<<2), (1u<<1), (1u<<0) 
				};
			return A[(offset >> 5)] & maskone[offset & 0x1fu];
		}
		/**
		 * read one bit from a bit vector
		 * @param A bit vector
		 * @param offset
		 * @return bit
		 **/
		template<typename iterator>
		inline bool getBit8(iterator A, uint64_t const offset)
		{
			static const 
				uint64_t maskone[] = { 
					(1ULL<<63), (1ULL<<62), (1ULL<<61), (1ULL<<60), (1ULL<<59), (1ULL<<58), (1ULL<<57), (1ULL<<56),
					(1ULL<<55), (1ULL<<54), (1ULL<<53), (1ULL<<52), (1ULL<<51), (1ULL<<50), (1ULL<<49), (1ULL<<48),
					(1ULL<<47), (1ULL<<46), (1ULL<<45), (1ULL<<44), (1ULL<<43), (1ULL<<42), (1ULL<<41), (1ULL<<40),
					(1ULL<<39), (1ULL<<38), (1ULL<<37), (1ULL<<36), (1ULL<<35), (1ULL<<34), (1ULL<<33), (1ULL<<32),
					(1ULL<<31), (1ULL<<30), (1ULL<<29), (1ULL<<28), (1ULL<<27), (1ULL<<26), (1ULL<<25), (1ULL<<24),
					(1ULL<<23), (1ULL<<22), (1ULL<<21), (1ULL<<20), (1ULL<<19), (1ULL<<18), (1ULL<<17), (1ULL<<16),
					(1ULL<<15), (1ULL<<14), (1ULL<<13), (1ULL<<12), (1ULL<<11), (1ULL<<10), (1ULL<<9), (1ULL<<8),
					(1ULL<<7), (1ULL<<6), (1ULL<<5), (1ULL<<4), (1ULL<<3), (1ULL<<2), (1ULL<<1), (1ULL<<0) 
				};
			return A[(offset >> 6)] & maskone[offset & 0x3Fu];
		}
		/**
		 * read one bit from a bit vector
		 * @param A bit vector
		 * @param offset
		 * @return bit
		 **/
		template<typename iterator>
		inline bool getBit(iterator A, uint64_t const offset)
		{
			typedef typename std::iterator_traits<iterator>::value_type value_type;
			
			switch( sizeof(value_type) )
			{
				case 8:          return getBit8(A,offset);
				case 4:          return getBit4(A,offset);
				case 2:          return getBit2(A,offset);
				case 1: default: return getBit1(A,offset);
			}
		}
	}
}
#endif
