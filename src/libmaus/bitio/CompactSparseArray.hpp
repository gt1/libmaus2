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

#if ! defined(COMPACTSPARSEARRAY_HPP)
#define COMPACTSPARSEARRAY_HPP

#include <libmaus/bitio/CompactArrayBase.hpp>
#include <libmaus/util/iterator.hpp>
#include <libmaus/bitio/getBits.hpp>
#include <libmaus/bitio/putBits.hpp>

namespace libmaus
{
	namespace bitio
	{
		struct CompactSparseArray
		{
			typedef CompactSparseArray this_type;
			
			typedef uint64_t value_type;

			typedef ::libmaus::util::AssignmentProxy<this_type,value_type> proxy_type;
			typedef ::libmaus::util::AssignmentProxyIterator<this_type,value_type> iterator;
			typedef ::libmaus::util::ConstIterator<this_type,value_type> const_iterator;
			
			const_iterator begin() const
			{
				return const_iterator(this,0);
			}
			const_iterator end() const
			{
				return const_iterator(this,size());
			}
			iterator begin()
			{
				return iterator(this,0);
			}
			iterator end()
			{
				return iterator(this,size());
			}
			
			uint64_t size() const
			{
				return n;
			}

			uint64_t * D;
			
			uint64_t const n; // length of sequence
			uint64_t const b; // bits per stored number
			
			uint64_t const o; // additive offset
			uint64_t const k; // multiplicative offset

			CompactSparseArray(
				uint64_t * rD,
				uint64_t const rn, 
				uint64_t const rb,
				uint64_t const ro,
				uint64_t const rk
			) : D(rD), n(rn), b(rb), o(ro), k(rk)
			{}

			uint64_t get(uint64_t i) const { return ::libmaus::bitio::getBits(D, i*k + o, b); }
			void set(uint64_t i, uint64_t v) { putBits(D, i*k+o, b, v); /* assert ( get(i) == v ); */ }	
		};
	}
}
#endif
