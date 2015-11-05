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
			}

			static int64_t getNumPhysPages()
			{
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
			}

			static int64_t getNumAvPhysPages()
			{
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
			}

			static int64_t getPhysicalMemory()
			{
				return getNumPhysPages() * getPageSize();
			}

			static int64_t getAvailablePhysicalMemory()
			{
				return getNumAvPhysPages() * getPageSize();
			}
		};
	}
}
#endif
