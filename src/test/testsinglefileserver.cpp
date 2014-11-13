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
#include <libmaus/util/ArgInfo.hpp>
#include <libmaus/network/SingleFileServer.hpp>

int main(int argc, char * argv[])
{
	try
	{
		libmaus::util::ArgInfo const arginfo(argc,argv);
		std::string const fn = arginfo.restargs.at(0);
		int const port = arginfo.getValue<int>("port",4444);
		uint64_t const backlog = arginfo.getValue<int>("backlog",128);
		std::string const hostname = arginfo.getUnparsedValue("hostname","localhost");

		libmaus::network::SingleFileServer server(fn,hostname,port,backlog);
		server.exec();
		std::cout << hostname << '\t' << server.port << '\t' << server.getPid() << std::endl;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
