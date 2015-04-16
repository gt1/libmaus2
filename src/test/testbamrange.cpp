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

#include <libmaus/bambam/BamRangeDecoder.hpp>

int main(int argc, char * argv[])
{
	try
	{
		libmaus::util::ArgInfo const arginfo(argc,argv);
		
		std::string const bamname = arginfo.getRestArg<std::string>(0);
		std::string const range = arginfo.restargs.at(1); // "3:100000-200000";
		
		libmaus::bambam::BamRangeDecoder bamdec(bamname,range);
		while ( bamdec.readAlignment() )
		{
			libmaus::bambam::BamAlignment const & algn = bamdec.getAlignment();		
			std::cout << algn.formatAlignment(bamdec.getHeader()) << std::endl;
		}	
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
