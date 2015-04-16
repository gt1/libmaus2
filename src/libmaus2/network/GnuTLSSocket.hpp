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
#if ! defined(LIBMAUS_NETWORK_GNUTLSSOCKET_HPP)
#define LIBMAUS_NETWORK_GNUTLSSOCKET_HPP

#include <libmaus/network/GnuTLSInit.hpp>
#include <libmaus/network/Socket.hpp>
#include <libmaus/network/SocketInputOutputInterface.hpp>

#if defined(LIBMAUS_HAVE_GNUTLS)
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#endif

namespace libmaus
{
	namespace network
	{
		struct GnuTLSSocket : private libmaus::network::GnuTLSInit, public SocketInputOutputInterface
		{
			typedef GnuTLSSocket this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
		
			std::string const hostname;

			#if defined(LIBMAUS_HAVE_GNUTLS)
			gnutls_certificate_credentials_t xcred;	
			gnutls_session_t session;
			#endif

			libmaus::network::ClientSocket::unique_ptr_type PCS;
			
			#if defined(LIBMAUS_HAVE_GNUTLS)
			static int verify_certificate_callback(gnutls_session_t session);
			#endif

			GnuTLSSocket(
				std::string const & rhostname,
				unsigned int port,
				char const * certfile,
				char const * certdir,
				bool const checkcertificate
			);
			
			~GnuTLSSocket();
			
			void write(std::string const & s);
			void write(char const * p, size_t n);
			ssize_t readPart(char * p, size_t n);
			ssize_t read(char * p, size_t n);
		};
	}
}
#endif
