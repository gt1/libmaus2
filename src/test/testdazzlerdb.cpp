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
#include <libmaus2/dazzler/align/AlignmentWriter.hpp>

#include <iostream>

#include <libmaus2/dazzler/db/DatabaseFile.hpp>
#include <libmaus2/dazzler/align/AlignmentFile.hpp>
#include <libmaus2/lcs/EditDistance.hpp>
#include <libmaus2/util/ArgInfo.hpp>
#include <libmaus2/lcs/ND.hpp>
#include <libmaus2/lcs/NDextend.hpp>
#include <libmaus2/dazzler/db/OutputBase.hpp>
#include <libmaus2/lcs/LocalAlignmentPrint.hpp>

#include <libmaus2/dazzler/align/SortingOverlapOutputBuffer.hpp>

#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>

#if defined(LIBMAUS2_HAVE_DALIGNER)
#include "DB.h"
#include "align.h"
#include "QV.h"
#endif

#include <libmaus2/lcs/DalignerLocalAlignment.hpp>

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

uint8_t remapFunction(uint8_t const & a)
{
	return ::libmaus2::fastx::remapChar(a);
}

#include <libmaus2/random/Random.hpp>

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgInfo const arginfo(argc,argv);

		#if defined(LIBMAUS2_HAVE_DALIGNER)
		
		#if 1
		{
			libmaus2::lcs::DalignerLocalAlignment DLA;
			//uint8_t A[] = { 'T','T','A','A','A','G','G'};
			//uint8_t B[] = { 'G','G','A','A','A','T','T'};
			uint8_t A[] = { 3,3,0,0,0,2,2};
			uint8_t B[] = { 2,2,0,0,0,3,3};
			libmaus2::lcs::LocalEditDistanceResult LEDR = DLA.process(&A[0],sizeof(A),&B[0],sizeof(B));
			libmaus2::lcs::LocalAlignmentPrint::printAlignmentLines(
				std::cerr,
				&A[0],
				&A[0] + sizeof(A),
				&B[0],
				&B[0] + sizeof(B),
				80,
				DLA.ta,
				DLA.te,
				LEDR,
				remapFunction,
				remapFunction
			);
		}
		{
			libmaus2::lcs::DalignerLocalAlignment DLA;
			//uint8_t A[] = { 'T','T','A','A','A','G','G'};
			//uint8_t B[] = { 'G','G','A','A','A','T','T'};
			//uint8_t A[] = { 3,3,0,0,0,2,2};
			//uint8_t B[] = { 2,2,0,0,3,3};
			uint8_t A[] = { 3,3,0,0,0,0,2,2};
			uint8_t B[] = { 2,2,0,0,1,0,0,3,3};
			libmaus2::lcs::LocalEditDistanceResult LEDR = DLA.process(&A[0],sizeof(A),&B[0],sizeof(B));
			libmaus2::lcs::LocalAlignmentPrint::printAlignmentLines(
				std::cerr,
				&A[0],
				&A[0] + sizeof(A),
				&B[0],
				&B[0] + sizeof(B),
				80,
				DLA.ta,
				DLA.te,
				LEDR,
				remapFunction,
				remapFunction
			);
		}
		#endif
		
		{
			std::vector<uint8_t> vecA, vecB, vecC, vecD, vecE;
			for ( uint64_t i = 0; i < 512; ++i )
			{
				vecA.push_back(::libmaus2::random::Random::rand8()%4);
				vecB.push_back(::libmaus2::random::Random::rand8()%4);
			}
			for ( uint64_t i = 0; i < 8192; ++i )
			{
				vecC.push_back(::libmaus2::random::Random::rand8()%4);
			}
			for ( uint64_t i = 0; i < 512; ++i )
			{
				vecD.push_back(::libmaus2::random::Random::rand8()%4);
				vecE.push_back(::libmaus2::random::Random::rand8()%4);
			}
			
			std::string const sa = 
				std::string(vecA.begin(),vecA.end()) + 
				std::string(vecC.begin(),vecC.end()) + 
				std::string(vecD.begin(),vecD.end());
			std::string const sb = 
				std::string(vecB.begin(),vecB.end()) + 
				std::string(vecC.begin(),vecC.end()) + 
				std::string(vecE.begin(),vecE.end());
				
			libmaus2::autoarray::AutoArray<uint8_t> A(sa.size());
			libmaus2::autoarray::AutoArray<uint8_t> B(sb.size());
			std::copy(sa.begin(),sa.end(),A.begin());
			std::copy(sb.begin(),sb.end(),B.begin());
			
			libmaus2::lcs::DalignerLocalAlignment DLA;
			uint64_t const frontclip = vecA.size() - 128;
			libmaus2::lcs::LocalEditDistanceResult LEDR = DLA.process(A.begin()+frontclip,A.size()-frontclip,B.begin()+frontclip,B.size()-frontclip);
						
			libmaus2::lcs::LocalAlignmentPrint::printAlignmentLines(
				std::cerr,
				A.begin()+frontclip,A.end(),
				B.begin()+frontclip,B.end(),
				80,
				DLA.ta,
				DLA.te,
				LEDR,
				remapFunction,
				remapFunction
			);
			std::cerr << LEDR << std::endl;
		}
		#endif

		#if defined(LIBMAUS2_HAVE_DALIGNER)
		{
			std::string const dbfn = arginfo.restargs.at(0);
			libmaus2::autoarray::AutoArray<char> Adbfn(dbfn.size()+1);
			std::copy(dbfn.begin(),dbfn.end(),Adbfn.begin());
			Adbfn[dbfn.size()] = 0;
			
			HITS_DB db;
			int const r = Open_DB(Adbfn.begin(),&db);
			if ( r < 0 )
			{
				std::cerr << "Failed to Open_DB" << std::endl;
			}
			else
			{
				std::cerr << "database opened ok" << std::endl;

				char inqualtrackname[] = "inqual";

				int const tr = Check_Track(&db,&inqualtrackname[0]);

				std::cerr << "tr=" << tr << std::endl;

				if ( tr >= 0 )
				{
					HITS_TRACK *inqualtrack = Load_Track(&db,&inqualtrackname[0]);
					std::cerr << "track name=" << inqualtrack->name << std::endl;
					std::cerr << "size=" << inqualtrack->size << std::endl;
					std::cerr << "anno=" << inqualtrack->anno << std::endl;
					std::cerr << "data=" << inqualtrack->data << std::endl;
				}

				Close_DB(&db);
			}
		}
		#endif

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
		
		if ( PDB1 )
			std::cerr << *PDB1;
		if ( PDB2 )
			std::cerr << *PDB2;

		try
		{
			libmaus2::dazzler::db::Track::unique_ptr_type track(PDB1->readTrack("inqual"));
			std::cerr << "loaded track " << track->name << std::endl;
			libmaus2::dazzler::db::TrackAnnoInterface const & anno = track->getAnno();
			for ( uint64_t i = 0; i < PDB1->size(); ++i )
			{
				std::cerr << i << "\t" << anno[i] << "\t" << anno[i+1] << "\t" << anno[i+1]-anno[i] << "\t" << PDB1->getRead(i) << std::endl;

				#if 0
				if ( anno[i+1]-anno[i] != (PDB1->getRead(i).rlen + 99)/100 )
					std::cerr << "weird" << std::endl;
				#endif
			}
		}
		catch(std::exception const & ex)
		{
			std::cerr << ex.what();
		}
					
		libmaus2::aio::InputStream::unique_ptr_type Palgnfile(libmaus2::aio::InputStreamFactoryContainer::constructUnique(aligns));
		libmaus2::dazzler::align::AlignmentFile algn(*Palgnfile);

		libmaus2::lcs::EditDistanceTraceContainer ATC;		
		libmaus2::lcs::EditDistanceTraceContainer RATC;		
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


			// compute alignment trace
			OVL.computeTrace(reinterpret_cast<uint8_t const *>(aptr),reinterpret_cast<uint8_t const *>(bptr),algn.tspace,ATC,ND);

			{
				libmaus2::dazzler::align::Overlap ROVL =
					libmaus2::dazzler::align::Overlap::computeOverlap(
						OVL.flags,
						OVL.aread,
						OVL.bread,
						OVL.path.abpos,
						OVL.path.aepos,
						OVL.path.bbpos,
						OVL.path.bepos,
						algn.tspace,
						ATC
					);

				ROVL.computeTrace(reinterpret_cast<uint8_t const *>(aptr),reinterpret_cast<uint8_t const *>(bptr),algn.tspace,RATC,ND);
	
				if ( ! ROVL.compareMetaLower(OVL) )
				{
					std::cerr << OVL << std::endl;
					std::cerr << ROVL << std::endl;
				
					assert ( ROVL.compareMetaLower(OVL) );
				}
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
