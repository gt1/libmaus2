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


#if ! defined(READELIAS_HPP)
#define READELIAS_HPP

#include <libmaus/bitio/getBit.hpp>

namespace libmaus
{
	namespace bitio
	{
		template<typename reader_type>
		inline uint64_t readUnary(reader_type & reader)
		{
			uint64_t i = 0;
			
			while ( ! reader.readBit() )
				i++;
			
			return i;
		}
		template<typename iterator>
		inline uint64_t readUnary(iterator A, uint64_t & offset)
		{
			uint64_t i = 0;
			
			while ( ! getBit(A,offset++) )
				i++;
			
			return i;
		}
		template<typename reader_type>
		inline uint64_t readBinary(reader_type & reader, uint64_t const l)
		{
			uint64_t m = 0;
			
			for ( uint64_t i = 0; i < l; ++i )
			{
				m <<= 1;
				
				if ( reader.readBit() )
					m |= 1;
			}
				
			return m;
		}
		template<typename iterator>
		inline uint64_t readBinary(iterator A, uint64_t & offset, uint64_t const l)
		{
			uint64_t m = 0;
			
			for ( uint64_t i = 0; i < l; ++i )
			{
				m <<= 1;
				
				if ( getBit(A,offset++) )
					m |= 1;
			}
				
			return m;
		}
		template<typename iterator>
		inline uint64_t readElias2(iterator A, uint64_t & offset)
		{
			uint64_t const log_2 = readUnary(A,offset);
			uint64_t const log_1 = readBinary(A,offset,log_2);
			uint64_t const n = readBinary(A,offset,log_1);
			return n;
		}
		template<typename reader_type>
		inline uint64_t readElias2(reader_type & reader)
		{
			uint64_t const log_2 = readUnary(reader);
			uint64_t const log_1 = reader.read(log_2);
			uint64_t const n = reader.read(log_1);
			return n;
		}
	}
}
#endif
