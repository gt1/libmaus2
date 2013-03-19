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
				
			static bool istty()
			{
				bool tty = false;

		#if defined(HAVE_ISATTY)
				tty = isatty(STDERR_FILENO);
		#endif

				return tty;	
			}
				
			static std::string color(Color c)
			{
				bool const tty = istty();
			
				std::string s;

				if ( tty )
				{
					switch (c)
					{
						case c_black:
							s = "\e[0;30m";
							break;
						case c_red:
							s = "\e[0;31m";
							break;
						case c_green:
							s = "\e[0;32m";
							break;
						case c_yellow:
							s = "\e[0;33m";
							break;
						case c_blue:
							s = "\e[0;34m";
							break;
						case c_magenta:
							s = "\e[0;35m";
							break;
						case c_cyan:
							s = "\e[0;36m";
							break;
						case c_lightgray:
							s = "\e[0;37m";
							break;
						case c_defaultcolor:
							s = "\e[0;39m";
							break;
					}
				}
				else
				{
					switch (c)
					{
						case c_black:
							s = "</span><span style=\"color:black\">";
							break;
						case c_red:
							s = "</span><span style=\"color:red\">";
							break;
						case c_green:
							s = "</span><span style=\"color:green\">";
							break;
						case c_yellow:
							s = "</span><span style=\"color:yellow\">";
							break;
						case c_blue:
							s = "</span><span style=\"color:blue\">";
							break;
						case c_magenta:
							s = "</span><span style=\"color:magenta\">";
							break;
						case c_cyan:
							s = "</span><span style=\"color:cyan\">";
							break;
						case c_lightgray:
							s = "</span><span style=\"color:rgb(32,32,32)\">";
							break;
						case c_defaultcolor:
							s = "</span><span style=\"color:black\">";
							break;
					}
				}
				
				return s;
			}

			static std::string black() { return color(c_black); }
			static std::string red() { return color(c_red);  }
			static std::string green() { return color(c_green); }
			static std::string yellow() { return color(c_yellow);  }
			static std::string blue() { return color(c_blue);  }
			static std::string magenta() {  return color(c_magenta); }
			static std::string cyan() { return color(c_cyan); }
			static std::string lightgray() { return color(c_lightgray); }
			static std::string defaultcolor() { return color(c_defaultcolor); }
			
			static std::string start() {
				bool const tty = istty();

				std::string s;
			
				if ( tty )
				{
				
				}
				else
				{
					s = "<pre><span style=\"color:black\">";
				}
				
				return s;
			}
			static std::string stop() {
				bool const tty = istty();

				std::string s;
			
				if ( tty )
				{
				
				}
				else
				{
					s = "</span></pre>";
				}
				
				return s;
			}
		};
	}
}
#endif
