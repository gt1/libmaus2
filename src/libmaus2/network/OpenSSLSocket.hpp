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
#if ! defined(LIBMAUS2_NETWORK_OPENSSLSOCKET_HPP)
#define LIBMAUS2_NETWORK_OPENSSLSOCKET_HPP

#include <libmaus2/network/OpenSSLInit.hpp>
#include <libmaus2/network/SocketInputOutputInterface.hpp>
#include <cstring>

namespace libmaus2
{
	namespace network
	{
		struct OpenSSLSocket : public ::libmaus2::network::OpenSSLInit, public SocketInputOutputInterface
		{
			typedef OpenSSLSocket this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
		
			#if defined(LIBMAUS2_HAVE_OPENSSL)
			SSL_CTX * ctx;
			BIO * bio;
			SSL * ssl;
			#endif
			
			void cleanup()
			{
				#if defined(LIBMAUS2_HAVE_OPENSSL)
				if ( bio )
				{
					BIO_free_all(bio);
					bio = 0;
				}
				if ( ctx )
				{
					SSL_CTX_free(ctx);
					ctx = 0;
				}
				#endif
			}
			
			OpenSSLSocket(
				std::string const & 
					#if defined(LIBMAUS2_HAVE_OPENSSL)
					hostname
					#endif
				, 
				unsigned int 
					#if defined(LIBMAUS2_HAVE_OPENSSL)
					port
					#endif
				,
				char const * 
					#if defined(LIBMAUS2_HAVE_OPENSSL)
					certfile
					#endif
				,
				char const * 
					#if defined(LIBMAUS2_HAVE_OPENSSL)
					certdir
					#endif
				,
				bool const
					#if defined(LIBMAUS2_HAVE_OPENSSL)
					checkcertificate
					#endif
			)
			#if defined(LIBMAUS2_HAVE_OPENSSL)	 
			: ctx(0), bio(0), ssl(0)
			#endif
			{
				#if defined(LIBMAUS2_HAVE_OPENSSL)	 
				ctx = SSL_CTX_new(SSLv23_client_method());

				if ( ! ctx )
				{
					cleanup();
					
					libmaus2::exception::LibMausException lme;
					unsigned long r;
					while ( (r=ERR_get_error()) )
						lme.getStream() << ERR_error_string(r,NULL) << std::endl;
					lme.finish();
					throw lme;
				}

				if(! SSL_CTX_load_verify_locations(ctx,certfile,certdir) )
				{
					cleanup();
					
					libmaus2::exception::LibMausException lme;
					unsigned long r;
					while ( (r=ERR_get_error()) )
						lme.getStream() << ERR_error_string(r,NULL) << std::endl;
						
					lme.finish();
					throw lme;
				}

				bio = BIO_new_ssl_connect(ctx);
			
				if ( ! bio )
				{	
					cleanup();
					
					libmaus2::exception::LibMausException lme;
					unsigned long r;
					while ( (r=ERR_get_error()) )
						lme.getStream() << ERR_error_string(r,NULL) << std::endl;
					lme.finish();
					throw lme;
				}

				BIO_get_ssl(bio, & ssl);

				SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);

				std::ostringstream bioostr;
				bioostr << hostname << ":" << port;

				BIO_set_conn_hostname(bio, bioostr.str().c_str());

				/* Verify the connection opened and perform the handshake */
				if(BIO_do_connect(bio) <= 0)
				{
					cleanup();
					
					libmaus2::exception::LibMausException lme;
					unsigned long r;
					while ( (r=ERR_get_error()) )
						lme.getStream() << ERR_error_string(r,NULL) << std::endl;
					lme.finish();
					throw lme;
				}

				if(BIO_do_handshake(bio) <= 0) 
				{
					cleanup();
					
					libmaus2::exception::LibMausException lme;
					unsigned long r;
					while ( (r=ERR_get_error()) )
						lme.getStream() << ERR_error_string(r,NULL) << std::endl;

					lme.finish();
					throw lme;
				}                                                     
				
