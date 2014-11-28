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

#include <libmaus/network/GnuTLSSocket.hpp>

#if defined(LIBMAUS_HAVE_GNUTLS)
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#endif

#if defined(LIBMAUS_HAVE_GNUTLS)
/* check certificate */
int libmaus::network::GnuTLSSocket::verify_certificate_callback(gnutls_session_t session)
{
	/* read hostname */
	#if 0
	char const * hostname = (char const *)gnutls_session_get_ptr(session);
	std::cerr << "* HOST NAME " << hostname << std::endl;
	#endif
	
	#if 1
	unsigned int status = 0;
	int r = gnutls_certificate_verify_peers2(session, &status);
	#else
	gnutls_typed_vdata_st data[2];
		
	memset(data, 0, sizeof(data));
			
	data[0].type = GNUTLS_DT_DNS_HOSTNAME;
	data[0].data = (void*)hostname;
					
	data[1].type = GNUTLS_DT_KEY_PURPOSE_OID;
	data[1].data = (void*)GNUTLS_KP_TLS_WWW_SERVER;
	unsigned int status = 0;
	int r = gnutls_certificate_verify_peers(session, data, 2,&status);
	#endif
	
	if (r < 0) 
	{
		fprintf(stderr,"Certificate error: %s\n", gnutls_strerror(r));
		return GNUTLS_E_CERTIFICATE_ERROR;
	}
	
	#if 0
	gnutls_certificate_type_t type = gnutls_certificate_type_get(session);
	char const * tmp = gnutls_certificate_type_get_name(type);
	fprintf(stderr,"* CERT NAME %s\n", tmp);
	#endif
	
	return 0;
}
#endif

libmaus::network::GnuTLSSocket::GnuTLSSocket(
	std::string const & 
		#if defined(LIBMAUS_HAVE_GNUTLS)
		rhostname
		#endif
	,
	unsigned int 
		#if defined(LIBMAUS_HAVE_GNUTLS)
		port
		#endif
	,
	char const * 
		#if defined(LIBMAUS_HAVE_GNUTLS)
		certfile
		#endif
	,
	char const * 
		#if defined(LIBMAUS_HAVE_GNUTLS)
		// certdir
		#endif
	,
	bool const 
		#if defined(LIBMAUS_HAVE_GNUTLS)
		checkcertificate
		#endif
) : libmaus::network::GnuTLSInit()
#if defined(LIBMAUS_HAVE_GNUTLS)
, hostname(rhostname)
#endif
{
	#if defined(LIBMAUS_HAVE_GNUTLS)
	libmaus::network::ClientSocket::unique_ptr_type TCS(new libmaus::network::ClientSocket(port,hostname.c_str()));
	PCS = UNIQUE_PTR_MOVE(TCS);

	/* X509 stuff */
	gnutls_certificate_allocate_credentials(&xcred);

	if ( certfile )
		gnutls_certificate_set_x509_trust_file(xcred, certfile, GNUTLS_X509_FMT_PEM);

	if ( checkcertificate )
		gnutls_certificate_set_verify_function(xcred, verify_certificate_callback);

	/* init session */		
	gnutls_init(&session, GNUTLS_CLIENT);

	gnutls_session_set_ptr(session, (void *) hostname.c_str());
	gnutls_server_name_set(session, GNUTLS_NAME_DNS, hostname.c_str(), hostname.size());

	/* use default priorities */
	// gnutls_set_default_priority(session);
	char const * errpos = NULL;
	gnutls_priority_set_direct(session,"NORMAL:-VERS-TLS-ALL:+VERS-TLS1.0:+VERS-SSL3.0:%COMPAT",&errpos);
	
	/* put the x509 credentials to the current session */
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred);
	
	// gnutls_transport_set_int(session, PCS->getFD());
	gnutls_transport_set_ptr(session, reinterpret_cast<gnutls_transport_ptr_t>(PCS->getFD()));
	
	// gnutls_handshake_set_timeout(session,GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT);
	
	int ret = -1;
	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	if (ret < 0) 
	{
		libmaus::exception::LibMausException lme;
		lme.getStream() << "GnuTLSSocket: TLS handshake failed : " << gnutls_strerror(ret) << "\n";
		lme.finish();
		throw lme;
	} 
	else
	{
		#if 0
		char *desc = gnutls_session_get_desc(session);			
		std::cerr << "GnuTLSSocket: TLS session info " << desc << std::endl;
		gnutls_free(desc);
		#endif
		#if 0
		std::cerr << "GnuTLSSocket: TLS session ok" << std::endl;
		#endif
	}
	#else
	libmaus::exception::LibMausException lme;
	lme.getStream() << "GnuTLSSocket: no support for GNU TLS present.\n";
	lme.finish();
	throw lme;
	#endif
}

