/*
    libmaus
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
#if ! defined(LIBMAUS_UTIL_UNITNUM_HPP)
#define LIBMAUS_UTIL_UNITNUM_HPP

#include <sstream>
#include <libmaus/types/types.hpp>

namespace libmaus
{
	namespace util
	{
		struct UnitNum
		{
			static std::string unitNum(uint64_t n)
			{
				std::ostringstream ostr;
				if ( n < 1024 )
				{
					ostr << n;
				}
				else if ( n < 1024*1024 )
				{
					ostr << (n / 1024.0) << "k";
				}
				else if ( n < 1024ull*1024ull*1024ull )
				{
					ostr << (n / (1024.0*1024.0)) << "m";
				}
				else
				{
					ostr << (n / (1024.0*1024.0*1024.0)) << "g";	
				}
				
				return ostr.str();
			}
		};
	}
}
#endif
