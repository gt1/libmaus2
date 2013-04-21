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

#if ! defined(SATURATINGCOUNTER_HPP)
#define SATURATINGCOUNTER_HPP

#include <libmaus/util/unique_ptr.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/rank/ERank222B.hpp>
#include <libmaus/bitio/putBit.hpp>
#include <libmaus/bitio/getBit.hpp>

namespace libmaus
{
	namespace util
	{
		struct SaturatingCounter
		{
			typedef SaturatingCounter this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
		
			uint64_t const n;
			::libmaus::autoarray::AutoArray<uint64_t,::libmaus::autoarray::alloc_type_c> B;
			uint8_t * const A;
			
			typedef ::libmaus::rank::ERank222B rank_type;
			typedef rank_type::unique_ptr_type rank_ptr_type;
			
			rank_ptr_type rank;
			
			SaturatingCounter(uint64_t const rn);

			static unsigned int const shift[4];
			static uint8_t const mask[4];
						
			uint8_t get(uint64_t const i) const
			{
				return (A [ i >> 2 ] >> shift[ i & 3 ]) & 0x3;
			}			
			
			void set(uint64_t const i, uint8_t const v)
			{
				A [ i >> 2 ] &= mask[ i & 3 ];
				A [ i >> 2 ] |= v << shift [ i & 3 ]; 
			}
			
			void inc(uint64_t const i)
			{
				uint8_t const v = get(i);
				uint8_t const w = v + 1;
				uint8_t const c = (w & 0xFC);
				uint8_t const nv = (w | (c>>1) | (c>>2)) & 0x3;
				set ( i, nv );
			}
			
			uint64_t size() const;
			void shrink();
			
			uint64_t rank1(uint64_t const i) const
			{
				return rank->rank1(i);
			}
			uint64_t rank0(uint64_t const i) const
			{
				return rank->rank0(i);
			}
			bool getBit(uint64_t const i)
			{
				return ::libmaus::bitio::getBit(B.get(),i);
			}
		};	
		
		std::ostream & operator<<(std::ostream & out, SaturatingCounter const & S);
	}
}
#endif
