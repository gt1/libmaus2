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

#if ! defined(STACKTRACE_HPP)
#define STACKTRACE_HPP

#include <libmaus/LibMausConfig.hpp>

#if defined(LIBMAUS_HAVE_EXECINFO_H)
#include <execinfo.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>
#include <libmaus/types/types.hpp>
#include <limits.h>
#include <iostream>

namespace libmaus
{
	namespace util
	{
		struct StackTrace
		{
			std::vector < std::string > trace;   

			static void simpleStackTrace(std::ostream & ostr = std::cerr);

			StackTrace();
			
			static std::string getExecPath();
			static std::pair < std::string, std::string > components(std::string line, char const start, char const end);
			std::string toString(bool const translate = true) const;
			static std::string getStackTrace();
		};
	}
}
#endif
