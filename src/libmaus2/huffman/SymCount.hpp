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
#if !defined(LIBMAUS2_HUFFMAN_SYMCOUNT_HPP)
#define LIBMAUS2_HUFFMAN_SYMCOUNT_HPP

#include <libmaus2/types/types.hpp>

namespace libmaus2
{
	namespace huffman
	{
		struct SymCount
		{
			SymCount(
				int64_t const rsym = 0,
				uint64_t const rcnt = 0,
				bool const rsbit = false
			) : sym(rsym), cnt(rcnt), sbit(rsbit)
			{

			}

			int64_t sym;
			uint64_t cnt;
			bool sbit;

			bool operator==(SymCount const & S) const
			{
				return sym == S.sym && cnt == S.cnt && sbit == S.sbit;
			}
		};
	}
}
#endif
