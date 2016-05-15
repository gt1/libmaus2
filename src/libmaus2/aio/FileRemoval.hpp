/*
    libmaus2
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
#if ! defined(LIBMAUS2_AIO_FILEREMOVAL_HPP)
#define LIBMAUS2_AIO_FILEREMOVAL_HPP

#include <libmaus2/aio/MemoryFileContainer.hpp>
#include <cstdio>

namespace libmaus2
{
	namespace aio
	{
		struct FileRemoval
		{
			static bool hasPrefix(std::string const & name, std::string const & prefix);
			static void removeFileNoUnregister(std::string const & name);
			static void removeFile(std::string const & name);
		};
	}
}
#endif
