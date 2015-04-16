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
#if ! defined(LIBMAUS2_NETWORK_CURLINIT_HPP)
#define LIBMAUS2_NETWORK_CURLINIT_HPP

#include <libmaus2/LibMausConfig.hpp>
#include <libmaus2/parallel/PosixSpinLock.hpp>

#if defined(LIBMAUS2_HAVE_LIBCURL)
#include <curl/curl.h>
#endif

namespace libmaus2
{
	namespace network
	{
		struct CurlInit
		{
			static libmaus2::parallel::PosixSpinLock lock;
			static uint64_t initcomplete;
			
			CurlInit()
			{
				std::ostringstream errstr;
				if ( ! init(errstr) )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << errstr.str() << std::endl;
					lme.finish();
					throw lme;
				}
			}
			
			~CurlInit()
			{
				shutdown();
			}
			
			static bool init(std::ostream & errstr)
			{
				#if defined(LIBMAUS2_HAVE_LIBCURL)
				libmaus2::parallel::ScopePosixSpinLock llock(lock);
				if ( ! initcomplete )
				{
					CURLcode const initfailed = curl_global_init(CURL_GLOBAL_ALL);
					
					if ( ! initfailed )
					{			
						initcomplete += 1;
						return true;
					}
					else
					{
						errstr << "curl_global_init failed: " << curl_easy_strerror(initfailed) << std::endl;
						return false;
					}
				}
				else
				{
					return true;
				}
				#else
				errstr << "curl_global_init failed: curl support is missing" << std::endl;
				return false;
				#endif
			}
			
			static void shutdown()
			{
				#if defined(LIBMAUS2_HAVE_LIBCURL)
				libmaus2::parallel::ScopePosixSpinLock llock(lock);
				if ( initcomplete && (! --initcomplete) )
					curl_global_cleanup();
				#endif
			}
		};
	}
}
#endif
