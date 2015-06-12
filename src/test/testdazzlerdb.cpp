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
#include <libmaus2/lcs/ND.hpp>
#include <libmaus2/lcs/NDextend.hpp>

#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>

uint64_t getColumns()
{
	// get window size
	struct winsize w;
	
	if ( isatty(STDOUT_FILENO) && ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 )
		return w.ws_col;
	else if ( getenv("COLUMNS") != 0 )
		return atoi(getenv("COLUMNS"));
	else
		return 80;
}

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgInfo const arginfo(argc,argv);

		bool const printAlignments = arginfo.getValue<int>("print",1);
		bool const loadall  = arginfo.getValue<int>("loadalla",false);
		bool loadalla = arginfo.getValue<int>("loadalla",loadall);
		bool loadallb = arginfo.getValue<int>("loadallb",loadall);
		double const eratelimit = arginfo.getValue<double>("eratelimit",1.0);
		uint64_t const cols = getColumns();
		
		libmaus2::dazzler::db::DatabaseFile::unique_ptr_type PDB1;
		libmaus2::dazzler::db::DatabaseFile::unique_ptr_type PDB2;
		libmaus2::dazzler::db::DatabaseFile * DB1 = 0;
		libmaus2::dazzler::db::DatabaseFile * DB2 = 0;
		std::string aligns;
		std::vector<libmaus2::dazzler::db::Read> VreadsMeta1;
		std::vector<libmaus2::dazzler::db::Read> VreadsMeta2;
		std::vector<libmaus2::dazzler::db::Read> const * readsMeta1 = 0;
		std::vector<libmaus2::dazzler::db::Read> const * readsMeta2 = 0;

		libmaus2::autoarray::AutoArray<char> AreadsA;
		std::vector<uint64_t> AreadsOffA;	
		libmaus2::autoarray::AutoArray<char> AreadsB;
		std::vector<uint64_t> AreadsOffB;	

		libmaus2::autoarray::AutoArray<char> const * readsA = 0;
		std::vector<uint64_t> const * readsOffA = 0;	
		libmaus2::autoarray::AutoArray<char> const * readsB = 0;
		std::vector<uint64_t> const * readsOffB = 0;	
		
		if ( arginfo.restargs.size() == 2 )
		{
			std::string const dbfn = arginfo.restargs.at(0);
			aligns = arginfo.restargs.at(1);
		
			libmaus2::dazzler::db::DatabaseFile::unique_ptr_type PDB(new libmaus2::dazzler::db::DatabaseFile(dbfn));
			PDB->computeTrimVector();
			PDB1 = UNIQUE_PTR_MOVE(PDB);	
			DB1 = PDB1.get();
			DB2 = PDB1.get();
			
			DB1->getAllReads(VreadsMeta1);
			readsMeta1 = &VreadsMeta1;
			readsMeta2 = &VreadsMeta1;
			
			if ( loadalla || loadallb )
			{
				DB1->decodeAllReads(AreadsA,AreadsOffA);
				loadalla = true;
				loadallb = true;
				
				readsA = &AreadsA;
				readsOffA = &AreadsOffA;
				readsB = &AreadsA;
				readsOffB = &AreadsOffA;
			}
		}
		else if ( arginfo.restargs.size() == 3 )
		{
			std::string const db1fn = arginfo.restargs.at(0);
			std::string const db2fn = arginfo.restargs.at(1);
			aligns = arginfo.restargs.at(2);
		
			libmaus2::dazzler::db::DatabaseFile::unique_ptr_type TPDB1(new libmaus2::dazzler::db::DatabaseFile(db1fn));
			TPDB1->computeTrimVector();
			PDB1 = UNIQUE_PTR_MOVE(TPDB1);

			libmaus2::dazzler::db::DatabaseFile::unique_ptr_type TPDB2(new libmaus2::dazzler::db::DatabaseFile(db2fn));
			TPDB2->computeTrimVector();
			PDB2 = UNIQUE_PTR_MOVE(TPDB2);
				
			DB1 = PDB1.get();
			DB2 = PDB2.get();
			
			DB1->getAllReads(VreadsMeta1);
			readsMeta1 = &VreadsMeta1;

			DB2->getAllReads(VreadsMeta2);
			readsMeta2 = &VreadsMeta2;

			if ( loadalla )
			{
				DB1->decodeAllReads(AreadsA,AreadsOffA);
				readsA = &AreadsA;
				readsOffA = &AreadsOffA;
			}
			if ( loadallb )
			{
				DB2->decodeAllReads(AreadsB,AreadsOffB);
				readsB = &AreadsB;
				readsOffB = &AreadsOffB;
			}
		}
		else
		{
			std::cerr << "usage: " << argv[0] << " <reads.db> <alignments.las> or <reads.db> <reads.db> <alignments.las>" << std::endl;
			return EXIT_FAILURE;
		}
					
		libmaus2::aio::InputStream::unique_ptr_type Palgnfile(libmaus2::aio::InputStreamFactoryContainer::constructUnique(aligns));
		libmaus2::dazzler::align::AlignmentFile algn(*Palgnfile);

		libmaus2::lcs::EditDistanceTraceContainer ATC;		
		libmaus2::lcs::NDextendDNA ND;
		libmaus2::dazzler::align::Overlap OVL;
		
		// number of alignments processed
		uint64_t z = 0;
		
		libmaus2::autoarray::AutoArray<char> Aspace;
		int64_t aid = -1;
		char const * aptr = NULL;
		
		libmaus2::autoarray::AutoArray<char> Bspace;
		libmaus2::autoarray::AutoArray<char> Binvspace;
		int64_t bid = -1;
		char const * bbaseptr = NULL;
		char const * bptr = NULL;
		bool Binvspacevalid = false;

		libmaus2::aio::InputStream::unique_ptr_type PbaseStreamA(DB1->openBaseStream());
		libmaus2::aio::InputStream::unique_ptr_type PbaseStreamB(DB2->openBaseStream());

		while ( algn.getNextOverlap(*Palgnfile,OVL) )
		{
			#if 0
			// check path
			int32_t p = OVL.path.bbpos;
			for ( size_t i = 0; i < OVL.path.path.size(); ++i )
				p += OVL.path.path[i].second;
			assert ( p == OVL.path.bepos );
			#endif
			
			if ( OVL.aread != aid )
			{
				libmaus2::dazzler::db::Read const & R = readsMeta1->at(OVL.aread);
				
				if ( loadalla )
				{
					#if 0
					if ( R.rlen > static_cast<int64_t>(Aspace.size()) )
						Aspace.resize(R.rlen);
					std::copy(readsA.begin()+readsOffA[OVL.aread],readsA.begin()+readsOffA[OVL.aread+1],Aspace.begin());
					#endif
					
					aptr = readsA->begin()+(*readsOffA)[OVL.aread];
				}
				else
				{
					PbaseStreamA->clear();
					PbaseStreamA->seekg(R.boff);
					libmaus2::dazzler::db::DatabaseFile::decodeRead(*PbaseStreamA,Aspace,R.rlen);
					aptr = Aspace.begin();
				}
				
				aid = OVL.aread;
			}

			if ( OVL.bread != bid )
			{
				libmaus2::dazzler::db::Read const & R = readsMeta2->at(OVL.bread);
				
				if ( loadallb )
				{
					#if 0
					if ( R.rlen > static_cast<int64_t>(Bspace.size()) )
						Bspace.resize(R.rlen);
					std::copy(readsA.begin()+readsOffA[OVL.bread],readsA.begin()+readsOffA[OVL.bread+1],Bspace.begin());
					#endif
					bbaseptr = readsB->begin()+(*readsOffB)[OVL.bread];
				}
				else
				{
					PbaseStreamB->clear();
					PbaseStreamB->seekg(R.boff);
					libmaus2::dazzler::db::DatabaseFile::decodeRead(*PbaseStreamB,Bspace,R.rlen);
					bbaseptr = Bspace.begin();
				}
				
				bid = OVL.bread;
				Binvspacevalid = false;						
			}
			
			if ( OVL.isInverse() )
			{
				if ( ! Binvspacevalid )
				{
					libmaus2::dazzler::db::Read const & R = readsMeta2->at(OVL.bread);
				
					if ( R.rlen > static_cast<int64_t>(Binvspace.size()) )
						Binvspace.resize(R.rlen);
				
					char const * pin =  bbaseptr;
					char * pout = Binvspace.begin() + R.rlen;
				
					for ( int64_t i = 0; i < R.rlen; ++i )
						*(--pout) = libmaus2::fastx::invertUnmapped(*(pin++));

					Binvspacevalid = true;
				}

				bptr = Binvspace.begin();
			}
			else
			{
				bptr = bbaseptr;
			}

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
				char const * asubsub_b = aptr + std::max(a_i,OVL.path.abpos);
				char const * asubsub_e = asubsub_b + a_i_1-std::max(a_i,OVL.path.abpos);
				
				// block on B
				char const * bsubsub_b = bptr + b_i;
				char const * bsubsub_e = bsubsub_b + (b_i_1-b_i);

				bool const ok = ND.process(asubsub_b,(asubsub_e-asubsub_b),bsubsub_b,bsubsub_e-bsubsub_b);
				assert ( ok );

				#if 0
				ND.printAlignmentLines(std::cout,asubsub_b,asubsub_e-asubsub_b,bsubsub_b,bsubsub_e-bsubsub_b,cols);
				#endif
				
				#if 0
				std::cerr << ED_EDR << "\t" << OVL.path.path[i].first << std::endl;
				ED.printAlignmentLines(std::cout,asubsub,bsubsub,cols);
				#endif
				
				// add trace to full alignment
				ATC.push(ND);
				
				// update start points
				b_i = b_i_1;
				a_i = a_i_1;
			}

			// print alignment if requested
			if ( printAlignments )
			{
				libmaus2::lcs::AlignmentStatistics const AS = ATC.getAlignmentStatistics();
				
				if ( AS.getErrorRate() <= eratelimit )
				{
					std::cout << OVL << std::endl;
					std::cout << AS << std::endl;
					ATC.printAlignmentLines(std::cout,aptr + OVL.path.abpos,OVL.path.aepos-OVL.path.abpos,bptr + OVL.path.bbpos,OVL.path.bepos-OVL.path.bbpos,cols);
				}
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
