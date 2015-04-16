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

#if ! defined(CONSOLECOLOR_HPP)
#define CONSOLECOLOR_HPP

#include <cstdio>
#include <string>
#include <libmaus2/LibMausConfig.hpp>

#if defined(HAVE_ISATTY)
#include <unistd.h>
#endif

namespace libmaus2
{
	namespace util
	{
		/**
		 * class for setting the text colour in an output terminal;
		 * if stderr is not a terminal, then the codes delivered are
		 * HTML tags
		 *
		 * the methods need to be used in the following order (example):
		 *
		 * ConsoleColor::start();
		 * ConsoleColor::green();
		 * std::cerr << "Hello world." << std::endl;
		 * ... // more text
		 * ConsoleColor::stop();
		 **/
		struct ConsoleColor {
			//! available colours
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
				
			/**
			 * @return true if stderr is a terminal
			 **/
			static bool istty();
			/**
			 * return escape code for turning the text color to c 
			 *
			 * @param c requested color
			 * @return escape code for turning text to color c
			 **/
			static std::string color(Color c);
			/**
			 * @return escape code for turning text colour to black
			 **/
			static std::string black();
			/**
			 * @return escape code for turning text colour to red
			 **/
			static std::string red();
			/**
			 * @return escape code for turning text colour to green
			 **/
			static std::string green();
			/**
			 * @return escape code for turning text colour to yellow
			 **/
			static std::string yellow();
			/**
			 * @return escape code for turning text colour to blue
			 **/
			static std::string blue();
			/**
			 * @return escape code for turning text colour to magenta
			 **/
			static std::string magenta();
			/**
			 * @return escape code for turning text colour to cyan
			 **/
			static std::string cyan();
			/**
			 * @return escape code for turning text colour to light gray
			 **/
			static std::string lightgray();
			/**
			 * @return escape code for turning text colour to default colour
			 **/
			static std::string defaultcolor();
			
			/**
			 * @return code for starting coloured printing
			 **/
			static std::string start();
			/**
			 * @return code for ending coloured printing
			 **/
			static std::string stop();
		};
	}
}
#endif
