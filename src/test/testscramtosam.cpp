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
#include <libmaus/LibMausConfig.hpp>
#include <iostream>
#include <cstdlib>

#if ! defined(LIBMAUS_HAVE_IO_LIB)
int main()
{
	std::cerr << "io_lib support is not present." << std::endl;
	return EXIT_FAILURE;
}
#else
#include <libmaus/util/ArgInfo.hpp>
#include <libmaus/bambam/ScramDecoder.hpp>

int main(int argc, char * argv[])
{
	try
	{
		::libmaus::util::ArgInfo const arginfo(argc,argv);

		::libmaus::bambam::ScramDecoder dec("-","rc","");

		std::cout << dec.bamheader.text;
			
		while ( dec.readAlignment() )
		{
			std::cout << dec.formatAlignment() << std::endl;				
		}		
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
#endif
