/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#if ! defined(LIBMAUS2_UTIL_MEMORYSTATISTICS_HPP)
#define LIBMAUS2_UTIL_MEMORYSTATISTICS_HPP

#include <unistd.h>
#include <libmaus2/exception/LibMausException.hpp>

namespace libmaus2
{
	namespace util
	{
		struct MemoryStatistics
		{
			static int64_t getPageSize()
			{
				#if defined(_SC_PAGESIZE)
				long const v = sysconf(_SC_PAGESIZE);

				if ( v < 0 )
				{
					int const error = errno;
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "libmaus2::util::getPageSize() failed: " << strerror(error) << std::endl;
					lme.finish();
					throw lme;
				}

				return static_cast<int64_t>(v);
				#elif defined(LIBMAUS2_HAVE_GETPAGESIZE)
				return getpagesize();
				#else
				libmaus2::exception::LibMausException lme;
				lme.getStream() << "libmaus2::util::MemoryStatistics::getPageSize(): not supported." << std::endl;
				lme.finish();
				throw lme;
				#endif
			}

			static int64_t getNumPhysPages()
			{
				#if defined(_SC_PHYS_PAGES)
				long const v = sysconf(_SC_PHYS_PAGES);

				if ( v < 0 )
				{
					int const error = errno;
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "libmaus2::util::getNumPhysPages() failed: " << strerror(error) << std::endl;
					lme.finish();
					throw lme;
				}

				return static_cast<int64_t>(v);
				#else
				libmaus2::exception::LibMausException lme;
				lme.getStream() << "libmaus2::util::MemoryStatistics::getNumPhysPages(): not supported." << std::endl;
				lme.finish();
				throw lme;
				#endif
			}

			static int64_t getNumAvPhysPages()
			{
				#if defined(_SC_AVPHYS_PAGES)
				long const v = sysconf(_SC_AVPHYS_PAGES);

				if ( v < 0 )
				{
					int const error = errno;
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "libmaus2::util::getNumAvPhysPages() failed: " << strerror(error) << std::endl;
					lme.finish();
					throw lme;
				}

				return static_cast<int64_t>(v);
				#else
				libmaus2::exception::LibMausException lme;
				lme.getStream() << "libmaus2::util::MemoryStatistics::getNumAvPhysPages(): not supported." << std::endl;
				lme.finish();
				throw lme;
				#endif
			}

			static int64_t getPhysicalMemory()
			{
				#if defined(__APPLE__)
				int mib[2] = { CTL_HW, HW_MEMSIZE };
				int64_t physmem;
				size_t vlen = sizeof(int64_t);
				sysctl(&mib[0], 2, &physmem, &vlen, NULL, 0);
				return physmem;
				#else
				return getNumPhysPages() * getPageSize();
				#endif
			}

			static int64_t getAvailablePhysicalMemory()
			{
				return getNumAvPhysPages() * getPageSize();
			}
		};
	}
}
#endif
