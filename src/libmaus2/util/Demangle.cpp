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

#include <libmaus2/util/Demangle.hpp>

#if defined(__GNUC__)
#include <cxxabi.h>
#include <cstring>
#endif

std::string libmaus2::util::Demangle::demangleName(std::string const name)
{
	#if defined(__GNUC__)
	int status = 0;
	size_t length = 0;

	char * demangled = __cxxabiv1::__cxa_demangle(name.c_str(),NULL,&length,&status);

	// if demangling is successful
	if ( status == 0 )
	{
		try
		{
			// copy string
			std::string const s(demangled);
			// free block allocated by __cxa_demangle
			::free(demangled);
			return s;
		}
		catch(...)
		{
			// failed to construct string, do not leak memory
			::free(demangled);
			return name;
		}
	}
	else
	{
		// free block
		::free(demangled);
		return name;
	}
	#else
	return name;
	#endif
}
