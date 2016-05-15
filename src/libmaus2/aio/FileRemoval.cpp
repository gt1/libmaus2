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
#include <libmaus2/util/TempFileRemovalContainer.hpp>
#include <libmaus2/aio/FileRemoval.hpp>

bool libmaus2::aio::FileRemoval::hasPrefix(std::string const & name, std::string const & prefix)
{
	return name.size() >= prefix.size() && name.substr(0,prefix.size()) == prefix;
}

void libmaus2::aio::FileRemoval::removeFileNoUnregister(std::string const & name)
{
	if ( hasPrefix(name,"file://") )
	{
		std::string const fn = name.substr(strlen("file://"));
		libmaus2::aio::FileRemoval::removeFile(fn);
	}
	else if ( hasPrefix(name,"mem:") )
	{
		std::string const fn = name.substr(strlen("mem:"));
		MemoryFileContainer::eraseEntry(fn);
	}
	else
	{
		::remove(name.c_str());
	}
}

void libmaus2::aio::FileRemoval::removeFile(std::string const & name)
{
	removeFileNoUnregister(name);
	libmaus2::util::TempFileRemovalContainer::removeTempFile(name);
}
