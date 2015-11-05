/*
    libmaus2
    Copyright (C) 2009-2013 German Tischler
    Copyright (C) 2011-2013 Genome Research Limited

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

#if ! defined(GETDOTTEDADDRESS_HPP)
#define GETDOTTEDADDRESS_HPP

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <string>
#include <map>

#include <libmaus2/exception/LibMausException.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/network/Interface.hpp>
#include <libmaus2/network/GetHostName.hpp>

namespace libmaus2
{
	namespace network
	{
		struct GetDottedAddress
		{
			static std::string getDottedAdress(std::string const & hostname)
			{
				struct hostent * he = gethostbyname2(hostname.c_str(),AF_INET);

				if ( ! he )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "failed to get address for " << hostname << " via gethostbyname: " << hstrerror(h_errno);
					se.finish();
					throw se;
				}

				if ( he->h_addr_list[0] == 0 )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "failed to get address for " << hostname << " via gethostbyname (no address returned)";
					se.finish();
					throw se;
				}

				::libmaus2::autoarray::AutoArray<char> B(256);
				std::string const dotted = inet_ntop(AF_INET,&(he->h_addr_list[0]),B.get(),B.size());

				return dotted;
			}

			static std::vector< std::vector <uint8_t> > getIP4Address(std::string const & hostname)
			{
				struct hostent * he = gethostbyname2(hostname.c_str(),AF_INET);

				if ( ! he )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "failed to get address for " << hostname << " via gethostbyname: " << hstrerror(h_errno);
					se.finish();
					throw se;
				}

				std::vector< std::vector <uint8_t> > V;

				for ( uint64_t j = 0; he->h_addr_list[j]; ++j )
				{
					in_addr const * addr = reinterpret_cast< in_addr const * >(he->h_addr_list[j]);
					uint32_t const adr32 = ntohl(addr->s_addr);
					std::vector < uint8_t > VV;
					VV.push_back( (adr32 >> 24) & 0xFF );
					VV.push_back( (adr32 >> 16) & 0xFF );
					VV.push_back( (adr32 >>  8) & 0xFF );
					VV.push_back( (adr32 >>  0) & 0xFF );
					V.push_back(VV);

				}

				return V;
			}

			static std::vector< std::vector <uint8_t> > getIP4Address()
			{
				return getIP4Address(::libmaus2::network::GetHostName::getHostName());
			}

			static std::vector <uint8_t> getClassC(std::string const & hostname)
			{
				std::vector< std::vector <uint8_t> > adr = getIP4Address(hostname);
				if ( ! adr.size() )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Unable to get IPv4 address for hostname " << hostname << std::endl;
					se.finish();
					throw se;
				}
				std::vector < uint8_t > V = adr[0];
				V[3] = 0;
				return V;
			}

			static std::map < std::vector <uint8_t>, std::vector < std::string > >
				getClassC(std::vector < std::string > const & hostnames)
			{
				std::map < std::vector <uint8_t>, std::vector < std::string > > S;
				for ( uint64_t i = 0; i < hostnames.size(); ++i )
					S[getClassC(hostnames[i])].push_back(hostnames[i]);
				return S;
			}
		};
	}
}
#endif
