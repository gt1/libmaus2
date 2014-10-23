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
#include <libmaus/util/LineBuffer.hpp>
#include <iostream>

#include <libmaus/bambam/SamInfo.hpp>

int main(int argc, char * argv[])
{
	try
	{
		libmaus::util::ArgInfo const arginfo(argc,argv);
		char const * a = 0;
		char const * e = 0;
		
		if ( arginfo.restargs.size() )
		{
			for ( uint64_t i = 0; i < arginfo.restargs.size(); ++i )
			{
				std::string const fn = arginfo.restargs.at(i);
				libmaus::aio::CheckedInputStream CIS1(fn);
				libmaus::aio::CheckedInputStream CIS2(fn);
				libmaus::util::LineBuffer LB(CIS1);
				
				while ( LB.getline(&a,&e) )
				{
					std::string const line(a,e);
					std::string refline;
					std::getline(CIS2,refline);
					
					if ( line != refline )
					{
						std::cerr << line << "\n!=\n" << refline << "\n";
						return EXIT_FAILURE;
					}
				}
			}
		}
		else
		{
			libmaus::bambam::SamInfo SI;
			libmaus::util::LineBuffer LB(std::cin,1);
			while ( LB.getline(&a,&e) )
			{
				std::cout << std::string(a,e) << "\n";		
				SI.parseSamLine(a,e);
			}
		}
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;	
	}
}
