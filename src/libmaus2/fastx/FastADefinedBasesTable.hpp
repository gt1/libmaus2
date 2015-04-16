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
#if ! defined(LIBMAUS2_FASTX_FASTADEFINEDBASESTABLE_HPP)
#define LIBMAUS2_FASTX_FASTADEFINEDBASESTABLE_HPP

#include <libmaus2/autoarray/AutoArray.hpp>
#include <algorithm>

namespace libmaus2
{
	namespace fastx
	{
		struct FastADefinedBasesTable
		{
			::libmaus2::autoarray::AutoArray<bool> table;
			
			FastADefinedBasesTable()
			: table(256,false)
			{
				std::fill(table.begin(),table.end(),false);
				table['A'] = table['T'] = table['U'] = table['G'] = table['C'] = table['K'] =
				table['M'] = table['R'] = table['Y'] = table['S'] = table['W'] = table['B'] =
				table['V'] = table['H'] = table['D'] = true;
			}
			
			bool operator[](uint64_t const i) const
			{
				return table[i];
			}
		};
	}
}
#endif
