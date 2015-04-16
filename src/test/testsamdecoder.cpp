/*
    libmaus2
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
#include <libmaus2/aio/PosixFdInputStream.hpp>
#include <libmaus2/bambam/BamBlockWriterBaseFactory.hpp>
#include <libmaus2/bambam/SamDecoder.hpp>
#include <libmaus2/util/ArgInfo.hpp>

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgInfo const arginfo(argc,argv);
		bool const verbose = arginfo.getValue<int>("verbose",0);
		libmaus2::aio::PosixFdInputStream PFIS(STDIN_FILENO);
		libmaus2::bambam::SamDecoder SD(PFIS);
		libmaus2::bambam::BamAlignment const & algn = SD.getAlignment();
		libmaus2::bambam::BamHeader const & header = SD.getHeader();		
		::libmaus2::bambam::BamFormatAuxiliary aux;

		libmaus2::bambam::BamBlockWriterBase::unique_ptr_type Pwriter(libmaus2::bambam::BamBlockWriterBaseFactory::construct(header,arginfo));
		libmaus2::bambam::BamBlockWriterBase & writer = *Pwriter;
		
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
