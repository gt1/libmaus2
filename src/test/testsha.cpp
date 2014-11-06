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
#include <iostream>
#include <libmaus/digest/Digests.hpp>
#include <libmaus/util/Demangle.hpp>

template<typename crc>
void printCRC(std::string const & text)
{
	crc dig;
	dig.init();
	dig.update(reinterpret_cast<uint8_t const *>(text.c_str()),text.size());
	std::cout << libmaus::util::Demangle::demangle<crc>() << "\t" << std::hex << dig.digestui() << std::dec << std::endl;
}

int main()
{
	try
	{
		std::string const text = "Hello world!";

		printCRC<libmaus::digest::CRC32>(text);
		printCRC<libmaus::util::MD5>(text);

		#if defined(LIBMAUS_HAVE_NETTLE)
		printCRC<libmaus::digest::SHA1>(text);
		printCRC<libmaus::digest::SHA2_224>(text);
		printCRC<libmaus::digest::SHA2_256>(text);
		printCRC<libmaus::digest::SHA2_384>(text);
		printCRC<libmaus::digest::SHA2_512>(text);
		#else	
		libmaus::exception::LibMausException lme;
		lme.getStream() << "support for nettle library is not present" << std::endl;
		lme.finish();
		throw lme;
		#endif
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
