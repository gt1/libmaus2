/**
    libmaus2
    Copyright (C) 2007-2012 Simon Gog  All Right Reserved.
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
#if ! defined(NEARESTNEIGHBOURDICTIONARY_HPP)
#define NEARESTNEIGHBOURDICTIONARY_HPP

#include <libmaus2/bitio/CompactArray.hpp>
#include <libmaus2/bitio/BitVector.hpp>

namespace libmaus2
{
	namespace util
	{
		/**
		 * nearest neighbour dictionary imported from SDSL
		 **/
		struct NearestNeighbourDictionary
		{
			typedef NearestNeighbourDictionary this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			static uint64_t const nndblocksize = 32;
			
			::libmaus2::bitio::CompactArray::unique_ptr_type m_abs_samples;
			::libmaus2::bitio::CompactArray::unique_ptr_type m_differences;
			::libmaus2::bitio::IndexedBitVector::unique_ptr_type m_contains_abs_sample;
			uint64_t m_ones;
			uint64_t m_size;
			
			uint64_t byteSize() const;
			uint64_t size() const;

			/*
			 * constructor. the code in of the constructor is imported from Simon Gog's SDSL library
			 */
			NearestNeighbourDictionary(::libmaus2::bitio::BitVector const & v);

			bool operator[](uint64_t const idx) const
			{
				uint64_t const r = rankm1(idx+1);
				return r && (selectp(r) == idx);
			}
			
			uint64_t rank1(uint64_t const idx) const
			{
				return rankm1(idx+1);
			}

			/**
			 * rank excluding index idx
			 **/	
			uint64_t rankm1(uint64_t idx) const;
			
			uint64_t select1(uint64_t i) const
			{
				return selectp(i+1);
			}
			
			/* ranks start at 1 */
			uint64_t selectp(uint64_t i) const;
			
			uint64_t prev1(uint64_t i) const 
			{
				uint64_t r = rankm1(i+1);
				assert(r>0);
				return selectp(r);
			}

			uint64_t next1(uint64_t i) const 
			{
				uint64_t r = rankm1(i);
				assert(r < m_ones);
				return selectp(r+1);
			}                                    
		};
	}
}
#endif

