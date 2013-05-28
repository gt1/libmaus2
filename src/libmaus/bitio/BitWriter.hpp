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

#if ! defined(BITWRITER_HPP)
#define BITWRITER_HPP

#include <libmaus/math/numbits.hpp>
#include <libmaus/math/lowbits.hpp>
#include <libmaus/util/unique_ptr.hpp>
#include <vector>
#include <iterator>
#include <ostream>

namespace libmaus
{
	namespace bitio
	{
		/**
		 * bit stream writer class
		 **/
		template<typename _data_type, typename _data_iterator, _data_type _basemask>
		struct BitWriterTemplate
		{
			public:
			/**
			 * data type used by this bit writer class
			 **/
			typedef _data_type data_type;
			typedef _data_iterator data_iterator;
			static _data_type const basemask = _basemask;
			
			// ptr
			typedef BitWriterTemplate<data_type,data_iterator,basemask> this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			private:
			data_iterator U;
			data_type mask;
			data_type cur;
			
			public:
			/**
			 * initialize writer with pointer to array
			 **/
			BitWriterTemplate(data_iterator rU) : U(rU), mask(basemask), cur(0) {}
			
			void writeUnary(uint64_t k)
			{
				for ( uint64_t i = 0; i < k; ++i )
					writeBit(0);
				writeBit(1);
			}
			
			/**
			 *
			 **/
			template<typename N>
			void writeElias2(N n)
			{
				// number of bits to store n
				unsigned int log_1 = ::libmaus::math::numbits(n);
				// number of bits to store log_1
				unsigned int log_2 = ::libmaus::math::numbits(log_1);
				
				// write log_2 in unary form
				writeUnary(log_2);
				
				// write log_1 using log_2 bits
				write(log_1,log_2);
				
				// write n using log_1 bits
				write(n,log_1);
			}
			
			/**
			 * write a b bit number n
			 * @param n number to be written
			 * @param b number of bits to write
			 **/
			template<typename N>
			void write(N n, unsigned int b)
			{
				if ( b )
				{
					// N m = ::libmaus::math::lowbits(b-1);
					N m = (1ull << (b-1));
						
					// write number, msb to lsb
					for ( unsigned int i = 0; i < b; ++i, m >>= 1 )
					{
						writeBit( (n & m) != 0 );
						#if defined(RANK_WRITE_DEBUG)
						n &= ~m;
						#endif
					}
					#if defined(RANK_WRITE_DEBUG)
					assert ( n == 0 );
					#endif
				}
			}
			/**
			 * write one bit to stream
			 * @param bit
			 **/
			void writeBit(bool const bit)
			{
				if ( bit ) 
				{
					cur |= mask;
				}
				
				mask >>= 1;
				
				if ( ! mask )
				{
					*(U++) = cur;
					mask = basemask;
					cur = 0;
				}
			}
			/**
			 * flush output (align to byte boundary) by writing zero bits
			 **/
			void flush()
			{
				while ( mask != basemask )
					writeBit(0);
			}
		};

		typedef BitWriterTemplate<uint8_t,   uint8_t *,              0x80    > BitWriter;
		typedef BitWriterTemplate<uint16_t, uint16_t *,            0x8000    > BitWriter2;
		typedef BitWriterTemplate<uint32_t, uint32_t *,        0x80000000    > BitWriter4;
		typedef BitWriterTemplate<uint64_t, uint64_t *, 0x8000000000000000ULL> BitWriter8;
		typedef BitWriterTemplate<uint8_t , std::back_insert_iterator< std::vector<uint8_t> >, 0x80> BitWriterVector8;
		typedef BitWriterTemplate<uint64_t , std::back_insert_iterator< std::vector<uint64_t> >, 0x8000000000000000ULL> BitWriterVector64;
		typedef BitWriterTemplate<uint8_t , std::ostream_iterator < uint8_t >, 0x80 > BitWriterStream8;
	}
}
#endif
