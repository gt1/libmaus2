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
#if ! defined(LIBMAUS_NETWORK_HTTPABSOLUTEURL_HPP)
#define LIBMAUS_NETWORK_HTTPABSOLUTEURL_HPP

#include <string>
#include <cstring>
#include <libmaus/exception/LibMausException.hpp>

namespace libmaus
{
	namespace network
	{
		struct HttpAbsoluteUrl
		{
			std::string host;
			unsigned int port;
			std::string path;

			static bool isAbsoluteUrl(std::string const & s)
			{
				if ( s.find("://") == std::string::npos )
					return false;
					
				std::string const prot = s.substr(0,s.find("://"));
				
				for ( uint64_t i = 0; i < prot.size(); ++i )
					if ( ! isalpha(prot[i]) )
						return false;
				
				return true;
			}
			
			static bool isHttpAbsoluteUrl(std::string const & s)
			{
				std::string const prefix = "http://";
				return (s.size() > prefix.size()) && s.substr(0,prefix.size()) == prefix;				
			}
			
			HttpAbsoluteUrl() {}
			HttpAbsoluteUrl(std::string const url)
			: port(80)
			{
				if ( ! isHttpAbsoluteUrl(url) )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "HttpAbsoluteUrl: malformed url " << url << std::endl;
					lme.finish();
					throw lme;		
				}
				
				std::string rest = url.substr(::std::strlen("http://"));
				
				uint64_t slash = 0;
				while ( slash < rest.size() && rest[slash] != '/' )
					++slash;
				
				host = rest.substr(0,slash);
				
				uint64_t col = host.size();
				for ( uint64_t i = 0; i < host.size(); ++i )
					if ( host[i] == ':' )
						col = i;
						
				bool alldig = true;
				for ( uint64_t i = col+1; i < host.size(); ++i )
					if ( !isdigit(host[i]) )
						alldig = false;
						
				if ( col != host.size() && alldig )
				{
					std::istringstream portistr(host.substr(col+1));
					portistr >> port;
					
					if ( !portistr )
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "HttpAbsoluteUrl: malformed url " << url << std::endl;
						lme.finish();
						throw lme;		
					}
					
					host = host.substr(0,col);
				}

				if ( slash != rest.size() )
					path = rest.substr(slash);
				
				if ( ! path.size() )
					path = "/";
				
				#if 0	
				std::cerr << "host " << host << std::endl;
				std::cerr << "port " << port << std::endl;
				std::cerr << "path " << path << std::endl;
				#endif
			}
		};

		std::ostream & operator<<(std::ostream & out, libmaus::network::HttpAbsoluteUrl const & url);
	}
}
#endif
