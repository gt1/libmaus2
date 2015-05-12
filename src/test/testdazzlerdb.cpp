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

#include <libmaus2/dazzler/db/DatabaseFile.hpp>
#include <libmaus2/dazzler/align/AlignmentFile.hpp>
#include <libmaus2/lcs/EditDistance.hpp>
#include <libmaus2/util/ArgInfo.hpp>

#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgInfo const arginfo(argc,argv);
		
		if ( arginfo.restargs.size() < 2 )
		{
			std::cerr << "usage: " << argv[0] << " <reads.db> <alignments.las>" << std::endl;
			return EXIT_FAILURE;
		}
		
		bool const printAlignments = arginfo.getValue<int>("print",1);
		
		std::string const dbfn = arginfo.restargs.at(0);
		std::string const aligns = arginfo.restargs.at(1);
		
		libmaus2::dazzler::db::DatabaseFile DB(dbfn);
		
		// compute the trim vector
		DB.computeTrimVector();

		#if 0
		std::vector<libmaus2::dazzler::db::Read> allReads;
		DB.getAllReads(allReads);
		#endif
		
		libmaus2::aio::InputStream::unique_ptr_type Palgnfile(libmaus2::aio::InputStreamFactoryContainer::constructUnique(aligns));
		libmaus2::dazzler::align::AlignmentFile algn(*Palgnfile);

		// get window size
		struct winsize w;
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
		uint64_t const cols = isatty(STDOUT_FILENO) ? w.ws_col : 80;

		libmaus2::lcs::EditDistance<> ED;
		libmaus2::lcs::EditDistanceTraceContainer ATC;		
		libmaus2::dazzler::align::Overlap OVL;
		
		// number of alignments processed
		uint64_t z = 0;
		
		while ( algn.getNextOverlap(*Palgnfile,OVL) )
		{
			#if 0
			// check path
			int32_t p = OVL.path.bbpos;
			for ( size_t i = 0; i < OVL.path.path.size(); ++i )
				p += OVL.path.path[i].second;
			assert ( p == OVL.path.bepos );
			#endif
			
			// read A
			std::string const aread = DB[OVL.aread];
			// read B (or reverse complement)
			std::string const bread = OVL.isInverse() ? libmaus2::fastx::reverseComplementUnmapped(DB[OVL.bread]) : DB[OVL.bread];

			// part of A used for alignment			
			std::string const asub = aread.substr(OVL.path.abpos,OVL.path.aepos-OVL.path.abpos);
			// part of B used for alignment
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
			
			// current point on A
			int32_t a_i = ( OVL.path.abpos / algn.tspace ) * algn.tspace;
			// current point on B
			int32_t b_i = ( OVL.path.bbpos );
			
			// reset trace container
			ATC.reset();
			
			for ( size_t i = 0; i < OVL.path.path.size(); ++i )
			{
				// block end point on A
				int32_t const a_i_1 = std::min ( static_cast<int32_t>(a_i + algn.tspace), static_cast<int32_t>(OVL.path.aepos) );
				// block end point on B
				int32_t const b_i_1 = b_i + OVL.path.path[i].second;

				// block on A		
				std::string const asubsub = aread.substr(std::max(a_i,OVL.path.abpos),a_i_1-std::max(a_i,OVL.path.abpos));
				// block on B
				std::string const bsubsub = bread.substr(b_i,b_i_1-b_i);

				#if 0
				libmaus2::lcs::EditDistanceResult ED_EDR = 
				#endif
					// compute (global) alignment of blocks
					ED.process(asubsub.begin(),asubsub.size(),bsubsub.begin(),bsubsub.size()/* , k  */);
				#if 0
				std::cerr << ED_EDR << "\t" << OVL.path.path[i].first << std::endl;
				ED.printAlignmentLines(std::cout,asubsub,bsubsub,cols);
				#endif
				
				// add trace to full alignment
				ATC.push(ED);
				
				// update start points
				b_i = b_i_1;
				a_i = a_i_1;
			}

			// print alignment if requested
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
