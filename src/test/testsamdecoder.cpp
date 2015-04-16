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
// #include <iostream>
#include <libmaus/aio/PosixFdInputStream.hpp>
#include <libmaus/bambam/BamBlockWriterBaseFactory.hpp>
#include <libmaus/bambam/SamDecoder.hpp>
#include <libmaus/util/ArgInfo.hpp>

int main(int argc, char * argv[])
{
	try
	{
		libmaus::util::ArgInfo const arginfo(argc,argv);
		bool const verbose = arginfo.getValue<int>("verbose",0);
		libmaus::aio::PosixFdInputStream PFIS(STDIN_FILENO);
		libmaus::bambam::SamDecoder SD(PFIS);
		libmaus::bambam::BamAlignment const & algn = SD.getAlignment();
		libmaus::bambam::BamHeader const & header = SD.getHeader();		
		::libmaus::bambam::BamFormatAuxiliary aux;

		libmaus::bambam::BamBlockWriterBase::unique_ptr_type Pwriter(libmaus::bambam::BamBlockWriterBaseFactory::construct(header,arginfo));
		libmaus::bambam::BamBlockWriterBase & writer = *Pwriter;
		
		if ( verbose )
			std::cerr << header.text;
		
		while ( SD.readAlignment() )
		{
			algn.checkAlignment(header);
			writer.writeAlignment(algn);

			if ( verbose )
			{
				algn.formatAlignment(std::cerr,header,aux);
				std::cerr.put('\n');
			}
		}	
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;	
	}
}
