/*
    libmaus2
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

#include <libmaus2/bambam/BamDecoder.hpp>
#include <libmaus2/bambam/BamWriter.hpp>
#include <ctime>
#include <cstdlib>

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgInfo const arginfo(argc,argv);
		int const level = arginfo.getValue<int>("level",-1);
		bool const randomer = arginfo.getValue<int>("random",1);
		srand(time(0));
		
		libmaus2::bambam::BamDecoder bamdec(std::cin);
		libmaus2::bambam::BamAlignment & algn = bamdec.getAlignment();		
		libmaus2::bambam::BamWriter bamwr(std::cout,bamdec.getHeader(),level);
		libmaus2::bambam::BamWriter::stream_type & wrstream = bamwr.getStream();
		libmaus2::bambam::BamAuxFilterVector MQfilter;
		MQfilter.set('M','Q');
		
		while ( bamdec.readAlignment() )
		{
			if ( randomer )
			{
				if ( (rand() & 1) == 1 )
					algn.putNextRefId(-1);
				if ( (rand() & 1) == 1 )
					algn.putNextPos(-1);
				if ( (rand() & 1) == 1 )
					algn.putTlen(0);			
				if ( (rand() & 1) == 1 )
					algn.putFlags( algn.getFlags() & (~(libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FMUNMAP)) );
				if ( (rand() & 1) == 1 )
					algn.putFlags( algn.getFlags() & (~(libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FMREVERSE)) );
				if ( (rand() & 1) == 1 )
					algn.filterOutAux(MQfilter);
				algn.serialise(wrstream);			
			}
			else
			{
				algn.putNextRefId(-1);
				algn.putNextPos(-1);
				algn.putTlen(0);			
				algn.putFlags( algn.getFlags() & (~(libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FMUNMAP)) );
				algn.putFlags( algn.getFlags() & (~(libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FMREVERSE)) );
				algn.filterOutAux(MQfilter);
				algn.serialise(wrstream);
			}
		}	
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
