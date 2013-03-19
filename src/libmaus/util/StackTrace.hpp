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
// #include <libmaus/util/PosixExecute.hpp>

namespace libmaus
{
	namespace util
	{
		struct StackTrace
		{
			std::vector < std::string > trace;   

			static void simpleStackTrace(std::ostream & ostr = std::cerr)
			{
				#if defined(LIBMAUS_HAVE_BACKTRACE)
				unsigned int const depth = 20;
				void *array[depth];
				size_t const size = backtrace(array,depth);
				char ** strings = backtrace_symbols(array,size);

				for ( size_t i = 0; i < size; ++i )
					ostr << "[" << i << "]" << strings[i] << std::endl;

				free(strings);
				#endif
			}

			StackTrace()
			{
				#if defined(LIBMAUS_HAVE_BACKTRACE)
				unsigned int const depth = 20;
				void *array[depth];
				size_t const size = backtrace(array,depth);
				char ** strings = backtrace_symbols(array,size);

				for ( size_t i = 0; i < size; ++i )
					trace.push_back(strings[i]);

				free(strings);
				#endif
			}
			
			static std::string getExecPath()
			{
				#if defined(_linux__)
				char buf[PATH_MAX+1];
				std::fill(
					&buf[0],&buf[sizeof(buf)/sizeof(buf[0])],0);
				if ( readlink("/proc/self/exe", &buf[0], PATH_MAX) < 0 )
					throw std::runtime_error("readlink(/proc/self/exe) failed.");
				return std::string(&buf[0]);
				#else
				return "program";
				#endif
			}
			
			static std::pair < std::string, std::string > components(std::string line, char const start, char const end)
			{
				uint64_t i = line.size();
				
				while ( i != 0 )
					if ( line[--i] == end )
						break;
				
				if ( line[i] != end || i == 0 )
					return std::pair<std::string,std::string>(std::string(),std::string());
				
				uint64_t const addrend = i;

				while ( i != 0 )
					if ( line[--i] == start )
						break;
					
				return std::pair<std::string,std::string>(
					line.substr(0,i),
					line.substr(i+1, addrend-(i+1) ));
			}
			
			std::string toString(bool const translate = true) const;
			
			static std::string getStackTrace()
			{
				StackTrace st;
				return st.toString();
			}
		};
	}
}
#endif
