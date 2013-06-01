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

#if ! defined(I386CACHELINESIZE_HPP)
#define I386CACHELINESIZE_HPP

#include <libmaus/LibMausConfig.hpp>
#include <iostream>
#include <numeric>

namespace libmaus
{
	namespace util
	{
		#if defined(LIBMAUS_USE_ASSEMBLY) && defined(LIBMAUS_HAVE_i386)
		/**
		 * class for i386 cache line size detection
		 **/
		struct I386CacheLineSize
		{
			private:
			/**
			 * call cpuid function
			 *
			 * @param eax register eax
			 * @param ebx register ebx
			 * @param ecx register ecx
			 * @param edx register edx
			 **/
			static void cpuid(
				uint32_t & eax,
				uint32_t & ebx,
				uint32_t & ecx,
				uint32_t & edx
			);

			static unsigned int getCacheLineSizeSingle(unsigned int const val);
			static unsigned int getCacheLineSize(unsigned int const reg);
			
			public:
			/**
			 * @return cache line size (0 if it cannot be determined)
			 **/
			static unsigned int getCacheLineSize();
		};
		#endif
	}
}
#endif
