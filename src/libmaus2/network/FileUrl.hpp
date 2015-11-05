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
#if ! defined(LIBMAUS2_NETWORK_FILEURL_HPP)
#define LIBMAUS2_NETWORK_FILEURL_HPP

#include <libmaus2/network/UrlBase.hpp>

namespace libmaus2
{
	namespace network
	{
		struct FileUrl : public UrlBase
		{
			std::string filename;

			static bool isFileUrl(std::string const & url)
			{
				return isAbsoluteUrl(url) && (getProtocol(url) == "file");
			}

			FileUrl(std::string const & url)
			{
				if ( isAbsoluteUrl(url) )
				{
					if ( getProtocol(url) == "file" )
					{
						filename = url.substr(7);
					}
					else
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "FileUrl: " << url << " is not a file protocol URL" << "\n";
						lme.finish();
						throw lme;
					}
				}
				else
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "FileUrl: " << url << " is not an URL" << "\n";
					lme.finish();
					throw lme;
				}
			}
		};
	}
}
#endif
