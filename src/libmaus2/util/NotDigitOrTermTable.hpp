/*
    libmaus2
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#if ! defined(LIBMAUS2_UTIL_NOTDIGITORTERMTABLE_HPP)
#define LIBMAUS2_UTIL_NOTDIGITORTERMTABLE_HPP

#include <ostream>

namespace libmaus2
{
	namespace util
	{
		struct NotDigitOrTermTable
		{
			static char const table[256];
			
			bool operator[](int i) const
			{
				return table[i];
			}
			
			std::ostream & print(std::ostream & out) const
			{
				out << "NotDigitOrTermTable::table[256] = {\n";
				int i = 0;
				while ( i < 256 )
				{
					if ( table[i] )
						out.put('1');
					else
						out.put('0');
					if ( i + 1 < 256 )
						out.put(',');
					if ( ++i % 16 == 0 )
						out.put('\n');
				}
				out << "};\n";
				return out;
			}
		};	
	}
}
#endif
