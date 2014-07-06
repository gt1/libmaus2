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
#include <libmaus/network/UrlInputStream.hpp>

#include <libmaus/util/ArgInfo.hpp>
#include <libmaus/network/FtpSocket.hpp>
#include <libmaus/network/FtpSocketInputStream.hpp>

int main(int argc, char * argv[])
{
	try
	{
		libmaus::util::ArgInfo const arginfo(argc,argv);
		std::string const url = arginfo.getRestArg<std::string>(0);

		// libmaus::network::FtpSocketInputStream FSIS(url,1024);
		libmaus::network::UrlInputStream FSIS(url,1024);

		int c = -1;
		while ( (c=FSIS.get()) >= 0 )
			std::cout.put(c);
		std::cout.flush();
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
