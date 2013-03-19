/**
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
**/

#if ! defined(INDEXENTRY_HPP)
#define INDEXENTRY_HPP

#include <libmaus/types/types.hpp>

namespace libmaus
{
	namespace huffman
	{
		struct IndexEntry
		{
			uint64_t pos;
			uint64_t kcnt;
			uint64_t vcnt;
			
			IndexEntry() {}
			IndexEntry(uint64_t const rpos, uint64_t const rkcnt, uint64_t const rvcnt)
			: pos(rpos), kcnt(rkcnt), vcnt(rvcnt) {}
		};
		
		struct IndexEntryValueAdd
		{
			uint64_t operator()(uint64_t const & A, IndexEntry const & B) const
			{
				return A + B.vcnt;
			}
		};
		
		struct IndexEntryKeyAdd
		{
			uint64_t operator()(uint64_t const & A, IndexEntry const & B) const
			{
				return A + B.kcnt;
			}
		};
		
		inline std::ostream & operator<<(std::ostream & out, IndexEntry const & IE)
		{
			out << "IndexEntry(" << IE.pos << "," << IE.kcnt << "," << IE.vcnt << ")";
			return out;
		}
	}
}
#endif
