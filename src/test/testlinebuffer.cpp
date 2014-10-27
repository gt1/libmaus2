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
#include <libmaus/bambam/BamHeaderLowMem.hpp>

#include <libmaus/aio/PosixFdInputStream.hpp>

#include <libmaus/bambam/SamDecoder.hpp>

int main(int argc, char * argv[])
{
	try
	{
		libmaus::bambam::SamDecoder SD(std::cin);
		libmaus::bambam::BamAlignment const & algn = SD.getAlignment();
		libmaus::bambam::BamHeader const & header = SD.getHeader();		
		::libmaus::bambam::BamFormatAuxiliary aux;
		
		while ( SD.readAlignment() )
		{
			algn.formatAlignment(std::cout,header,aux);
			std::cout.put('\n');
		}
	
		#if 0
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
			#if 0
			libmaus::bambam::ScramDecoder SD("-","rs",std::string());
			#endif
		
			#if 1
			libmaus::aio::PosixFdInputStream PFIS(STDIN_FILENO);
			libmaus::util::LineBuffer LB(PFIS,64*1024);
			
			std::ostringstream headerstr;
			bool lineok = false;
			while ( (lineok=LB.getline(&a,&e)) && (e-a) && (a[0] == '@') )
			{
				headerstr.write(a,e-a);
				headerstr.put('\n');
			}
			
			if ( lineok )
				LB.putback(a);
				
			std::string const & sheader = headerstr.str();
			libmaus::bambam::BamHeader const header(sheader);
			
			std::cout << header.text;
			
			libmaus::bambam::SamInfo SI(header);
			::libmaus::bambam::BamFormatAuxiliary aux;
			uint64_t c = 0;
			while ( LB.getline(&a,&e) )
			{
				// std::cout << std::string(a,e) << "\n";
				SI.parseSamLine(a,e);
				
				#if 0
				SI.algn.formatAlignment(std::cout,header,aux);
				std::cout.put('\n');	
				#endif
				
				++c;
				
				if ( (c & (1024*1024-1)) == 0 )
					std::cerr << c << std::endl;
			}
			std::cerr << c << std::endl;
			#endif
		}
		#endif
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;	
	}
}
