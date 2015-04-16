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
#if ! defined(LIBMAUS_PARALLEL_NUMCPUS)
#define LIBMAUS_PARALLEL_NUMCPUS

#include <libmaus2/LibMausConfig.hpp>
#include <libmaus2/types/types.hpp>
#include <cassert>

#if defined(_OPENMP)
#include <omp.h>
#endif

#if defined(__APPLE__)
#include <sys/sysctl.h>
#endif

#if defined(__linux__)
#include <unistd.h>
#include <linux/sysctl.h>              
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
				return sysconf(_SC_NPROCESSORS_ONLN);
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
