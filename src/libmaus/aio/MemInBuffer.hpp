/*
    libmaus
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


#if ! defined(MEMINBUFFER_HPP)
#define MEMINBUFFER_HPP

#include <libmaus/types/types.hpp>
#include <libmaus/util/unique_ptr.hpp>

namespace libmaus
{
	namespace aio
	{
		struct MemInBuffer
		{
			uint64_t const * in;
			uint64_t const * const ine;
			
			typedef MemInBuffer this_type;
			typedef ::libmaus::util::unique_ptr < this_type >::type unique_ptr_type;
			
			MemInBuffer(uint64_t const * const rin, uint64_t const n)
			: in(rin), ine(in+n)
			{
			
			}
			
			bool getNext(uint64_t & v)
			{
				if ( in == ine )
					return false;
				else
				{
					v = *(in++);
					return true;
				}
			}
			
			void flush()
			{
			
			}
		};
	}
}
#endif
