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

#if ! defined(LIBMAUS_UTIL_SIMPLEHASHMAPCONSTANTS_HPP)
#define LIBMAUS_UTIL_SIMPLEHASHMAPCONSTANTS_HPP

#include <libmaus/types/types.hpp>
#include <limits>
#include <libmaus/uint/uint.hpp>

namespace libmaus
{
	namespace util
	{
		template<typename _key_type>
		struct SimpleHashMapConstants
		{
			typedef _key_type key_type;
			
			static key_type const unused()
			{
				return std::numeric_limits<key_type>::max();
			}

			static key_type const deleted()
			{
				return unused()-1;
			}
			
			// unused or deleted
			static bool isFree(key_type const & v)
			{
				return (v & deleted()) == deleted();
			}
			
			// in use (not unused or deleted)
			static bool isInUse(key_type const & v)
			{
				return !isFree(v);
			}
			
			virtual ~SimpleHashMapConstants() {}
		};

		template<unsigned int k>
		struct SimpleHashMapConstants< libmaus::uint::UInt<k> >
		{
			typedef libmaus::uint::UInt<k> key_type;
			
			key_type const unusedValue;
			key_type const deletedValue;
			
			static key_type computeUnusedValue()
			{
				key_type U;
				key_type Ulow(std::numeric_limits<uint64_t>::max());
				
				for ( unsigned int i = 0; i < k; ++i )
				{
					U <<= 64;
					U |= Ulow;
				}
				
				return U;
			}

			static key_type computeDeletedValue()
			{
				// get full mask
				key_type U = computeUnusedValue();
				// erase top bit
				U.setBit(k*64-1, 0);
				return U;
			}
						
			key_type const & unused() const
			{
				return unusedValue;
			}

			key_type const & deleted() const
			{
				return deletedValue;
			}

			bool isFree(key_type const & v) const
			{
				return (v & deletedValue) == deletedValue;
			}
			
			bool isInUse(key_type const & v) const
			{
				return !isFree(v);
			}
			
			SimpleHashMapConstants() : unusedValue(computeUnusedValue()), deletedValue(computeDeletedValue()) {}
			virtual ~SimpleHashMapConstants() {}
		};
	}
}
#endif
