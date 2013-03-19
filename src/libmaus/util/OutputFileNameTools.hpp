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

#if ! defined(OUTPUTFILENAMETOOLS)
#define OUTPUTFILENAMETOOLS

#include <string>
#include <vector>

namespace libmaus
{
	namespace util
	{
		struct OutputFileNameTools
		{
			static std::string lcp(std::string const & a, std::string const & b);
			static std::string lcp(std::vector<std::string> const & V);
			static bool endsOn(std::string const & a, std::string const & b);
			static std::string clipOff(std::string const & a, std::string const & b);
			static std::string endClip(std::string a, char const * const * V);
			static std::string endClipLcp(std::vector<std::string> const & V, char const * const * const E);
		};
	}
}
#endif
