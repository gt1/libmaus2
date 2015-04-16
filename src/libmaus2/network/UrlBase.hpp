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
#if ! defined(LIBMAUS2_NETWORK_URLBASE_HPP)
#define LIBMAUS2_NETWORK_URLBASE_HPP

#include <string>
#include <cstring>
#include <libmaus2/exception/LibMausException.hpp>

namespace libmaus2
{
	namespace network
	{
		struct UrlBase
		{
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
			
			static std::string getProtocol(std::string const & s)
			{
				if ( ! isAbsoluteUrl(s) )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "UrlBase::getProtocol() called for non absolute url " << s << "\n";
					lme.finish();
					throw lme;
				}
				
				std::string const prot = s.substr(0,s.find("://"));
				
				return prot;
			}
		};
	}
}
#endif
