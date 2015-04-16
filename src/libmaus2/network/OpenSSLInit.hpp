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
#if ! defined(LIBMAUS_NETWORK_OPENSSLINIT_HPP)
#define LIBMAUS_NETWORK_OPENSSLINIT_HPP

#include <libmaus2/LibMausConfig.hpp>
#include <libmaus2/parallel/PosixSpinLock.hpp>

#if defined(LIBMAUS_HAVE_OPENSSL)
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

namespace libmaus2
{
	namespace network
	{
		struct OpenSSLInit
		{
			static libmaus2::parallel::PosixSpinLock lock;
			static uint64_t initcomplete;
			
			OpenSSLInit()
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
			
			~OpenSSLInit()
			{
				shutdown();
			}
			
			static bool init(std::ostream & 
				#if !defined(LIBMAUS_HAVE_OPENSSL)
				errstr
				#endif
			)
			{
				#if defined(LIBMAUS_HAVE_OPENSSL)
				libmaus2::parallel::ScopePosixSpinLock llock(lock);
				if ( ! initcomplete )
				{
					SSL_load_error_strings();   
					SSL_library_init();
					ERR_load_BIO_strings();
					OpenSSL_add_all_algorithms();   
					return true;                                
				}
				else
				{
					return true;
				}
				#else
				errstr << "OpenSSL init failed: OpenSSL support is missing" << std::endl;
				return false;
				#endif
			}
			
			static void shutdown()
			{
				#if defined(LIBMAUS_HAVE_OPENSSL)
				libmaus2::parallel::ScopePosixSpinLock llock(lock);
				if ( initcomplete && (! --initcomplete) )
				{
					ERR_free_strings();
					EVP_cleanup();
				}
				#endif
			}
		};
	}
}
#endif
