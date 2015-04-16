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

#if ! defined(ESELECTBASE_HPP)
#define ESELECTBASE_HPP

#include <libmaus2/rank/ERankBase.hpp>

namespace libmaus2
{
	namespace select
	{
		template<bool _sym>
		struct ESelectBase : public ::libmaus2::rank::ERankBase
		{
			static bool const sym = _sym;
			typedef ESelectBase<_sym> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
		
			static inline uint64_t process(uint64_t v)
			{
				if ( _sym )
					return v;
				else
					return ~v;
			}

			/**
			 * return offset of i+1th 1 bit in v
			 **/
			static uint64_t select1Slow(uint16_t v, unsigned int i);
			/**
			 * compute four russians table
			 **/
			static ::libmaus2::autoarray::AutoArray<uint8_t> computeRussians();

			// russians table
			::libmaus2::autoarray::AutoArray<uint8_t> const R;

			/**
			 * return offset of i+1th 1 bit in v
			 **/
			uint64_t select1(uint64_t v, uint64_t i) const
			{
				uint64_t j = 0;
				unsigned int p;
				
				/**
				 * push decision to lower 32 bits
				 **/
				if ( (p=popcount4(v >> 32)) <= i )
					j += 32, i -= p;
				else
					v >>= 32;

				/**
				 * push decision to lower 16 bits
				 **/
				if ( (p=popcount2(v >> 16)) <= i )
					j += 16, i -= p;
				else
					v >>= 16;
				
				return j + ESelectBase<_sym>::R [ ((v&0xFFFFull) << 4) | i ];
			}

			ESelectBase() : ERankBase(), R(computeRussians()) {}
		};

		extern template struct ESelectBase<false>;
		extern template struct ESelectBase<true>;
	}
}
#endif
