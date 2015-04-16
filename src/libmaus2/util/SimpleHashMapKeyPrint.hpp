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
#if ! defined(LIBMAUS_UTIL_SIMPLEHASHMAPKEYPRINT_HPP)
#define LIBMAUS_UTIL_SIMPLEHASHMAPKEYPRINT_HPP

#include <iostream>
#include <libmaus/types/types.hpp>
#include <libmaus/uint/uint.hpp>
#include <vector>

namespace libmaus
{
	namespace util
	{
		template<typename N>
		struct SimpleHashMapKeyPrint
		{
			static std::ostream & printKey(std::ostream & out, N const & v)
			{
				return (out << v);
			}
		};
		
		template<unsigned int k>
		struct SimpleHashMapKeyPrint< libmaus::uint::UInt<k> >
		{
			static std::ostream & printKey(std::ostream & out, libmaus::uint::UInt<k> const & U)
			{
				return (out << U);
			}
		};
		
		#if defined(LIBMAUS_HAVE_UNSIGNED_INT128)
		template<>
		struct SimpleHashMapKeyPrint<libmaus::uint128_t>
		{
			static std::ostream & printKey(std::ostream & out, libmaus::uint128_t const & rv)
			{
				libmaus::uint128_t v = rv;
				if ( ! v )
					return out << '0';
				else
				{
					// uint128_t number can be no more than ceil(log(2^128-1)/log(10)) = 39 decimal digits long
					char C[39];
					int numdig = 0;

					while ( v )
					{
						C[numdig++] = v % 10;
						v /= 10;
					}
					while ( --numdig >= 0 )
						out.put(C[numdig]);
					
					return out;
				}
			}		
		};
		#endif
	}
}
#endif
