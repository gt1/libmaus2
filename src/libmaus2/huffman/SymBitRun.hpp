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
#if ! defined(LIBMAUS2_HUFFMAN_SYMBITRUN_HPP)
#define LIBMAUS2_HUFFMAN_SYMBITRUN_HPP

#include <libmaus2/huffman/SymBit.hpp>

namespace libmaus2
{
	namespace huffman
	{
		struct SymBitRun : public SymBit
		{
			uint64_t rlen;

			SymBitRun(
				int64_t const rsym = 0,
				bool const rsbit = false,
				uint64_t const rrlen = 0
			) : SymBit(rsym,rsbit), rlen(rrlen)
			{

			}

			SymBitRun & operator=(SymBit const & SC)
			{
				static_cast<SymBit &>(*this) = SC;
				return *this;
			}

			bool operator==(SymBitRun const & SC) const
			{
				return static_cast<SymBit const &>(*this) == SC && rlen == SC.rlen;
			}

			bool operator==(SymBit const & SC) const
			{
				return static_cast<SymBit const &>(*this) == SC;
			}
		};
	}
}
#endif
