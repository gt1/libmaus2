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
#if ! defined(LIBMAUS2_PARALLEL_OMPNUMTHREADSSCOPE_HPP)
#define LIBMAUS2_PARALLEL_OMPNUMTHREADSSCOPE_HPP

#if defined(_OPENMP)
#include <omp.h>
#endif

namespace libmaus2
{
	namespace parallel
	{
		struct OMPNumThreadsScope
		{
			static uint64_t getMaxThreads()
			{
				#if defined(_OPENMP)
				return omp_get_max_threads();
				#else
				return 1;
				#endif
			}
			
			uint64_t const prevnumthreads;
		
			OMPNumThreadsScope(uint64_t const 
				#if defined(_OPENMP)
				newnumthreads = getMaxThreads()
				#endif
			)
			: prevnumthreads(getMaxThreads())
			{
				#if defined(_OPENMP)
				omp_set_num_threads(newnumthreads);
				#endif
			}
			~OMPNumThreadsScope()
			{
				#if defined(_OPENMP)
				omp_set_num_threads(prevnumthreads);
				#endif			
			}
		};
	}
}
#endif
