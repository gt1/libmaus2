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
#if ! defined(LIBMAUS2_NETWORK_HTTPHEADER_HPP)
#define LIBMAUS2_NETWORK_HTTPHEADER_HPP

#include <libmaus2/network/HttpAbsoluteUrl.hpp>
#include <libmaus2/network/Socket.hpp>
#include <libmaus2/network/OpenSSLSocket.hpp>
#include <libmaus2/network/GnuTLSSocket.hpp>
#include <libmaus2/network/SocketInputStream.hpp>
#include <libmaus2/util/stringFunctions.hpp>
#include <deque>

namespace libmaus2
{
	namespace network
	{
		struct HttpHeader
		{
			static std::string despace(std::string const & s)
			{
				std::deque<char> d(s.begin(),s.end());
				while ( d.size() && isspace(d.front()) )
					d.pop_front();
				while ( d.size() && isspace(d.back()) )
					d.pop_back();
				return std::string(d.begin(),d.end());
			}
			
			static std::string tolower(std::string s)
			{
				for ( uint64_t i = 0; i < s.size(); ++i )
					s[i] = ::std::tolower(s[i]);
				return s;
			}

			std::string statusline;
			std::map<std::string,std::string> fields;
			::libmaus2::network::HttpAbsoluteUrl url;

			libmaus2::network::ClientSocket::unique_ptr_type CS;
			libmaus2::network::OpenSSLSocket::unique_ptr_type OS;
			libmaus2::network::GnuTLSSocket::unique_ptr_type GTLSIOS;
			libmaus2::network::SocketInputOutputInterface * SIOS;
			libmaus2::network::SocketInputStream::unique_ptr_type SIS;

			int64_t getContentLength() const
			{
				if ( fields.find("content-length") == fields.end() )
					return -1;
				
				std::string const scontentlength = fields.find("content-length")->second;
				
				std::istringstream scontentlengthistr(scontentlength);
				
				int64_t contentlength;
				scontentlengthistr >> contentlength;
				
				if ( ! scontentlengthistr )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "HttpHeader: cannot parse content length field value " << scontentlength << std::endl;
					lme.finish();
					throw lme;		
				}
				
				return contentlength;
			}
			
			std::istream & getStream()
			{
				return *SIS;
			}
			
			static bool hasHttpProxy()
			{
				char const * proxystring = getenv("http_proxy");
				
				if ( ! proxystring )
					return false;

				if ( ! strlen(proxystring) )
					return false;
					
				if ( 
					!
						(
							libmaus2::network::HttpAbsoluteUrl::isHttpAbsoluteUrl(proxystring)
							||
							libmaus2::network::HttpAbsoluteUrl::isHttpsAbsoluteUrl(proxystring)
						)
				)
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "HttpHeader: unknown http_proxy setting " << proxystring << std::endl;
					lme.finish();
					throw lme;				
				}
				
				return true;
			}

			static bool hasHttpsProxy()
			{
				char const * proxystring = getenv("https_proxy");
				
				if ( ! proxystring )
					return false;
					
				if ( ! strlen(proxystring) )
					return false;
					
				if ( 
					!
						(
							libmaus2::network::HttpAbsoluteUrl::isHttpAbsoluteUrl(proxystring)
							||
							libmaus2::network::HttpAbsoluteUrl::isHttpsAbsoluteUrl(proxystring)
						)
				)
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "HttpHeader: unknown https_proxy setting " << proxystring << std::endl;
					lme.finish();
					throw lme;				
				}
				
