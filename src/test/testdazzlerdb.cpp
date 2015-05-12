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
#include <iostream>
#include <libmaus2/util/ArgInfo.hpp>
#include <libmaus2/dazzler/db/DatabaseFile.hpp>
#include <libmaus2/dazzler/align/AlignmentFile.hpp>

#include <libmaus2/lcs/GenericEditDistance.hpp>
#include <libmaus2/lcs/EditDistance.hpp>

#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgInfo const arginfo(argc,argv);
		bool const printAlignments = arginfo.getValue<int>("print",1);
		
		std::string s = arginfo.restargs.at(0);
		
		libmaus2::dazzler::db::DatabaseFile HDF(s);		
		
		std::vector<libmaus2::dazzler::db::Read> allReads;
		
		#if 0		
		for ( uint64_t i = 0; i < HDF.size(); ++i )
			std::cout << HDF[i] << std::endl;
		#endif
		
		// compute the trim vector
		HDF.computeTrimVector();

		HDF.getAllReads(allReads);
		
		std::string aligns = arginfo.restargs.at(1);
		libmaus2::aio::InputStream::unique_ptr_type Palgnfile(libmaus2::aio::InputStreamFactoryContainer::constructUnique(aligns));
		libmaus2::dazzler::align::AlignmentFile algn(*Palgnfile);

		struct winsize w;
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
		uint64_t const cols = isatty(STDOUT_FILENO) ? w.ws_col : 80;
        
		libmaus2::lcs::EditDistance<> ED;
		libmaus2::lcs::EditDistanceTraceContainer ATC;
		
		libmaus2::dazzler::align::Overlap OVL;
		uint64_t z = 0;
		
		while ( algn.getNextOverlap(*Palgnfile,OVL) )
		{
			// check path
			int32_t p = OVL.path.bbpos;
			for ( size_t i = 0; i < OVL.path.path.size(); ++i )
				p += OVL.path.path[i].second;
			assert ( p == OVL.path.bepos );
			
			std::string const aread = HDF[OVL.aread];
			std::string const bread = OVL.isInverse() ? libmaus2::fastx::reverseComplementUnmapped(HDF[OVL.bread]) : HDF[OVL.bread];
			
			std::string const asub = aread.substr(OVL.path.abpos,OVL.path.aepos-OVL.path.abpos);
			std::string const bsub = bread.substr(OVL.path.bbpos,OVL.path.bepos-OVL.path.bbpos);

		
			#if 0
			libmaus2::lcs::EditDistance<> ED;
			libmaus2::lcs::EditDistanceResult ED_EDR = ED.process(
				asub.begin(),
				asub.size(),
				bsub.begin(),
				bsub.size()
				/* , k  */
			);
			std::cerr << ED_EDR << std::endl;
			ED.printAlignmentLines(std::cout,asub,bsub,cols);
			#endif
			
			int32_t a_i = ( OVL.path.abpos / algn.tspace ) * algn.tspace;
			int32_t b_i = ( OVL.path.bbpos );
			
			ATC.reset();
			
			for ( size_t i = 0; i < OVL.path.path.size(); ++i )
			{
				int32_t const a_i_1 = std::min ( static_cast<int32_t>(a_i + algn.tspace), static_cast<int32_t>(OVL.path.aepos) );
				int32_t const b_i_1 = b_i + OVL.path.path[i].second;
								
				std::string const asubsub = aread.substr(std::max(a_i,OVL.path.abpos),a_i_1-std::max(a_i,OVL.path.abpos));
				std::string const bsubsub = bread.substr(b_i,b_i_1-b_i);

				#if 0
				libmaus2::lcs::EditDistanceResult ED_EDR = 
				#endif
					ED.process(asubsub.begin(),asubsub.size(),bsubsub.begin(),bsubsub.size()/* , k  */);
				#if 0
				std::cerr << ED_EDR << "\t" << OVL.path.path[i].first << std::endl;
				ED.printAlignmentLines(std::cout,asubsub,bsubsub,cols);
				#endif
				
				ATC.push(ED);
				
				b_i = b_i_1;
				a_i = a_i_1;
			}

			if ( printAlignments )
			{
				std::cout << OVL << std::endl;
				ATC.printAlignmentLines(std::cout,asub,bsub,cols);
			}
			
			z += 1;
			
			if ( z % 1024 == 0 )
				std::cerr.put('.');
		}
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
