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

static uint64_t stringToFlag(std::string const & s)
{
	if ( s == "PAIRED" )
		return ::libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FPAIRED;
	else if ( s == "PROPER_PAIR" )
		return ::libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FPROPER_PAIR;
	else if ( s == "UNMAP" )
		return ::libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FUNMAP;
	else if ( s == "MUNMAP" )
		return ::libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FMUNMAP;
	else if ( s == "REVERSE" )
		return ::libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FREVERSE;
	else if ( s == "MREVERSE" )
		return ::libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FMREVERSE;
	else if ( s == "READ1" )
		return ::libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FREAD1;
	else if ( s == "READ2" )
		return ::libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FREAD2;
	else if ( s == "SECONDARY" )
		return ::libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FSECONDARY;
	else if ( s == "QCFAIL" )
		return ::libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FQCFAIL;
	else if ( s == "DUP" )
		return ::libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FDUP;
	else
	{
		::libmaus::exception::LibMausException se;
		se.getStream() << "Unknown flag " << s << std::endl;
		se.finish();
		throw se;
	}
}

static uint64_t stringToFlags(std::string const & s)
{
	std::deque<std::string> const tokens = ::libmaus::util::stringFunctions::tokenize(s,std::string(","));
	uint64_t flags = 0;
	
	for ( uint64_t i = 0; i < tokens.size(); ++i )
		flags |= stringToFlag(tokens[i]);
		
	return flags;
}


void bamcollate(libmaus::util::ArgInfo const & arginfo)
{
	uint32_t const excludeflags = stringToFlags(arginfo.getValue<std::string>("exclude","SECONDARY,QCFAIL"));
	libmaus::bambam::CircularHashCollatingBamDecoder CHCBD(std::cin,"tmpfile",excludeflags);
	libmaus::bambam::CircularHashCollatingBamDecoder::OutputBufferEntry const * ob = 0;

	libmaus::bambam::BamHeader const & bamheader = CHCBD.bamdec.bamheader;
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

struct BamToFastqOutputFileSet
{
	std::map < std::string, libmaus::aio::CheckedOutputStream::shared_ptr_type > files;
	std::ostream & Fout;
	std::ostream & F2out;
	std::ostream & Oout;
	std::ostream & O2out;
	std::ostream & Sout;
	
	static std::map < std::string, libmaus::aio::CheckedOutputStream::shared_ptr_type > openFiles(libmaus::util::ArgInfo const & arginfo)
	{
		std::map < std::string, libmaus::aio::CheckedOutputStream::shared_ptr_type > files;
		
		char const * fileargs[] = { "F", "F2", "O", "O2", "S", 0 };
		
		for ( char const ** filearg = &fileargs[0]; *filearg; ++filearg )
		{
			std::string const fn = arginfo.getValue<std::string>(*filearg,"-");
			
			if ( fn != "-" && files.find(fn) == files.end() )
			{
				files [ fn ] =
					libmaus::aio::CheckedOutputStream::shared_ptr_type(new libmaus::aio::CheckedOutputStream(fn));
			}
		}

		return files;	
	}
	
	static std::ostream & getFile(libmaus::util::ArgInfo const & arginfo, std::string const opt, std::map < std::string, libmaus::aio::CheckedOutputStream::shared_ptr_type > & files)
	{
		std::string const fn = arginfo.getValue<std::string>(opt,"-");
		
		if ( fn == "-" )
			return std::cout;
		else
		{
			assert ( files.find(fn) != files.end() );
			return *(files.find(fn)->second);
		}
	}
	
	BamToFastqOutputFileSet(libmaus::util::ArgInfo const & arginfo)
	: files(openFiles(arginfo)), 
	  Fout( getFile(arginfo,"F",files) ),
	  F2out( getFile(arginfo,"F2",files) ),
	  Oout( getFile(arginfo,"O",files) ),
	  O2out( getFile(arginfo,"O2",files) ),
	  Sout( getFile(arginfo,"S",files) )
	{
	} 
	
	~BamToFastqOutputFileSet()
	{
		Fout.flush(); F2out.flush();
		Oout.flush(); O2out.flush();
		Sout.flush();
		
		for (
			std::map < std::string, libmaus::aio::CheckedOutputStream::shared_ptr_type >::iterator ita = files.begin();
			ita != files.end(); ++ita 
		)
		{
			ita->second->flush();
			ita->second->close();
			ita->second.reset();
		}
	}
};


void bamtofastq(libmaus::util::ArgInfo const & arginfo)
{
	uint32_t const excludeflags = stringToFlags(arginfo.getValue<std::string>("exclude","SECONDARY,QCFAIL"));
	BamToFastqOutputFileSet OFS(arginfo);
	libmaus::bambam::CircularHashCollatingBamDecoder CHCBD(std::cin,"tmpfile",excludeflags);
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
