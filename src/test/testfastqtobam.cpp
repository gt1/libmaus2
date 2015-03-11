/*
    libmaus
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#include <libmaus/aio/PosixFdInputStream.hpp>
#include <libmaus/aio/PosixFdOutputStream.hpp>
#include <libmaus/bambam/RgInfo.hpp>
#include <libmaus/bambam/parallel/FastqToBamControl.hpp>
#include <libmaus/lz/BgzfDeflate.hpp>
#include <libmaus/parallel/NumCpus.hpp>
#include <config.h>

static std::string writeHeader(libmaus::util::ArgInfo const & arginfo, std::ostream & out)
{
	libmaus::bambam::RgInfo const rginfo(arginfo);

	std::ostringstream headerostr;
	headerostr << "@HD\tVN:1.4\tSO:unknown\n";
	headerostr 
		<< "@PG"<< "\t" 
		<< "ID:" << "fastqtobam" << "\t" 
		<< "PN:" << "fastqtobam" << "\t"
		<< "CL:" << arginfo.commandline << "\t"
		<< "VN:" << std::string(PACKAGE_VERSION)
		<< std::endl;
	headerostr << rginfo.toString();
	::libmaus::bambam::BamHeader bamheader;
	bamheader.text = headerostr.str();		

	libmaus::lz::BgzfOutputStream bgzf(out);
	bamheader.serialise(bgzf);
	bgzf.flush();
	
	return rginfo.ID;
}

static int fastqtobampar(libmaus::util::ArgInfo const & arginfo)
{
	std::ostream & out = std::cout;
	uint64_t const numlogcpus = arginfo.getValue<int>("threads",libmaus::parallel::NumCpus::getNumLogicalProcessors());
	int const level = arginfo.getValue<int>("level",Z_DEFAULT_COMPRESSION);
		
	std::string const rgid = writeHeader(arginfo,out);

	libmaus::parallel::SimpleThreadPool STP(numlogcpus);
	uint64_t const outblocks = 1024;
	uint64_t const inputblocksize = 1024*1024*64;
	uint64_t const inblocks = 64;
	libmaus::aio::PosixFdInputStream PFIS(STDIN_FILENO);
	libmaus::bambam::parallel::FastqToBamControl FTBC(PFIS,out,STP,inblocks,inputblocksize,outblocks,level,rgid);

	FTBC.enqueReadPackage();
	FTBC.waitCompressionFinished();
		
	STP.terminate();		
	STP.join();

	return EXIT_SUCCESS;
}

int main(int argc, char * argv[])
{
	try
	{
		libmaus::util::ArgInfo const arginfo(argc,argv);
		
		return fastqtobampar(arginfo);
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
