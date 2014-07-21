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
#include <libmaus/network/GnuTLSSocket.hpp>

int main()
{
	try
	{
		// GnuTLSSocket sock("agressoweb.internal.sanger.ac.uk",443,0,0,true);		
		// GnuTLSSocket sock("mail.google.com",443,0,0,true);		
		//GnuTLSSocket sock("webmail.sanger.ac.uk",443,0,0,true);		
		libmaus::network::GnuTLSSocket sock("www.sanger.ac.uk",443,"/etc/ssl/certs/ca-certificates.crt","/etc/ssl/certs/",true);		

		char buf[1024];		
		ssize_t r = -1;
		sock.write(std::string("GET / HTTP/1.0\r\n\r\n"));
		while ( (r=sock.readPart(&buf[0],sizeof(buf)/sizeof(buf[0]))) > 0 )
		{
			std::cout.write(&buf[0],r);
		}
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
