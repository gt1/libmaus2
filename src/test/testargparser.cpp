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
#include <libmaus2/util/ArgParser.hpp>
#include <iostream>

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgParser const arg(argc,argv);
		std::cout << arg << std::endl;
		for ( uint64_t i = 0; i < arg.size(); ++i )
			std::cout << "arg[" << i << "]=" << arg[i] << std::endl;
		std::cout << arg.getParsedRestArg<int>(3) << std::endl;
		std::cout << arg.getUnsignedNumericRestArg<uint64_t>(4) << std::endl;
		std::cout << arg["I"] << std::endl;
		std::cout << arg["splita"] << std::endl;
		std::cout << arg.getUnsignedNumericArg<uint64_t>("m") << std::endl;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
