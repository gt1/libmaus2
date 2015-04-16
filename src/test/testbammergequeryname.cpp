/*
    libmaus2
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
#include <libmaus2/util/ArgInfo.hpp>
#include <libmaus2/bambam/BamMergeQueryName.hpp>
#include <libmaus2/bambam/BamWriter.hpp>

#include "config.h"

::libmaus2::bambam::BamHeader::unique_ptr_type updateHeader(
	::libmaus2::util::ArgInfo const & arginfo,
	::libmaus2::bambam::BamHeader const & header
)
{
	std::string const headertext(header.text);

	// add PG line to header
	std::string const upheadtext = ::libmaus2::bambam::ProgramHeaderLineSet::addProgramLine(
		headertext,
		"testbammergequeryname", // ID
		"testbammergequeryname", // PN
		arginfo.commandline, // CL
		::libmaus2::bambam::ProgramHeaderLineSet(headertext).getLastIdInChain(), // PP
		std::string(PACKAGE_VERSION) // VN			
	);
	// construct new header
	::libmaus2::bambam::BamHeader::unique_ptr_type uphead(new ::libmaus2::bambam::BamHeader(upheadtext));
	
	return UNIQUE_PTR_MOVE(uphead);
}

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgInfo const arginfo(argc,argv);

		std::vector<std::string> const & inputfilenames = arginfo.restargs;
		libmaus2::bambam::BamMergeQueryName bamdec(inputfilenames /* ,true */);
		libmaus2::bambam::BamAlignment const & algn = bamdec.getAlignment();
		libmaus2::bambam::BamHeader const & header = bamdec.getHeader();
		::libmaus2::bambam::BamHeader::unique_ptr_type uphead(updateHeader(arginfo,header));
		libmaus2::bambam::BamWriter writer(std::cout,*uphead);
		libmaus2::bambam::BamWriter::stream_type & bamoutstr = writer.getStream();
		while ( bamdec.readAlignment() )
			algn.serialise(bamoutstr);
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
