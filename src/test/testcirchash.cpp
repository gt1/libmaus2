/**
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
**/

#include <libmaus/bambam/CircularHashCollatingBamDecoder.hpp>

#include <libmaus/bambam/BamWriter.hpp>
#include <libmaus/bambam/ProgramHeaderLineSet.hpp>

#include <config.h>

void bamcollate(libmaus::util::ArgInfo const & arginfo)
{
	uint32_t const excludeflags = libmaus::bambam::BamFlagBase::stringToFlags(arginfo.getValue<std::string>("exclude","SECONDARY,QCFAIL"));
	libmaus::bambam::CircularHashCollatingBamDecoder CHCBD(std::cin,"tmpfile",excludeflags);
	libmaus::bambam::CircularHashCollatingBamDecoder::OutputBufferEntry const * ob = 0;

	libmaus::bambam::BamHeader const & bamheader = CHCBD.bamdec.getHeader();
	std::string const headertext(bamheader.text);

	// add PG line to header
	std::string const upheadtext = ::libmaus::bambam::ProgramHeaderLineSet::addProgramLine(
		headertext,
		arginfo.progname, // ID
		arginfo.progname, // PN
		arginfo.commandline, // CL
		::libmaus::bambam::ProgramHeaderLineSet(headertext).getLastIdInChain(), // PP
		std::string(PACKAGE_VERSION) // VN			
	);
	// construct new header
	::libmaus::bambam::BamHeader uphead(upheadtext);
	// change sort order
	uphead.changeSortOrder("unknown");
	
	libmaus::bambam::BamWriter bamwr(std::cout,uphead,0);
	
	uint64_t cnt = 0;
	
	unsigned int const verbshift = 20;
	libmaus::timing::RealTimeClock rtc; rtc.start();
	
	while ( (ob = CHCBD.process()) )
	{
		uint64_t const precnt = cnt;
		
		if ( ob->fpair )
		{
			libmaus::bambam::EncoderBase::putLE< libmaus::lz::BgzfDeflate<std::ostream>,uint32_t>(bamwr.bgzfos,ob->blocksizea);
			bamwr.bgzfos.write(reinterpret_cast<char const *>(ob->Da),ob->blocksizea);
			libmaus::bambam::EncoderBase::putLE< libmaus::lz::BgzfDeflate<std::ostream>,uint32_t>(bamwr.bgzfos,ob->blocksizeb);
			bamwr.bgzfos.write(reinterpret_cast<char const *>(ob->Db),ob->blocksizeb);

			cnt += 2;
		}
		else if ( ob->fsingle || ob->forphan1 || ob->forphan2 )
		{
			libmaus::bambam::EncoderBase::putLE< libmaus::lz::BgzfDeflate<std::ostream>,uint32_t>(bamwr.bgzfos,ob->blocksizea);
			bamwr.bgzfos.write(reinterpret_cast<char const *>(ob->Da),ob->blocksizea);

			cnt += 1;
		}
		
		if ( precnt >> verbshift != cnt >> verbshift )
		{
			std::cerr << (cnt >> 20) << "\t" << static_cast<double>(cnt)/rtc.getElapsedSeconds() << std::endl;
		}
	}
	
	std::cerr << cnt << std::endl;
}

#include <libmaus/bambam/BamToFastqOutputFileSet.hpp>
#include <libmaus/util/TempFileRemovalContainer.hpp>

void bamtofastqNonCollating(libmaus::util::ArgInfo const & arginfo)
{
	libmaus::timing::RealTimeClock rtc; rtc.start();
	uint32_t const excludeflags = libmaus::bambam::BamFlagBase::stringToFlags(arginfo.getValue<std::string>("exclude","SECONDARY,QCFAIL"));
	libmaus::bambam::BamDecoder bamdec(std::cin);
	libmaus::bambam::BamAlignment const & algn = bamdec.getAlignment();
	::libmaus::autoarray::AutoArray<uint8_t> T;
	uint64_t cnt = 0;
	uint64_t bcnt = 0;
	unsigned int const verbshift = 20;
		
	while ( bamdec.readAlignment() )
	{
		uint64_t const precnt = cnt++;
		
		if ( ! (algn.getFlags() & excludeflags) )
		{
			uint64_t la = libmaus::bambam::BamAlignmentDecoderBase::putFastQ(algn.D.begin(),T);
			std::cout.write(reinterpret_cast<char const *>(T.begin()),la);
			bcnt += la;
		}

		if ( precnt >> verbshift != cnt >> verbshift )
		{
			std::cerr 
				<< (cnt >> 20) 
				<< "\t"
				<< (static_cast<double>(bcnt)/(1024.0*1024.0))/rtc.getElapsedSeconds() << "MB/s"
				<< "\t" << static_cast<double>(cnt)/rtc.getElapsedSeconds() << std::endl;
		}
	}
		
	std::cout.flush();
}

