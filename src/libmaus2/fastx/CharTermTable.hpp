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

#if ! defined(LIBMAUS_FASTX_CHARTERMTABLE_HPP)
#define LIBMAUS_FASTX_CHARTERMTABLE_HPP

#include <libmaus2/autoarray/AutoArray.hpp>

namespace libmaus2
{
	namespace fastx
	{
		struct CharTermTable
		{
			private:
			::libmaus2::autoarray::AutoArray<bool> atable;
			bool * table;
			
			CharTermTable & operator=(CharTermTable const &);
			CharTermTable(CharTermTable const &);
			
			public:
			bool operator[](int i) const
			{
				return table[i];
			}
			
			CharTermTable(uint8_t c)
			: atable(257), table(atable.get()+1)
			{
				for ( unsigned int i = 0; i < 256; ++i )
					table[i] = false;
				table[-1] = true;
				table[c] = true;
			}
		};
	}
}
#endif
