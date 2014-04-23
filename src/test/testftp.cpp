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
#include <libmaus/network/FtpSocket.hpp>

int main(int argc, char * argv[])
{
	try
	{
		libmaus::util::ArgInfo const arginfo(argc,argv);
		std::string const url = arginfo.getRestArg<std::string>(0);
		libmaus::network::FtpSocket ftpsock(url,0,true);
		std::istream & ftpstream = ftpsock.getStream();
		
		libmaus::autoarray::AutoArray<char> A(1024*1024,false);
		uint64_t r = 0;
		while ( ftpstream )
		{
			ftpstream.read(A.begin(),A.size());
			uint64_t const gcnt = ftpstream.gcount();
			std::cout.write(A.begin(),gcnt);
			r += gcnt;
			
			std::cerr << "\r" << std::string(80,' ') << "\r" << "[V] " << r/(1024*1024);
		}
		
		std::cout.flush();
		std::cerr << "\r" << std::string(80,' ') << "\r" << "[V] " << r/(1024*1024) << "\t" << r << "\t" << ftpsock.size << "\n";
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
