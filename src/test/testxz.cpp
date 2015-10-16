/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#include <libmaus2/lz/XzInputStream.hpp>
#include <iostream>

int main()
{
	try
	{
		int const bufsize = 8192;
		char inbuf[8192];
		libmaus2::lz::XzInputStream xzin(std::cin);
		xzin.exceptions ( std::istream::badbit );
		while ( xzin )
		{
			xzin.read(&inbuf[0],bufsize);
			std::cout.write(&inbuf[0],xzin.gcount());
		}
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
