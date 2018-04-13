
/*
    libmaus2
    Copyright (C) 2018 German Tischler-Höhle

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
#include <libmaus2/util/ArgParser.hpp>
#include <libmaus2/dazzler/db/DatabaseFile.hpp>
#include <libmaus2/dazzler/align/Overlap.hpp>
#include <libmaus2/lcs/NNPCorLocalAligner.hpp>
#include <libmaus2/dazzler/align/AlignmentFile.hpp>
#include <libmaus2/dazzler/align/AlignmentWriter.hpp>

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgParser const arg(argc,argv);

		if ( arg.size() < 2 )
		{
			std::cerr << "usage: " << arg.progname << " <db> <out.las>" << std::endl;
			return EXIT_FAILURE;
		}

		std::string const dbfn = arg[0];
		std::string const outfn = arg[1];

		// load database meta info
		libmaus2::dazzler::db::DatabaseFile DB(dbfn);
		// trim database
		DB.computeTrimVector();

		uint64_t const bucketlog = 6; /* log of size of diagonal buckets */
		uint64_t const anak = 12; /* size of k-mer */
		uint64_t const maxmatches = 100000; /* maximum set of kmer matches */
		uint64_t const minbandscore = 2*anak; /* score required in band (and neighboring) */
		uint64_t const minlength = 500; /* minimum length of alignment reported */

		// aligner
		libmaus2::lcs::NNPCorLocalAligner nnplo(bucketlog,anak,maxmatches,minbandscore,minlength);
		// dense trace container
		libmaus2::lcs::AlignmentTraceContainer ATC;

		// minimum tspace not small (16 bit values in LAS)
		int64_t const tspace = libmaus2::dazzler::align::AlignmentFile::getMinimumNonSmallTspace();
		libmaus2::dazzler::align::AlignmentWriter AW(outfn,tspace,false /* index */);

		// read lines from standard input
		while ( std::cin )
		{
			std::string line;
			std::getline(std::cin,line);

			// input stream for line
			std::istringstream istr(line);
			uint64_t u0;
			uint64_t u1;

			// get read ids
			istr >> u0;
			istr >> u1;

			if ( ! line.size() )
				continue;

			// check read ids
			if ( ! istr )
			{
				std::cerr << "Cannot parse " << line << std::endl;
				continue;
			}
			if ( ! (u0 < DB.size()) )
			{
				std::cerr << "Unknown read id " << u0 << std::endl;
				continue;
			}
			if ( ! (u1 < DB.size()) )
			{
				std::cerr << "Unknown read id " << u1 << std::endl;
				continue;
			}
			if ( u0 == u1 )
			{
				std::cerr << "Same read id not supported" << std::endl;
				continue;
			}

			std::cerr << "[V] trying u0=" << u0 << " u1=" << u1 << std::endl;

			// decode reads (to symbols 'A', 'C', 'G', 'T')
			std::string a = DB.decodeRead(u0,false /* rc */);
			std::string b = DB.decodeRead(u1,false /* rc */);

			// map to 0,1,2,3
			std::string ma = libmaus2::fastx::mapString(a);
			std::string mb = libmaus2::fastx::mapString(b);

			// run alignment
                        std::vector< std::pair<libmaus2::lcs::NNPAlignResult,libmaus2::lcs::NNPTraceContainer::shared_ptr_type> >
                        	Vforw = nnplo.align(
                        		ma.begin(),ma.end(),
                        		mb.begin(),mb.end()
                        	);

			for ( uint64_t i = 0; i < Vforw.size(); ++i )
			{
				// result (contains coordinates like abpos,aepos etc
				libmaus2::lcs::NNPAlignResult const & res = Vforw[i].first;
				// sparse alignment
				libmaus2::lcs::NNPTraceContainer::shared_ptr_type const & sparsealgn = Vforw[i].second;
				// dense alignment (operations +,-,I,D)
				sparsealgn->computeTrace(ATC);

				std::cerr << "\tforward " << res << std::endl;


				// compute dazzler type alignment
				libmaus2::dazzler::align::Overlap const OVL = libmaus2::dazzler::align::Overlap::computeOverlap(
					0 /* flags */,
					u0,
					u1,
					res.abpos,
					res.aepos,
					res.bbpos,
					res.bepos,
					tspace,
					ATC
                                );

                                // write alignment
                                AW.put(OVL);

				// swap roles of A and B in dense trace (i.e. replace I by D and vice versa)
                                ATC.swapRoles();

				// compute dazzler type alignment for swapped
				libmaus2::dazzler::align::Overlap const ROVL = libmaus2::dazzler::align::Overlap::computeOverlap(
					0 /* flags */,
					u1,
					u0,
					res.bbpos,
					res.bepos,
					res.abpos,
					res.aepos,
					tspace,
					ATC
                                );

                                AW.put(ROVL);
			}

			// return memory
			nnplo.returnAlignments(Vforw);
			Vforw.resize(0);

			// compute reverse complement of 0,1,2,3 repr of b read
			mb = libmaus2::fastx::reverseComplement(mb);

			// run alignment
                        std::vector< std::pair<libmaus2::lcs::NNPAlignResult,libmaus2::lcs::NNPTraceContainer::shared_ptr_type> >
                        	Vback = nnplo.align(
                        		ma.begin(),ma.end(),
                        		mb.begin(),mb.end()
                        	);

			for ( uint64_t i = 0; i < Vback.size(); ++i )
			{
				// result (contains coordinates like abpos,aepos etc
				libmaus2::lcs::NNPAlignResult const & res = Vback[i].first;
				// sparse alignment
				libmaus2::lcs::NNPTraceContainer::shared_ptr_type const & sparsealgn = Vback[i].second;
				// dense alignment (operations +,-,I,D)
				sparsealgn->computeTrace(ATC);

				std::cerr << "\treverse " << res << std::endl;

				// compute dazzler type alignment
				libmaus2::dazzler::align::Overlap const OVL = libmaus2::dazzler::align::Overlap::computeOverlap(
					libmaus2::dazzler::align::Overlap::getInverseFlag() /* flags */,
					u0,
					u1,
					res.abpos,
					res.aepos,
					res.bbpos,
					res.bepos,
					tspace,
					ATC
                                );

                                // write alignment
                                AW.put(OVL);

				// swap roles of A and B in dense trace (i.e. replace I by D and vice versa)
                                ATC.swapRoles();
                                // reverse order of operations
                                ATC.reverse();

				// compute dazzler type alignment
				libmaus2::dazzler::align::Overlap const ROVL = libmaus2::dazzler::align::Overlap::computeOverlap(
					libmaus2::dazzler::align::Overlap::getInverseFlag() /* flags */,
					u1,
					u0,
					b.size() - res.bepos,
					b.size() - res.bbpos,
					a.size() - res.aepos,
					a.size() - res.abpos,
					tspace,
					ATC
                                );

                                // write alignment
                                AW.put(ROVL);
			}

			// return memory
			nnplo.returnAlignments(Vback);
			Vback.resize(0);
		}
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
