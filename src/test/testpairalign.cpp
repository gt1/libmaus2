/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#include <libmaus2/fastx/FastAReader.hpp>
#include <libmaus2/util/ArgParser.hpp>
#include <libmaus2/lcs/NP.hpp>
#include <libmaus2/lcs/AlignmentPrint.hpp>
#include <libmaus2/dazzler/align/Overlap.hpp>
#include <libmaus2/aio/OutputStreamInstance.hpp>
#include <libmaus2/dazzler/align/AlignmentWriter.hpp>

int getOrientation(std::string sid)
{
	if ( sid.find("orientation=") == sid.npos )
		return -1;

	sid = sid.substr(sid.find("orientation=") + strlen("orientation="));

	if ( sid.find(",") == sid.npos )
		return -1;

	sid = sid.substr(0,sid.find(","));

	return atol(sid.c_str());
}

void writeStringInColumn(std::ostream & out, std::string const & genome, uint64_t const cols)
{
	uint64_t p = 0;
	while ( p < genome.size() )
	{
		uint64_t const towrite = std::min(
			static_cast<size_t>(genome.size()-p),static_cast<size_t>(cols)
		);
		out.write(genome.c_str() + p,towrite);
		out.put('\n');
		p += towrite;
	}
}

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgParser const arg(argc,argv);
		std::string const fna = arg[0];
		std::string const fnb = arg[1];

		libmaus2::fastx::FastAReader fa_R(fna);
		libmaus2::fastx::FastAReader::pattern_type pattern;
		std::vector < libmaus2::fastx::FastAReader::pattern_type > Vref;
		while ( fa_R.getNextPatternUnlocked(pattern) )
			Vref.push_back(pattern);
		assert ( Vref.size() == 1 );
		std::string genome = Vref[0].spattern;
		genome = libmaus2::fastx::remapString(libmaus2::fastx::mapString(genome));

		std::string const prolog = "fab_genome";
		std::string const readprolog = "fab_read";
		libmaus2::aio::OutputStreamInstance::unique_ptr_type refOSI(new libmaus2::aio::OutputStreamInstance("fab_genome.fasta"));
		(*refOSI) << '>' << prolog << '/' << 0 /* read id */ << '/' << 0 << '_' << genome.size() << " RQ=0.851\n";
		writeStringInColumn(*refOSI,genome,80);
		refOSI->flush();
		refOSI.reset();

		libmaus2::fastx::FastAReader fb_R(fnb);
		libmaus2::lcs::NP NP;
		uint64_t readid = 0;
		int64_t const tspace = 128;

		libmaus2::aio::OutputStreamInstance::unique_ptr_type readOSI(new libmaus2::aio::OutputStreamInstance("fab_reads.fasta"));

		libmaus2::dazzler::align::AlignmentWriter::unique_ptr_type AW(new libmaus2::dazzler::align::AlignmentWriter("fab_reads.las",tspace,false /* no index */));

		while ( fb_R.getNextPatternUnlocked(pattern) )
		{
			std::string name = pattern.getStringId();
			if ( name.find("genome[") != name.npos )
			{
				std::string srange = name.substr(name.find("genome[") + strlen("genome["));
				if ( srange.find("]") != srange.npos )
				{
					srange = srange.substr(0,srange.find("]"));
					if ( srange.find("..") != srange.npos )
					{
						std::string const sfrom = srange.substr(0,srange.find(".."));
						std::string const sto = srange.substr(srange.find("..") + strlen(".."));
						uint64_t const ufrom = atol(sfrom.c_str());
						uint64_t const uto = atol(sto.c_str());
						int const orientation = getOrientation(name);
						std::cerr << pattern.getStringId() << " " << ufrom << " " << uto << " orientation " << orientation << std::endl;
						assert ( uto >= ufrom );

						std::string refpart = genome.substr(ufrom,uto-ufrom);
						std::string read = pattern.spattern;
						bool const rc = !orientation;

						if ( rc )
							read = libmaus2::fastx::reverseComplementUnmapped(read);

						refpart = libmaus2::fastx::remapString(libmaus2::fastx::mapString(refpart));
						read = libmaus2::fastx::remapString(libmaus2::fastx::mapString(read));

						(*readOSI) << '>' << readprolog << '/' << 0 /* read id */ << '/' << 0 << '_' << read.size() << " RQ=0.851\n";
						writeStringInColumn(*readOSI,read,80);

						NP.np(
							refpart.begin(),refpart.end(),
							read.begin(),read.end()
						);

						libmaus2::dazzler::align::Overlap OVL = libmaus2::dazzler::align::Overlap::computeOverlap(
							0 /* flags */,
							0 /* a read */,
							readid,
							ufrom,
							uto,
							0,
							read.size(),
							tspace, /* tspace */
							NP.getTraceContainer()
						);

						AW->put(OVL);

						// std::cerr << NP.getTraceContainer().getAlignmentStatistics() << std::endl;

						#if 0
						libmaus2::lcs::AlignmentPrint::printAlignmentLines(std::cerr,
							refpart.begin(),refpart.size(),
							read.begin(),read.size(),
							80,
							NP.ta,NP.te);
						#endif
					}
				}
			}

			readid += 1;
		}

		AW.reset();

		readOSI->flush();
		readOSI.reset();
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
