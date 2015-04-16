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

#if ! defined(EDGEBUFFERLOCALLOCKED_HPP)
#define EDGEBUFFERLOCALLOCKED_HPP

#include <libmaus2/graph/EdgeBuffer.hpp>
#include <libmaus2/graph/EdgeListLocalLockedFlush.hpp>

namespace libmaus2
{
	namespace graph
	{
		struct EdgeBufferLocalLocked
		{
			typedef EdgeBufferLocalLocked this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			EdgeListLocalLockedFlush & EBLF;
			EdgeBuffer < EdgeListLocalLockedFlush > EB;
			
			EdgeBufferLocalLocked(
				EdgeListLocalLockedFlush & rEBLF, 
				uint64_t const bufsize
			)
			: EBLF(rEBLF), EB(bufsize,EBLF)
			{
			
			}
			
			void put(::libmaus2::graph::TripleEdge const & T)
			{
				EB.put(T);
			}

			void flush()
			{
				EB.flush();
			}
		};
	}
}
#endif