libmaus::network::GnuTLSSocket::~GnuTLSSocket()
{
	#if defined(LIBMAUS_HAVE_GNUTLS)
	gnutls_bye(session, GNUTLS_SHUT_RDWR);
	gnutls_deinit(session);
	gnutls_certificate_free_credentials(xcred);
	#endif
}

void libmaus::network::GnuTLSSocket::write(std::string const & s)
{
	write(s.c_str(),s.size());
}

void libmaus::network::GnuTLSSocket::write(
	#if defined(LIBMAUS_HAVE_GNUTLS)
	char const * p, size_t n
	#else
	char const *, size_t	
	#endif
)
{
	#if defined(LIBMAUS_HAVE_GNUTLS)
	while ( n )
	{
		ssize_t const r = gnutls_record_send(session, p, n);
		
		if ( r < 0 )
		{
			switch ( r )
			{
				case GNUTLS_E_INTERRUPTED:
				case GNUTLS_E_AGAIN:
					break;
				default:
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "GnuTLSSocket::write failed with error " << gnutls_strerror(r) << "\n";
					lme.finish();
					throw lme;		
				}
			}
		}
		else
		{
			p += r;
			n -= r;
		}
	}
	#endif
}

ssize_t libmaus::network::GnuTLSSocket::readPart(
	#if defined(LIBMAUS_HAVE_GNUTLS)
	char * p, size_t n
	#else
	char *, size_t
	#endif
)
{
	#if defined(LIBMAUS_HAVE_GNUTLS)
	ssize_t r;
	unsigned int loops = 0;
	unsigned int const maxloops = 10;
	
	while ( (r = gnutls_record_recv(session, p, n)) < 0 )
	{
	
		if ( gnutls_error_is_fatal(r) ) 
		{
			libmaus::exception::LibMausException lme;
			lme.getStream() << "GnuTLSSocket::readPart failed with error " << gnutls_strerror(r) << "\n";
			lme.finish();
			throw lme;				
		}
		else
		{
			std::cerr << "GnuTLSSocket::readPart, warning: " << gnutls_strerror(r) << "\n";
			
			if ( ++loops > maxloops )
			{
				libmaus::exception::LibMausException lme;
				lme.getStream() << "GnuTLSSocket::readPart failed with error " << gnutls_strerror(r) << "\n";
				lme.finish();
				throw lme;						
			}
		}
	}
	
	if ( r == 0 )
	{
		return 0;
	}
	else
	{
		return r;
	}
	#else
	return -1;
	#endif
}

ssize_t libmaus::network::GnuTLSSocket::read(
	#if defined(LIBMAUS_HAVE_GNUTLS)
	char * p, size_t n
	#else
	char *, size_t
	#endif
)
{
	#if defined(LIBMAUS_HAVE_GNUTLS)
	ssize_t r = 0;
	
	while ( n )
	{
		ssize_t const t = readPart(p,n);
		assert ( t >= 0 );
		
		if ( ! t )
			return r;
		else
		{
			p += t;
			n -= t;
			r += t;
		}
	}
	
	return r;
	#else
	return -1;
	#endif
}
