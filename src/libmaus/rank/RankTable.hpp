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

#if ! defined(RANKTABLE_HPP)
#define RANKTABLE_HPP

#include <libmaus/types/types.hpp>
#include <libmaus/util/unique_ptr.hpp>

namespace libmaus
{
	namespace rank
	{
		/**
		 * Lookup table for population count in 16 bit numbers. The table uses 2^19 bytes.
		 **/
		struct RankTable
		{
			typedef RankTable this_type;
			typedef ::libmaus::util::unique_ptr < this_type >::type unique_ptr_type;
			
			private:
			uint8_t const * const table;
			static uint8_t * generateTable();
			
			public:
			/**
			 * compute population count of the i+1 most significant bits in m
			 * @param m
			 * @param i
			 * @return population count
			 **/
			unsigned int operator()(uint16_t const m, unsigned int const i) const
			{
				if ( m == 0xFFFFu ) return i+1;
				uint8_t const b = table[ (m << 3) + (i >> 1) ];		
				return (i&1)?(b >> 4):(b&0xF);
			}
		
			/**
			 * constructor
			 **/
			RankTable();
			/**
			 * destructor
			 **/
			~RankTable();
		};
		/**
		 * Simple lookup table for population count in 16 bit numbers. The table uses 2^16 bytes.
		 **/
		struct SimpleRankTable
		{
			typedef SimpleRankTable this_type;
			typedef ::libmaus::util::unique_ptr < this_type >::type unique_ptr_type;
			
			private:
			uint8_t const * const table;
			static uint8_t * generateTable();
			
			public:
			/**
			 * compute population count of the i+1 most significant bits in m
			 * @param m
			 * @param i
			 * @return population count
			 **/
			unsigned int operator()(uint16_t const m) const
			{
				return table[m];
			}
		
			/**
			 * constructor
			 **/
			SimpleRankTable();
			/**
			 * destructor
			 **/
			~SimpleRankTable();
		};
	}
}
#endif
