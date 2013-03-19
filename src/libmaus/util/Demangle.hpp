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

#if ! defined(DEMANGLE_HPP)
#define DEMANGLE_HPP

#include <string>

#if defined(__GNUC__)
#include <cxxabi.h>
#include <cstring>
#endif

namespace libmaus
{
	namespace util
	{
		struct Demangle
		{
			static std::string demangleName(std::string const name)
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
			template<typename eclass>
			static std::string demangle()
			{
				return demangleName(typeid(eclass).name());
			}
			template<typename eclass>
			static std::string demangle(eclass const &)
			{
				return demangleName(typeid(eclass).name());
			}
		};
	}
}
#endif
