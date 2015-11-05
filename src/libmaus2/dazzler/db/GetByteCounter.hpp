/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#if ! defined(LIBMAUS2_DAZZLER_DB_GETBYTECOUNTER_HPP)
#define LIBMAUS2_DAZZLER_DB_GETBYTECOUNTER_HPP

#include <libmaus2/types/types.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace db
		{
			struct GetByteCounter
			{
				uint64_t c;

				GetByteCounter()
				: c(0)
				{

				}

				int get()
				{
					++c;
					return 0;
				}

				bool operator()() const
				{
					return true;
				}
			};
		}
	}
}
#endif
