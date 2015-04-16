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
#if ! defined(LIBMAUS2_NETWORK_FTPURL_HPP)
#define LIBMAUS2_NETWORK_FTPURL_HPP

#include <string>
#include <sstream>
#include <libmaus2/exception/LibMausException.hpp>

namespace libmaus2
{
	namespace network
	{
		struct FtpUrl
		{
			std::string host;
			unsigned int port;
			std::string path;

			static bool parseUrl(std::string url, std::string & host, unsigned int & port, std::string & path)
			{
				host = std::string();
				port = 21;
				path = std::string();
			
				std::string const prot = "ftp://";
				if ( url.size() <= prot.size() || url.substr(0,prot.size()) != prot )
					return false;
					
				url = url.substr(prot.size());
				
				size_t const firstslash = url.find_first_of('/');
				
				if ( firstslash != std::string::npos )
				{
					host = url.substr(0,firstslash);
					path = url.substr(firstslash);
				}
				else
				{
					host = url;
				}
				
				size_t const col = host.find_first_of(':');
				
				if ( col != std::string::npos )
				{
					std::istringstream portistr(host.substr(col+1));
					portistr >> port;
					
					if ( ! portistr )
						return false;
						
					host = host.substr(0,col);
				}
				
				if ( !path.size() )
					path = "/";
				
				return true;
			}

			FtpUrl()
			{
			
			}
			
			FtpUrl(std::string const & url)
			{
				if ( ! parseUrl(url,host,port,path) )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "invalid ftp url " << url << std::endl;
					lme.finish();
					throw lme;
				}
			}
		};
	}
}
#endif
