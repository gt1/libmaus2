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
#if ! defined(LIBMAUS2_UTIL_NUMBERTOSTRING_HPP)
#define LIBMAUS2_UTIL_NUMBERTOSTRING_HPP

#include <libmaus2/types/types.hpp>
#include <sstream>
#include <vector>
#include <algorithm>
#include <ostream>

namespace libmaus2
{
	namespace util
	{
		template<typename _data_type>
		struct NumberToString
		{
			static std::string toString(_data_type const v)
			{
				std::ostringstream ostr;
				ostr << v;
				return ostr.str();
			}
		};

		#if defined(LIBMAUS2_HAVE_UNSIGNED_INT128)
		template<>
		struct NumberToString<libmaus2::uint128_t>
		{
			static std::string toString(libmaus2::uint128_t const v)
			{
				libmaus2::uint128_t w = v;

				if ( ! w )
				{
					return std::string("0");
				}
				else
				{
					libmaus2::uint128_t const ten = 10u;
					std::vector<char> V;
					while ( w )
					{
						V.push_back('0'+static_cast<uint64_t>(w % ten));
						w /= ten;
					}
					std::reverse(V.begin(),V.end());
					return std::string(V.begin(),V.end());
				}
			}
		};
		#endif
	}
}
#endif
