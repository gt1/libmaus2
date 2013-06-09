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
#include <libmaus/bambam/BamWriter.hpp>
#include <libmaus/bambam/BamFlagBase.hpp>

int main()
{
	try
	{
		::libmaus::bambam::BamHeader header;
		header.addChromosome("chr1",2000);
		
		// ::libmaus::bambam::BamWriter bamwriter(std::cout,header);
		::libmaus::bambam::BamParallelWriter bamwriter(std::cout,8,header);
		
		for ( uint64_t i = 0; i < 64*1024; ++i )
		{
			std::ostringstream rnstr;
			rnstr << "r" << "_" << std::setw(6) << std::setfill('0') << i;
			std::string const rn = rnstr.str();
			
			bamwriter.encodeAlignment(rn,0,1000,30, ::libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FPAIRED | ::libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FREAD1, "9M", 0,1500, -1, "ACGTATGCA", "HGHGHGHGH");
			bamwriter.putAuxString("XX","HelloWorld");
			bamwriter.commit();
			
			bamwriter.encodeAlignment(rn,0,1500, 20,::libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FPAIRED | ::libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FREAD2,"4=1X4=",0,1000,-1,"TTTTATTTT","HHHHAHHHH");
			bamwriter.putAuxNumber("XY",'i',42);
			bamwriter.putAuxNumber("XZ",'f',3.141592);
			bamwriter.commit();				
		}
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
