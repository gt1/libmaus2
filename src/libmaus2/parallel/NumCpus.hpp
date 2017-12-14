/*
    libmaus2
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS2_PARALLEL_NUMCPUS)
#define LIBMAUS2_PARALLEL_NUMCPUS

#include <libmaus2/LibMausConfig.hpp>
#include <libmaus2/types/types.hpp>
#include <cassert>
#include <string>
#include <cctype>
#include <fstream>

#if defined(_OPENMP)
#include <omp.h>
#endif

#if defined(__APPLE__)
#include <sys/sysctl.h>
#endif

#if defined(__linux__)
#include <unistd.h>
#if defined(LIBMAUS2_HAVE_LINUX_SYSCTL_H)
#include <linux/sysctl.h>
#else
#include <fstream>
#endif
#endif

namespace libmaus2
{
	namespace parallel
	{
		struct NumCpus
		{
			static uint64_t getNumLogicalProcessors()
			{
				#if defined(_OPENMP)
				return omp_get_max_threads();
				#elif defined(__linux__)
					#if defined(LIBMAUS2_HAVE_LINUX_SYSCTL_H)
					return sysconf(_SC_NPROCESSORS_ONLN);
					#else
					::std::ifstream istr("/proc/cpuinfo");
					if ( ! istr.is_open() )
						return 1;

					std::string line;
					uint64_t num = 0;
					while ( istr )
					{
						std::getline(istr,line);
						if ( line.find(':') != std::string::npos )
						{
							line = line.substr(0,line.find(':'));

							uint64_t i = 0;
							while ( i < line.size() && !isspace(line[i]) )
								++i;

							line = line.substr(0,i);

							if ( line == "processor" )
								num += 1;
						}
					}

					if ( num )
						return num;
					else
						return 1;
					#endif
				#elif defined(__APPLE__)
				uint32_t numlog = 0;
				size_t numloglen = sizeof(numlog);
				int const sysctlretname = sysctlbyname("hw.logicalcpu_max", &numlog, &numloglen, 0, 0);
				assert ( ! sysctlretname );
				return numlog;
				#else
				return 1;
				#endif
			}
		};
	}
}
#endif
