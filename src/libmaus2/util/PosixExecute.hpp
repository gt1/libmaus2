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

#if ! defined(POSIXEXECUTE_HPP)
#define POSIXEXECUTE_HPP

#include <string>
#include <libmaus2/util/ArgInfo.hpp>
                                          
namespace libmaus2
{
	namespace util
	{
		struct PosixExecute
		{
			static int getTempFile(std::string const stemplate, std::string & filename);
			static std::string getBaseName(std::string const & name);
			static std::string getProgBaseName(::libmaus2::util::ArgInfo const & arginfo);
			static int getTempFile(::libmaus2::util::ArgInfo const & arginfo, std::string & filename);
			static std::string loadFile(std::string const filename);
			static void executeOld(::libmaus2::util::ArgInfo const & arginfo, std::string const & command, std::string & out, std::string & err);
			static int setNonBlockFlag (int desc, bool on);
			static int execute(std::string const & command, std::string & out, std::string & err, bool const donotthrow = false);
		};
	}
}
#endif
