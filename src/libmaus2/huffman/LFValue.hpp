/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#if ! defined(LIBMAUS2_HUFFMAN_LFVALUE_HPP)
#define LIBMAUS2_HUFFMAN_LFVALUE_HPP

#include <libmaus2/types/types.hpp>

namespace libmaus2
{
	namespace huffman
	{
		struct LFValue
		{
			int64_t sym;
			uint64_t value;
			bool sbit;

			LFValue(int64_t const rsym = 0, uint64_t const rvalue = 0, bool const rsbit = false)
			: sym(rsym), value(rvalue), sbit(rsbit) {}

			bool operator==(LFValue const & O) const
			{
				return sym == O.sym && value == O.value && sbit == O.sbit;
			}
		};
	}
}
#endif
