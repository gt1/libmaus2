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


#if ! defined(GETBITS_HPP)
#define GETBITS_HPP

#include <limits>
#include <sstream>
#include <libmaus2/bitio/BitIO.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/math/MetaLog.hpp>

namespace libmaus2
{
	namespace bitio
	{
		template<unsigned int wordsize>
		struct GetBits
		{
			// get bits from an array
			template<typename iterator>
			inline static uint64_t getBits(iterator A, uint64_t offset, unsigned int numbits)
			{
				if ( ! numbits )
					return 0;
				assert ( numbits <= 64 );

				typedef typename std::iterator_traits<iterator>::value_type value_type;
				unsigned int const bcnt = 8 * sizeof(value_type);
				unsigned int const bshf = ::libmaus2::math::MetaLog2<bcnt>::log;
				unsigned int const bmsk = (1ul<<bshf)-1ul;

				uint64_t byteSkip = (offset >> bshf);
				unsigned int const bitSkip = (offset & bmsk);
				unsigned int const restBits = bcnt-bitSkip;
					
				uint64_t b = static_cast<uint64_t>(A[byteSkip]);
				// skip bits by masking them
				b &= std::numeric_limits<value_type>::max() >> (bcnt - restBits);
				
				if ( restBits == numbits )
					return b;
				else if ( numbits < restBits )
					return b >> (restBits - numbits);
					
				numbits -= restBits;
				
				while ( numbits >= bcnt )
				{
					if ( bcnt < 8*sizeof(b) )
						b <<= bcnt;
					b |= static_cast<uint64_t>( A[++byteSkip] );
					numbits -= bcnt;
				}
				
				if ( numbits )
					b = (b<<numbits) | (static_cast<uint64_t>( A[++byteSkip] ) >> (bcnt-numbits));
				
				return b;
			}
		};

		template<>
		struct GetBits<8>
		{
			// get bits from an array
			template<typename iterator>
			inline static uint64_t getBits(iterator A, uint64_t offset, unsigned int numbits)
			{
				if ( ! numbits )
					return 0;
				assert ( numbits <= 64 );

				typedef uint64_t value_type;
				// bits in value type = 64
				unsigned int const bcnt = 8 * sizeof(value_type);
				// log 64 = 6
				unsigned int const bshf = ::libmaus2::math::MetaLog2<bcnt>::log;
				// bitmask 63
				uint64_t const bmsk = (1ull << bshf)-1;

				// number of words to skip
				uint64_t byteSkip = (offset >> bshf);
				// number of bits to skip in words
				unsigned int const bitSkip = (offset & bmsk);
				// number of bits in first word we read
				unsigned int const restBits = bcnt-bitSkip;
					
				uint64_t b = static_cast<uint64_t>(A[byteSkip]);
				// skip bits by masking them
				b &= std::numeric_limits<value_type>::max() >> (bcnt - restBits);
				
				if ( restBits == numbits )
					return b;
				else if ( numbits < restBits )
					return b >> (restBits - numbits);
					
				numbits -= restBits;
					
				if ( numbits )
					b = (b<<numbits) | (static_cast<uint64_t>( A[++byteSkip] ) >> (bcnt-numbits));
				
				return b;
			}
		};


		template<typename iterator>
		inline uint64_t getBits(iterator A, uint64_t offset, unsigned int numbits)
		{
			return GetBits<sizeof(typename std::iterator_traits<iterator>::value_type)>::getBits(A,offset,numbits);
		}
	}
}
#endif