				if( SSL_get_peer_certificate(ssl) != NULL )
				{
					long verres;
					if( (verres=SSL_get_verify_result(ssl)) != X509_V_OK)
					{
						if ( checkcertificate )
						{
							cleanup();
							
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "server certificate verification failed: " << verres << std::endl;
				
							unsigned long r;
							while ( (r=ERR_get_error()) )
								lme.getStream() << ERR_error_string(r,NULL) << std::endl;
					
							lme.finish();
							throw lme;
						}
						else
						{
							std::cerr << "[V] warning, certificate validation failed: " << verres << std::endl;
						}
					}
				}
				else
				{
					if ( checkcertificate )
					{
						cleanup();
							
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "server did not present a certificate" << std::endl;
				
						unsigned long r;
						while ( (r=ERR_get_error()) )
							lme.getStream() << ERR_error_string(r,NULL) << std::endl;
				
						lme.finish();
						throw lme;
					}
					else
					{
						std::cerr << "[V] server did not present a certificate" << std::endl;
					}		
				}
				
				SSL_CIPHER const * cipher = SSL_get_current_cipher(ssl);
				std::cerr << "[D] using cipher " << SSL_CIPHER_get_name(cipher) << " version " << SSL_CIPHER_get_version(cipher) << std::endl;

				char ciphdesc[256];
				std::memset(&ciphdesc[0],0,sizeof(ciphdesc));
				char const * desc = SSL_CIPHER_description(cipher,&ciphdesc[0],sizeof(ciphdesc));
				
				std::cerr << "[D] cipher description " << desc;
				// int     SSL_CIPHER_get_bits(const SSL_CIPHER *c,int *alg_bits);				
				#else
				libmaus2::exception::LibMausException lme;
				lme.getStream() << "OpenSSLSocket: openssl support is not present" << std::endl;
				lme.finish();
				throw lme;				
				#endif
			}
			
			~OpenSSLSocket()
			{
				cleanup();
			}
			
			void write(
				char const * 
					#if defined(LIBMAUS2_HAVE_OPENSSL)
					p
					#endif
				,
				size_t 
					#if defined(LIBMAUS2_HAVE_OPENSSL)
					n
					#endif
			)
			{
				#if defined(LIBMAUS2_HAVE_OPENSSL)
				while ( n )
				{
					long w = -1;

					if( (w=BIO_write(bio, p, n)) <= 0)
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "OpenSSLSocket::write() failed: " << std::endl;
				
						unsigned long r;
						while ( (r=ERR_get_error()) )
							lme.getStream() << ERR_error_string(r,NULL) << std::endl;
				
						lme.finish();
						throw lme;
					
					}
					else
					{
						p += w;
						n -= w;
					}
				}
				#else
				libmaus2::exception::LibMausException lme;
				lme.getStream() << "OpenSSLSocket::write() failed: openssl support is not present" << std::endl;
				lme.finish();
				throw lme;		
				#endif
			}
			
			ssize_t readPart(
				char * 
					#if defined(LIBMAUS2_HAVE_OPENSSL)
					p
					#endif
				, 
				size_t 
					#if defined(LIBMAUS2_HAVE_OPENSSL)
					n
					#endif
			)
			{
				#if defined(LIBMAUS2_HAVE_OPENSSL)
				long r = BIO_read(bio,p,n);

				if ( r < 0 )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "OpenSSLSocket::read() failed: " << std::endl;
				
					unsigned long r;
					while ( (r=ERR_get_error()) )
						lme.getStream() << ERR_error_string(r,NULL) << std::endl;
				
					lme.finish();
					throw lme;		
				}
				else
				{
					return static_cast<uint64_t>(r);
				}
				#else
				libmaus2::exception::LibMausException lme;
				lme.getStream() << "OpenSSLSocket::read() failed: openssl support is not present" << std::endl;
				lme.finish();
				throw lme;		
				#endif
			}

			ssize_t read(
				char * 
					p
				, 
				size_t 
					n
			)
			{
				uint64_t r = 0;
				
				while ( n )
				{
					uint64_t lr = readPart(p,n);
					
					if ( lr == 0 )
						return r;
					
					p += lr;
					n -= lr;
				}
				
				return r;
			}
		};
	}
}
#endif
