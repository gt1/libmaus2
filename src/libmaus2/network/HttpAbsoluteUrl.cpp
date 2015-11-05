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
#include <libmaus2/network/HttpAbsoluteUrl.hpp>

std::ostream & libmaus2::network::operator<<(std::ostream & out, libmaus2::network::HttpAbsoluteUrl const & url)
{
	std::string const protocol = url.ssl ? "https" : "http";

	if ( (url.port == 80 && !url.ssl) || (url.port == 443 && url.ssl) )
		out << protocol << "://" << url.host << url.path;
	else
		out << protocol << "://" << url.host << ":" << url.port << url.path;
	return out;
}
