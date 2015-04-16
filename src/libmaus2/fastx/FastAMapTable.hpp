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
#if ! defined(LIBMAUS2_FASTX_FASTAMAPTABLE_HPP)
#define LIBMAUS2_FASTX_FASTAMAPTABLE_HPP

#include <libmaus2/fastx/FastADefinedBasesTable.hpp>
#include <cctype>

namespace libmaus2
{
	namespace fastx
	{
		struct FastAMapTable
		{
			::libmaus2::autoarray::AutoArray<uint8_t> table;

			FastAMapTable()
			: table(256,false)
			{
				::libmaus2::fastx::FastADefinedBasesTable FDBT;
			
				for ( uint64_t i = 0; i < 256; ++i )
				{
					uint8_t const up = isalpha(i) ? toupper(i) : i;
					
					if ( FDBT[up] )
						table[i] = up;
					else
						table[i] = 'N';
				}
			}
			
			uint8_t operator[](uint8_t const i) const
			{
				return table[i];
			}
		};
	}
}
#endif
