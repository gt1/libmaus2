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
#if ! defined(LIBMAUS2_NETWORK_HTTPBODYWRAPPER_HPP)
#define LIBMAUS2_NETWORK_HTTPBODYWRAPPER_HPP

#include <libmaus2/network/HttpBody.hpp>

namespace libmaus2
{
	namespace network
	{
		struct HttpBodyWrapper
		{
			private:
			libmaus2::network::HttpBody object;

			public:
			HttpBodyWrapper(std::istream & rin, bool const rischunked, int64_t const rlength)
			: object(rin,rischunked,rlength)
			{

			}

			libmaus2::network::HttpBody & getHttpBody()
			{
				return object;
			}
		};
	}
}
#endif
