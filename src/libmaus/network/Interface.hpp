/*
    libmaus
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

#if ! defined(INTERFACE_HPP)
#define INTERFACE_HPP

#include <string>
#include <vector>
#include <libmaus/types/types.hpp>
#include <libmaus/util/StringSerialisation.hpp>

namespace libmaus
{
	namespace network
	{
		struct Interface
		{
			std::string name;
			std::vector < uint8_t > addr;
			std::vector < uint8_t > baddr;
			std::vector < uint8_t > naddr;
			
			Interface()
			: addr(4), baddr(4), naddr(4)
			{}
			Interface(std::string const & rname, std::vector<uint8_t> const & raddr, std::vector<uint8_t> const & rbaddr,
				std::vector<uint8_t> const & rnaddr)
			: name(rname), addr(raddr),baddr(rbaddr), naddr(rnaddr) {}
			
			std::ostream & serialise(std::ostream & out) const
			{
				::libmaus::util::StringSerialisation::serialiseString(out,name);
				::libmaus::util::NumberSerialisation::serialiseNumberVector(out,addr);
				::libmaus::util::NumberSerialisation::serialiseNumberVector(out,baddr);
				::libmaus::util::NumberSerialisation::serialiseNumberVector(out,naddr);
				return out;
			}
			
			std::string serialise() const
			{
				std::ostringstream out;
				serialise(out);
				return out.str();
			}

			Interface(std::istream & in)
			: name(::libmaus::util::StringSerialisation::deserialiseString(in)),
			  addr(::libmaus::util::NumberSerialisation::deserialiseNumberVector<uint8_t>(in)),
			  baddr(::libmaus::util::NumberSerialisation::deserialiseNumberVector<uint8_t>(in)),
			  naddr(::libmaus::util::NumberSerialisation::deserialiseNumberVector<uint8_t>(in))
			{
			
			}
			
			Interface(std::string const & s)
			{
				std::istringstream in(s);
				name = ::libmaus::util::StringSerialisation::deserialiseString(in);
				addr = ::libmaus::util::NumberSerialisation::deserialiseNumberVector<uint8_t>(in);
				baddr = ::libmaus::util::NumberSerialisation::deserialiseNumberVector<uint8_t>(in);
				naddr = ::libmaus::util::NumberSerialisation::deserialiseNumberVector<uint8_t>(in);
			}
			
			uint8_t getLowA() const { return addr[0]&naddr[0]; }
			uint8_t getLowB() const { return addr[1]&naddr[1]; }
			uint8_t getLowC() const { return addr[2]&naddr[2]; }
			uint8_t getLowD() const { return addr[3]&naddr[3]; }
			uint8_t getSizeA() const { return static_cast<unsigned int>(static_cast<uint8_t>(~(naddr[0]))); }
			uint8_t getSizeB() const { return static_cast<unsigned int>(static_cast<uint8_t>(~(naddr[1]))); }
			uint8_t getSizeC() const { return static_cast<unsigned int>(static_cast<uint8_t>(~(naddr[2]))); }
			uint8_t getSizeD() const { return static_cast<unsigned int>(static_cast<uint8_t>(~(naddr[3]))); }
			uint8_t getHighA() const { return getLowA() + getSizeA(); }
			uint8_t getHighB() const { return getLowB() + getSizeB(); }
			uint8_t getHighC() const { return getLowC() + getSizeC(); }
			uint8_t getHighD() const { return getLowD() + getSizeD(); }
			
			bool onInterface(std::vector<uint8_t> const & V) const
			{
				return
					(V[0] >= getLowA() && V[0] <= getHighA()) &&
					(V[1] >= getLowB() && V[1] <= getHighB()) &&
					(V[2] >= getLowC() && V[2] <= getHighC()) &&
					(V[3] >= getLowD() && V[3] <= getHighD());
			}
		};

		inline std::ostream & operator<< ( std::ostream & out, Interface const & interface)
		{
			uint8_t const netlowa = interface.getLowA();
			uint8_t const netlowb = interface.getLowB();
			uint8_t const netlowc = interface.getLowC();
			uint8_t const netlowd = interface.getLowD();
			uint8_t const nethigha = interface.getHighA();
			uint8_t const nethighb = interface.getHighB();
			uint8_t const nethighc = interface.getHighC();
			uint8_t const nethighd = interface.getHighD();
			
			out << "Interface(" << interface.name << "," 
				<< static_cast<int>(interface.addr[0]) << "." 
				<< static_cast<int>(interface.addr[1]) << "." 
				<< static_cast<int>(interface.addr[2]) << "." 
				<< static_cast<int>(interface.addr[3])
				<< ","
				<< static_cast<int>(interface.baddr[0]) << "." 
				<< static_cast<int>(interface.baddr[1]) << "." 
				<< static_cast<int>(interface.baddr[2]) << "." 
				<< static_cast<int>(interface.baddr[3])
				<< ","
				<< static_cast<int>(interface.naddr[0]) << "." 
				<< static_cast<int>(interface.naddr[1]) << "." 
				<< static_cast<int>(interface.naddr[2]) << "." 
				<< static_cast<int>(interface.naddr[3])
				<< ",";
			if ( nethigha-netlowa )
				out << "[" << static_cast<int>(netlowa) << "," << static_cast<int>(nethigha) << "].";
			else
				out << static_cast<int>(netlowa) << ".";
			if ( nethighb-netlowb )
				out << "[" << static_cast<int>(netlowb) << "," << static_cast<int>(nethighb) << "].";
			else
				out << static_cast<int>(netlowb) << ".";
			if ( nethighc-netlowc )
				out << "[" << static_cast<int>(netlowc) << "," << static_cast<int>(nethighc) << "].";
			else
				out << static_cast<int>(netlowc) << ".";
			if ( nethighd-netlowd )
				out << "[" << static_cast<int>(netlowd) << "," << static_cast<int>(nethighd) << "]";
			else
				out << static_cast<int>(netlowd);
				
			out << ")";
			
			return out;
		}
	}
}
#endif
