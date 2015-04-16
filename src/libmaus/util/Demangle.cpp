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

#include <libmaus/util/Demangle.hpp>

#if defined(__GNUC__)
#include <cxxabi.h>
#include <cstring>
#endif

std::string libmaus::util::Demangle::demangleName(std::string const name)
{
	#if defined(__GNUC__)
	int status = 0;
	char buf[1024];
	memset(buf,0,sizeof(buf));
	size_t length = sizeof(buf);
	__cxxabiv1::__cxa_demangle(name.c_str(),buf,&length,&status);
	if ( status == 0 )
		return std::string(buf); // ,buf+length);
	else
		return name;
	#else
	return name;
	#endif
}
