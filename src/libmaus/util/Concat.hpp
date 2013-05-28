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

#if ! defined(CONCAT_HPP)
#define CONCAT_HPP

#include <libmaus/util/GetFileSize.hpp>

namespace libmaus
{
	namespace util
	{
		struct Concat
		{
			static void concat(std::istream & in, std::ostream & out);
			static void concat(std::string const & filename, std::ostream & out);
			static void concat(std::vector < std::string > const & files, std::ostream & out, bool const rem = true);
			static void concatParallel(
				std::vector < std::string > const & files, 
				std::string const & outputfilename, 
				bool const rem
			);
			static void concatParallel(
				std::vector < std::vector < std::string > > const & files, 
				std::string const & outputfilename, 
				bool const rem
			);
			static void concat(std::vector < std::string > const & files, std::string const & outputfile, bool const rem = true);
			static void concat(
				std::vector < std::vector < std::string > > const & files, 
				std::string const & outputfilename, 
				bool const rem
			);
		};
	}
}
#endif
