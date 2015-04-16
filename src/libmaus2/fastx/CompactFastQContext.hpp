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
#if ! defined(LIBMAUS2_FASTX_COMPACTFASTQCONTEXT_HPP)
#define LIBMAUS2_FASTX_COMPACTFASTQCONTEXT_HPP

#include <libmaus2/parallel/SynchronousCounter.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>

namespace libmaus2
{
	namespace fastx
	{
		struct CompactFastQContext
		{
			::libmaus2::parallel::SynchronousCounter<uint64_t> nextid;
			::libmaus2::autoarray::AutoArray<uint8_t> qbuf;

			CompactFastQContext(uint64_t const rnextid = 0) : nextid(rnextid), qbuf() {}
			
			uint64_t byteSize() const
			{
				return sizeof(CompactFastQContext) + qbuf.byteSize();
			}
		};
	}
}
#endif
