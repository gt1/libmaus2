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
#if ! defined(LIBMAUS_FASTX_FASTATWOBITTABLE_HPP)
#define LIBMAUS_FASTX_FASTATWOBITTABLE_HPP

#include <libmaus/autoarray/AutoArray.hpp>
#include <limits>
#include <cassert>

namespace libmaus
{
	namespace fastx
	{
		struct FastATwoBitTable
		{
			typedef FastATwoBitTable this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			libmaus::autoarray::AutoArray<unsigned char> T;
			
			FastATwoBitTable()
			: T(static_cast<size_t>(std::numeric_limits<unsigned char>::max())+1,false)
			{
				assert ( 3 < T.size() );
			
				std::fill(T.begin(),T.end(),0);
				
				T['a'] = T['A'] = 0;
				T['c'] = T['C'] = 1;
				T['g'] = T['G'] = 2;
				T['t'] = T['T'] = 3;
			}
			
			uint8_t operator[](char const a) const
			{
				return T[static_cast<unsigned char>(a)];
			}
			
			uint64_t operator()(char const * s) const
			{
				if ( ! s )
					return 0;

				uint64_t v = 0;
				
				while ( *s )
				{
					v <<= 2;
					v |= (*this)[*(s++)];
				}
				
				return v + 1;
			}
		};
	}
}
#endif
