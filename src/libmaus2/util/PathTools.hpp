/*
    libmaus2
    Copyright (C) 2018 German Tischler-Höhle

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
#if ! defined(LIBMAUS2_UTIL_PATHTOOLS_HPP)
#define LIBMAUS2_UTIL_PATHTOOLS_HPP

#include <libmaus2/util/ArgInfo.hpp>

namespace libmaus2
{
	namespace util
	{
		struct PathTools
		{
			static void lchdir(std::string const & s);
			static std::string sbasename(std::string const & s);
			static std::string sdirname(std::string const & s);
			static std::string getAbsPath(std::string const fn);
			static std::string getCurDir();
		};
	}
}
#endif
