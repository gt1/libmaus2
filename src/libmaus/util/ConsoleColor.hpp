/*
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
*/

#if ! defined(CONSOLECOLOR_HPP)
#define CONSOLECOLOR_HPP

#include <cstdio>
#include <string>
#include <libmaus/LibMausConfig.hpp>

#if defined(HAVE_ISATTY)
#include <unistd.h>
#endif

namespace libmaus
{
	namespace util
	{
		struct ConsoleColor {
			enum Color {
				c_black,
				c_red,
				c_green,
				c_yellow,
				c_blue,
				c_magenta,
				c_cyan,
				c_lightgray,
				c_defaultcolor
				};
				
			static bool istty();
			static std::string color(Color c);
			static std::string black();
			static std::string red();
			static std::string green();
			static std::string yellow();
			static std::string blue();
			static std::string magenta();
			static std::string cyan();
			static std::string lightgray();
			static std::string defaultcolor();
			
			static std::string start();
			static std::string stop();
		};
	}
}
#endif
