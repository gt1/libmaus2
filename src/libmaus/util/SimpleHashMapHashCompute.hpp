/*
    libmaus
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if !defined(LIBMAUS_UTIL_SIMPLEHASHMAPHASHCOMPUTE_HPP)
#define LIBMAUS_UTIL_SIMPLEHASHMAPHASHCOMPUTE_HPP

#include <libmaus/types/types.hpp>
#include <libmaus/hashing/hash.hpp>

namespace libmaus
{
	namespace util
	{	
		template<typename _key_type>
		struct SimpleHashMapHashCompute
		{
			typedef _key_type key_type;
			
			inline static uint64_t hash(uint64_t const v)
			{
				return libmaus::hashing::EvaHash::hash642(&v,1);
			}
		};

		#if defined(LIBMAUS_HAVE_UNSIGNED_INT128)
		template<>
		struct SimpleHashMapHashCompute<libmaus::uint128_t>
		{
			typedef libmaus::uint128_t key_type;
			
			inline static uint64_t hash(libmaus::uint128_t const v)
			{
				return libmaus::hashing::EvaHash::hash642(reinterpret_cast<uint64_t const *>(&v),2);
			}
		};
		#endif
	}
}
#endif
