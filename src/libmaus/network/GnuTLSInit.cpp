/*
    libmaus
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
#include <libmaus/network/GnuTLSInit.hpp>

#if defined(LIBMAUS_HAVE_GNUTLS)
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#endif

#include <libmaus/exception/LibMausException.hpp>

libmaus::parallel::PosixSpinLock libmaus::network::GnuTLSInit::lock;
uint64_t libmaus::network::GnuTLSInit::initcomplete = 0;

libmaus::network::GnuTLSInit::GnuTLSInit()
{
	libmaus::parallel::ScopePosixSpinLock slock(lock);
	if ( ! initcomplete++ )
	{
		#if defined(LIBMAUS_HAVE_GNUTLS)
		if (gnutls_check_version("2.12.14") == NULL) 
		{
			libmaus::exception::LibMausException lme;
			lme.getStream() << "Required GnuTLS 2.12.14 not available" << "\n";
			lme.finish();
			throw lme;
		}
		
		gnutls_global_init();	
		#endif
	}
}
libmaus::network::GnuTLSInit::~GnuTLSInit()
{
	libmaus::parallel::ScopePosixSpinLock slock(lock);
	if ( ! --initcomplete )
	{
		#if defined(LIBMAUS_HAVE_GNUTLS)
		gnutls_global_deinit();
		#endif
	}
}