void bamtofastqCollating(libmaus::util::ArgInfo const & arginfo)
{
	uint32_t const excludeflags = libmaus::bambam::BamFlagBase::stringToFlags(arginfo.getValue<std::string>("exclude","SECONDARY,QCFAIL"));
	libmaus::bambam::BamToFastqOutputFileSet OFS(arginfo);
	libmaus::util::TempFileRemovalContainer::setup();
	std::string const tmpfilename = arginfo.getValue<std::string>("T",arginfo.getDefaultTmpFileName());
	libmaus::util::TempFileRemovalContainer::addTempFile(tmpfilename);
	libmaus::bambam::CircularHashCollatingBamDecoder CHCBD(std::cin,tmpfilename,excludeflags);
	libmaus::bambam::CircularHashCollatingBamDecoder::OutputBufferEntry const * ob = 0;
	
	// number of alignments written to files
	uint64_t cnt = 0;
	// number of bytes written to files
	uint64_t bcnt = 0;
	unsigned int const verbshift = 20;
	libmaus::timing::RealTimeClock rtc; rtc.start();
	::libmaus::autoarray::AutoArray<uint8_t> T;
	
	while ( (ob = CHCBD.process()) )
	{
		uint64_t const precnt = cnt;
		
		if ( ob->fpair )
		{
			uint64_t la = libmaus::bambam::BamAlignmentDecoderBase::putFastQ(ob->Da,T);
			OFS.Fout.write(reinterpret_cast<char const *>(T.begin()),la);
			uint64_t lb = libmaus::bambam::BamAlignmentDecoderBase::putFastQ(ob->Db,T);
			OFS.F2out.write(reinterpret_cast<char const *>(T.begin()),lb);

			cnt += 2;
			bcnt += (la+lb);
		}
		else if ( ob->fsingle )
		{
			uint64_t la = libmaus::bambam::BamAlignmentDecoderBase::putFastQ(ob->Da,T);
			OFS.Sout.write(reinterpret_cast<char const *>(T.begin()),la);

			cnt += 1;
			bcnt += (la);
		}
		else if ( ob->forphan1 )
		{
			uint64_t la = libmaus::bambam::BamAlignmentDecoderBase::putFastQ(ob->Da,T);
			OFS.Oout.write(reinterpret_cast<char const *>(T.begin()),la);

			cnt += 1;
			bcnt += (la);
		}
		else if ( ob->forphan2 )
		{
			uint64_t la = libmaus::bambam::BamAlignmentDecoderBase::putFastQ(ob->Da,T);
			OFS.O2out.write(reinterpret_cast<char const *>(T.begin()),la);

			cnt += 1;
			bcnt += (la);
		}
		
		if ( precnt >> verbshift != cnt >> verbshift )
		{
			std::cerr 
				<< (cnt >> 20) 
				<< "\t"
				<< (static_cast<double>(bcnt)/(1024.0*1024.0))/rtc.getElapsedSeconds() << "MB/s"
				<< "\t" << static_cast<double>(cnt)/rtc.getElapsedSeconds() << std::endl;
		}
	}
	
	std::cerr << cnt << std::endl;
}

void bamtofastq(libmaus::util::ArgInfo const & arginfo)
{
	if ( arginfo.getValue<uint64_t>("collate",1) )
		bamtofastqCollating(arginfo);
	else
		bamtofastqNonCollating(arginfo);
}

int main(int argc, char * argv[])
{
	try
	{
		libmaus::util::ArgInfo arginfo(argc,argv);
		bamtofastq(arginfo);
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
