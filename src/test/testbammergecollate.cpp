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
#include <libmaus/bambam/CircularHashCollatingBamDecoder.hpp>	
#include <libmaus/bambam/BamWriter.hpp>
#include <libmaus/util/TempFileRemovalContainer.hpp>
#include <libmaus/util/ArgInfo.hpp>

using namespace std;

int main(int argc, char * argv[])
{
	try
	{
		libmaus::util::ArgInfo const arginfo(argc,argv);

		/* remove temporary file at program exit */
		std::string const tmpfilename = "tmpfile";
		libmaus::util::TempFileRemovalContainer::addTempFile(tmpfilename);
		
		if ( arginfo.getPairCount("I") )
		{
			typedef libmaus::bambam::BamMergeCoordinateCircularHashCollatingBamDecoder collator_type;
			typedef collator_type::alignment_ptr_type alignment_ptr_type;

			/* set up collator object */	
			collator_type C(arginfo.getPairValues("I"),tmpfilename,0,true,20,32*1024*1024);
			libmaus::bambam::BamHeader const & bamheader = C.getHeader();
			// collator_type C(cin,8,tmpfilename);
			pair<alignment_ptr_type,alignment_ptr_type> P;
			
			libmaus::bambam::BamWriter writer(std::cout,bamheader,0);

			/* read alignments */	
			while ( C.tryPair(P) )
			{
				if ( P.first )
					P.first->serialise(writer.getStream());
				if ( P.second )
					P.second->serialise(writer.getStream());
			}
		}
		else
		{
			typedef libmaus::bambam::BamCircularHashCollatingBamDecoder collator_type;
			// typedef BamParallelCircularHashCollatingBamDecoder collator_type;
			typedef collator_type::alignment_ptr_type alignment_ptr_type;

			/* set up collator object */	
			collator_type C(cin,tmpfilename,0,true,20,32*1024*1024);
			libmaus::bambam::BamHeader const & bamheader = C.getHeader();
			// collator_type C(cin,8,tmpfilename);
			pair<alignment_ptr_type,alignment_ptr_type> P;

			libmaus::bambam::BamWriter writer(std::cout,bamheader,0);

			/* read alignments */	
			while ( C.tryPair(P) )
			{
				if ( P.first )
					P.first->serialise(writer.getStream());
				if ( P.second )
					P.second->serialise(writer.getStream());
			}
		}
		
		return EXIT_SUCCESS;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