				return true;
			}
			
			struct InitParameters
			{
				std::string method;
				std::string addreq;
				std::string host;
				std::string path;
				unsigned int port;
				bool ssl;
				
				InitParameters() : port(0), ssl(false) {}
				InitParameters(
					std::string rmethod,
					std::string raddreq,
					std::string rhost,
					std::string rpath,
					unsigned int rport,
					bool rssl			
				)
				: method(rmethod), addreq(raddreq), host(rhost), path(rpath), port(rport), ssl(rssl)
				{
				
				}
				
				bool operator==(InitParameters const & o) const
				{
					return
						method==o.method &&
						addreq==o.addreq &&
						host==o.host &&
						path==o.path &&
						port==o.port &&
						ssl==o.ssl;
				}
				bool operator<(InitParameters const & o) const
				{
					if ( method != o.method ) return method < o.method;
					if ( addreq != o.addreq ) return addreq < o.addreq;
					if ( host != o.host ) return host < o.host;
					if ( path != o.path ) return path < o.path;
					if ( port != o.port ) return port < o.port;
					if ( ssl != o.ssl ) return ssl < o.ssl;
					return false;
				}
			};
			
			void init(
				std::string method, 
				std::string addreq, 
				std::string host, 
				std::string path, unsigned int port = 80, bool ssl = false
			)
			{
				bool headercomplete = false;
				
				std::set<InitParameters> seen;

				while ( ! headercomplete )
				{
					InitParameters const initparams(method,addreq,host,path,port,ssl);
					
					if ( seen.find(initparams) != seen.end() )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "HttpHeader: redirect loop detected, method=" << method << " host=" << host << " path=" << path << " port=" << port << " ssl=" << ssl << std::endl;
						lme.finish();
						throw lme;	
					}
					
					seen.insert(initparams);
				
					fields.clear();
					
					CS.reset();
					OS.reset();
					SIS.reset();
					GTLSIOS.reset();
					SIOS = 0;
					
					bool const hasproxy =
						(ssl && hasHttpsProxy())
						||
						((!ssl) && hasHttpProxy());
						
					if ( hasproxy )
					{
						HttpAbsoluteUrl proxyurl =
							ssl ? HttpAbsoluteUrl(getenv("https_proxy")) : HttpAbsoluteUrl(getenv("http_proxy"));
						
						// std::cerr << "[D] using proxy " << proxyurl << std::endl;
							
						if ( proxyurl.ssl )
						{
							#if defined(LIBMAUS2_HAVE_GNUTLS)
							libmaus2::network::GnuTLSSocket::unique_ptr_type tGTLSIOS(new libmaus2::network::GnuTLSSocket(proxyurl.host,proxyurl.port,"/etc/ssl/certs/ca-certificates.crt","/etc/ssl/certs",true));
							GTLSIOS = UNIQUE_PTR_MOVE(tGTLSIOS);
							SIOS = GTLSIOS.get();
							#else	
							libmaus2::network::OpenSSLSocket::unique_ptr_type tOS(new libmaus2::network::OpenSSLSocket(proxyurl.host,proxyurl.port,0,"/etc/ssl/certs",true));
							OS = UNIQUE_PTR_MOVE(tOS);						
							SIOS = OS.get();
							#endif

							libmaus2::network::SocketInputStream::unique_ptr_type tSIS(new libmaus2::network::SocketInputStream(*SIOS,64*1024));
							SIS = UNIQUE_PTR_MOVE(tSIS);
						}
						else
						{
							libmaus2::network::ClientSocket::unique_ptr_type tCS(new libmaus2::network::ClientSocket(proxyurl.port,proxyurl.host.c_str()));
							CS = UNIQUE_PTR_MOVE(tCS);

							SIOS = CS.get();
						
							libmaus2::network::SocketInputStream::unique_ptr_type tSIS(new libmaus2::network::SocketInputStream(*CS,64*1024));
							SIS = UNIQUE_PTR_MOVE(tSIS);
						}						
					}
					else
					{
						if ( ssl )
						{
							#if defined(LIBMAUS2_HAVE_GNUTLS)
							libmaus2::network::GnuTLSSocket::unique_ptr_type tGTLSIOS(new libmaus2::network::GnuTLSSocket(host,port,"/etc/ssl/certs/ca-certificates.crt","/etc/ssl/certs",true));
							GTLSIOS = UNIQUE_PTR_MOVE(tGTLSIOS);
							
							SIOS = GTLSIOS.get();
							#else
							libmaus2::network::OpenSSLSocket::unique_ptr_type tOS(new libmaus2::network::OpenSSLSocket(host,port,0,"/etc/ssl/certs",true));
							OS = UNIQUE_PTR_MOVE(tOS);
						
							SIOS = OS.get();
							#endif

							libmaus2::network::SocketInputStream::unique_ptr_type tSIS(new libmaus2::network::SocketInputStream(*SIOS,64*1024));
							SIS = UNIQUE_PTR_MOVE(tSIS);
						}
						else
						{
							libmaus2::network::ClientSocket::unique_ptr_type tCS(new libmaus2::network::ClientSocket(port,host.c_str()));
							CS = UNIQUE_PTR_MOVE(tCS);

							SIOS = CS.get();
						
							libmaus2::network::SocketInputStream::unique_ptr_type tSIS(new libmaus2::network::SocketInputStream(*CS,64*1024));
							SIS = UNIQUE_PTR_MOVE(tSIS);
						}
					}
					
					std::ostringstream reqastr;

					if ( hasproxy )
					{
						reqastr << method << " " 
							<< (ssl ? "https" : "http")
							<< "://"
							<< host
							<< ":"
							<< port
							<< path 
							<< " HTTP/1.1\r\n";
						reqastr << "Connection: close\r\n";
					}
					else
					{
						reqastr << method << " " << path << " HTTP/1.1\r\n";
						reqastr << "Host: " << host << "\r\n";
					}

					reqastr << addreq;
					reqastr << "\r\n";

					// send request
					std::string const reqa = reqastr.str();
					SIOS->write(reqa.c_str(),reqa.size());

					libmaus2::autoarray::AutoArray<char> c(128,false);
					char last4[4] = {0,0,0,0};
					bool done = false;
					
					std::ostringstream headstr;
					
					while ( !done )
					{
						SIS->readsome(c.begin(),c.size());
						ssize_t r = SIS->gcount();
						
						// buffer is empty, try to fill it
						if ( ! r )
						{
							SIS->read(c.begin(),1);
							r = SIS->gcount();
							
							if ( ! r )
							{
								libmaus2::exception::LibMausException lme;
								lme.getStream() << "HttpHeader: unexpected EOF/error while reading header" << std::endl;
								lme.finish();
								throw lme;	
							}
							else
							{
								SIS->unget();
								continue;
							}
						}
						
						for ( ssize_t i = 0; (!done) && i < r; ++i )
						{
							headstr.put(c[i]);
							
							last4[0] = last4[1];
							last4[1] = last4[2];
							last4[2] = last4[3];
							last4[3] = c[i];
						
							if ( 
								last4[0] == '\r' && 
								last4[1] == '\n' && 
								last4[2] == '\r' && 
								last4[3] == '\n'
							)
							{
								for ( ssize_t j = i+1; j < r; ++j )
									SIS->unget();

								// SIS->clear();
								done = true;
							}
						}			
					}
					
					std::istringstream linestr(headstr.str());
					std::vector<std::string> lines;
					while ( linestr )
					{
						std::string line;
						std::getline(linestr,line);
						
						while ( line.size() && isspace(line[line.size()-1]) )
						{
							line = line.substr(0,line.size()-1);
						}
						
						if ( line.size() )
							lines.push_back(line);
					}
					
					if ( ! lines.size() )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "HttpHeader: unable to get header" << std::endl;
						lme.finish();
						throw lme;
					}
					
					statusline = lines[0];
					std::vector<std::string> statustokens;
					for ( uint64_t i = 0; i < statusline.size(); )
					{
						while ( i < statusline.size() && isspace(statusline[i]) )
							++i;

						uint64_t j = i;					
						while ( j < statusline.size() && (!isspace(statusline[j])) )
							++j;
							
						std::string const token = statusline.substr(i,j-i);
						
						i = j;
						
						statustokens.push_back(token);
					}
					
					if ( statustokens.size() < 2 )
					{			
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "HttpHeader: status line " << statusline << " invalid" << std::endl;
						lme.finish();
						throw lme;
					}
				
					std::string const replyformat = statustokens[0];	
					std::istringstream statuscodeistr(statustokens[1]);
					uint64_t statuscode;
					statuscodeistr >> statuscode;
					
					if ( ! statuscodeistr )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "HttpHeader: invalid status code " << statustokens[1] << std::endl;
						lme.finish();
						throw lme;			
					}
					
					if (
						replyformat != "HTTP/1.0"
						&&
						replyformat != "HTTP/1.1"
					)
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "HttpHeader: unknown reply format " << replyformat << std::endl;
						lme.finish();
						throw lme;						
					}
					
					// std::cerr << "status code " << statuscode << std::endl;
					
					for ( uint64_t i = 1; i < lines.size(); ++i )
					{
						std::string const line = lines[i];
						
						uint64_t col = 0;
						while ( col < line.size() && line[col] != ':' )
							++col;
						
						if ( ! col || col == line.size() )
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "HttpHeader: invalid key value pair " << line << std::endl;
							lme.finish();
							throw lme;						
						}
						
						std::string const key = tolower(despace(line.substr(0,col)));
						std::string const val = despace(line.substr(col+1));
						
						fields[key] = val;
					}
					
					switch ( statuscode )
					{
						case 301:
						case 302:
						case 307:
						case 308:
						{				
							std::map<std::string,std::string>::const_iterator it = fields.end();
							
							if ( it == fields.end() )
								it = fields.find("Location");
							if ( it == fields.end() )
								it = fields.find("location");
							
							if ( it == fields.end() )
							{	
								libmaus2::exception::LibMausException lme;
								lme.getStream() << "HttpHeader: redirect status code " << statuscode << " but no location given" << std::endl;
								lme.finish();
								throw lme;						
							}
							
							std::string location = it->second;

							// std::cerr << "redirecting " << statuscode << " to " << location << std::endl;
							
							if ( 
								::libmaus2::network::HttpAbsoluteUrl::isHttpAbsoluteUrl(location) 
								||
								::libmaus2::network::HttpAbsoluteUrl::isHttpsAbsoluteUrl(location) 
							)
							{
								::libmaus2::network::HttpAbsoluteUrl url(location);
								host = url.host;
								port = url.port;
								path = url.path;
								ssl = url.ssl;
							}
							else if ( ::libmaus2::network::HttpAbsoluteUrl::isAbsoluteUrl(location) )
							{
								libmaus2::exception::LibMausException lme;
								lme.getStream() << "HttpHeader: unsupported protocol in location " << location << std::endl;
								lme.finish();
								throw lme;						
							}
							else
							{
								path = location;
							}
							
							break;
						}
						default:
						{
							headercomplete = true;
							break;
						}
					}	
				}	
				
				#if 0
				std::cerr << statusline << std::endl;
				for ( std::map<std::string,std::string>::const_iterator ita = fields.begin(); ita != fields.end(); ++ita )
				{
					std::cerr << ita->first << ": " << ita->second << std::endl;
				}
				#endif
				
				url.host = host;
				url.port = port;
				url.path = path;
				url.ssl = ssl;		
			}

			HttpHeader() {}
			HttpHeader(std::string method, std::string addreq, std::string url)
			{
				::libmaus2::network::HttpAbsoluteUrl absurl(url);
				init(method,addreq,absurl.host,absurl.path,absurl.port,absurl.ssl);
			}
			HttpHeader(std::string method, std::string addreq, std::string host, std::string path, unsigned int port = 80, bool ssl = false)
			{
				init(method,addreq,host,path,port,ssl);
			}
			
			bool isChunked() const
			{
				return (fields.find("transfer-encoding") != fields.end()) && (fields.find("transfer-encoding")->second == "chunked");
			}
			
			static int64_t getContentLength(std::string method, std::string addreq, std::string url)
			{
				try
				{
					HttpHeader header(method,addreq,url);
					return header.getContentLength();
				}
				catch(...)
				{
					return -1;
				}
			}

			std::set<std::string> getAccessControlAllowHeaders() const
			{
				std::set<std::string> V;
				if ( fields.find("access-control-allow-headers") == fields.end() )
					return V;
				std::string const v = fields.find("access-control-allow-headers")->second;
				std::deque<std::string> tokens = libmaus2::util::stringFunctions::tokenize(v,std::string(","));
				for ( uint64_t i = 0; i < tokens.size(); ++i )
				{
					std::string s = despace(tolower(tokens[i]));
					if ( s.size() )
						V.insert(s);
				}
				
				return V;
			}

			std::set<std::string> getAcceptRanges() const
			{
				std::set<std::string> V;
				if ( fields.find("accept-ranges") == fields.end() )
					return V;
				std::string const v = fields.find("accept-ranges")->second;
				std::deque<std::string> tokens = libmaus2::util::stringFunctions::tokenize(v,std::string(","));
				for ( uint64_t i = 0; i < tokens.size(); ++i )
				{
					std::string s = despace(tolower(tokens[i]));
					if ( s.size() )
						V.insert(s);
				}
				
				return V;
			}
			
			bool hasRanges() const
			{
				std::set<std::string> acah = getAccessControlAllowHeaders();
				std::set<std::string> ar = getAcceptRanges();
				
				return
					acah.find("range") != acah.end()
					&&
					ar.find("bytes") != ar.end();
			}
		};
	}
}
#endif
