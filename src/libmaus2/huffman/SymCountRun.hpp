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
#if ! defined(LIBMAUS2_HUFFMAN_SYMCOUNTRUN_HPP)
#define LIBMAUS2_HUFFMAN_SYMCOUNTRUN_HPP

#include <libmaus2/huffman/SymCount.hpp>

namespace libmaus2
{
	namespace huffman
	{
		struct SymCountRun : public SymCount
		{
			uint64_t rlen;

			SymCountRun(
				int64_t const rsym = 0,
				uint64_t const rcnt = 0,
				bool const rsbit = false,
				uint64_t const rrlen = 0
			) : SymCount(rsym,rcnt,rsbit), rlen(rrlen)
			{

			}

			SymCountRun & operator=(SymCount const & SC)
			{
				static_cast<SymCount &>(*this) = SC;
				return *this;
			}

			bool operator==(SymCountRun const & SC) const
			{
				return static_cast<SymCount const &>(*this) == SC && rlen == SC.rlen;
			}

			bool operator==(SymCount const & SC) const
			{
				return static_cast<SymCount const &>(*this) == SC;
			}
		};
	}
}
#endif
