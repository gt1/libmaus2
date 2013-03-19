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

#if ! defined(OFFSETELEMENT_HPP)
#define OFFSETELEMENT_HPP

#include <ostream>
#include <libmaus/types/types.hpp>

namespace libmaus
{
	namespace lcs
	{
		struct OffsetElement
		{
			uint32_t querypos;
			int32_t offset;
			
			OffsetElement() : querypos(0), offset(0) {}
			OffsetElement(uint16_t const rquerypos, int16_t const roffset) : querypos(rquerypos), offset(roffset) {}
			
			bool operator<(OffsetElement const & o) const
			{
				if ( offset != o.offset )
					return offset < o.offset;
				else
					return querypos < o.querypos;
			}
		};

		struct OffsetElementQueryposComparator
		{
			bool operator()(OffsetElement const & a, OffsetElement const & b) const
			{
				return a.querypos < b.querypos;
			}
		};

		struct OffsetElementOffsetComparator
		{
			bool operator()(OffsetElement const & a, OffsetElement const & b) const
			{
				return a.offset < b.offset;
			}
		};

		inline std::ostream & operator<<(std::ostream & out, OffsetElement const & O)
		{
			out << "(" << O.querypos << "," << O.offset << ")";
			return out;
		}
	}
}
#endif
