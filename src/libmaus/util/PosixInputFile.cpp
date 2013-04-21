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


#include <libmaus/util/PosixInputFile.hpp>
#include <libmaus/exception/LibMausException.hpp>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
                     
libmaus::util::PosixInputFile::PosixInputFile(std::string const & filename)
{
	PosixFileDescriptor::fd = open(filename.c_str(),O_RDONLY);
	
	if ( PosixFileDescriptor::fd < 0 )
	{
		::libmaus::exception::LibMausException se;
		se.getStream() << "PosixFileDescriptor: failed to open file " << filename << ": " << strerror(errno) << std::endl;
		se.finish();
		throw se;
	}
}
