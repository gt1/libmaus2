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

#if ! defined(NUMBITS_HPP)
#define NUMBITS_HPP

#include <libmaus/types/types.hpp>
#include <libmaus/autoarray/AutoArray.hpp>

namespace libmaus
{
	namespace math
	{
		template<typename N>
		unsigned int numbits(N n)
		{
			unsigned int c = 0;
			while ( n )
			{
				c++;
				n >>= 1;
			}
			return c;
		}
		
		inline unsigned int numbits32(uint32_t n)
		{
			unsigned int c = 0;
			if ( n & 0xFFFF0000ul )
				n >>= 16, c += 16;
			if ( n & 0xFF00ul )
				n >>= 8, c+=8;
			if ( n & 0xF0ul )
				n >>= 4, c+= 4;
			if ( n & 0xCul )
				n >>= 2, c+= 2;
			if ( n & 0x2ul )
				n >>= 1, c+= 1;
			if ( n )
				n >>= 1, c+= 1;
				
			return c;
		}
		
		struct NumBits8
		{
			::libmaus::autoarray::AutoArray<unsigned int> T;
			
			NumBits8() : T(256)
			{
				for ( unsigned int i = 0; i < T.size(); ++i )
					T[i] = ::libmaus::math::numbits(i);
			}
			
			unsigned int operator()(unsigned int const i) const
			{
				return T[i];
			}
		};
		
		struct NumBits32 : public NumBits8
		{
			typedef NumBits8 base_type;
		
			NumBits32() : NumBits8() {}
			
			unsigned int operator()(uint32_t n) const
			{
				unsigned int c = 0;
				uint32_t t;
				if ( (t=(n >> 16)) )
					n=t, c += 16;
				if ( (t=(n >> 8)) )
					n >>= 8, c += 8;
				return c + base_type::T[n];
			}
		};
		struct NumBits64 : public NumBits8
		{
			typedef NumBits8 base_type;
		
			NumBits64() : NumBits8() {}
			
			unsigned int operator()(uint64_t n) const
			{
				unsigned int c = 0;
				uint64_t t;
				if ( (t=(n >> 32)) )
					n=t, c += 32;
				if ( (t=(n >> 16)) )
					n=t, c += 16;
				if ( (t=(n >> 8)) )
					n >>= 8, c += 8;
				return c + base_type::T[n];
			}
		};

		inline uint64_t nextTwoPow(uint64_t const n)
		{
			uint64_t const numbits = ::libmaus::math::numbits(n);
			
			if ( n == (static_cast<uint64_t>(1ull) << (numbits-1)) )
				return n;
			else
				return (static_cast<uint64_t>(1ull) << (numbits));
		}
	}
}
#endif
