/*
    libmaus2
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
#if ! defined(LIBMAUS_SUFFIXSORT_GAPARRAYBYTEOVERFLOWKEYACCESSOR_HPP)
#define LIBMAUS_SUFFIXSORT_GAPARRAYBYTEOVERFLOWKEYACCESSOR_HPP

#include <istream>
#include <libmaus2/types/types.hpp>

namespace libmaus2
{
	namespace suffixsort
	{
		struct GapArrayByteOverflowKeyAccessor
		{
			std::istream & in;
			
			GapArrayByteOverflowKeyAccessor(std::istream & rin)
			: in(rin)
			{
			}
			
			uint64_t operator[](uint64_t const i) const
			{
				return get(i);
			}
			
			uint64_t get(uint64_t const i) const
			{
				in.clear();
				in.seekg(i * 2 * sizeof(uint64_t));
				
				uint64_t p[1];
				
				in.read(reinterpret_cast<char *>(&p[0]),1*sizeof(uint64_t));
				
				return p[0];		
			}
		};
	}
}
#endif
