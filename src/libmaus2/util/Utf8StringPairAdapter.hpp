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
#if ! defined(LIBMAUS2_UTIL_UTF8STRINGPAIRADAPTER_HPP)
#define LIBMAUS2_UTIL_UTF8STRINGPAIRADAPTER_HPP

#include <libmaus2/util/Utf8String.hpp>

namespace libmaus2
{
	namespace util
	{
		struct Utf8StringPairAdapter
		{
			Utf8String::shared_ptr_type U;

			Utf8StringPairAdapter(Utf8String::shared_ptr_type rU) : U(rU) {}

			uint64_t size() const
			{
				return 2*U->size();
			}

			wchar_t operator[](uint64_t const i) const
			{
				static unsigned int const shift = 12;
				static wchar_t const mask = (1u << shift)-1;

				wchar_t const full = U->get(i>>1);

				if ( i & 1 )
					return full & mask;
				else
					return full >> shift;
			}
			wchar_t get(uint64_t const i) const
			{
				return (*this)[i];
			}
		};
	}
}
#endif
