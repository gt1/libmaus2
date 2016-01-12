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
#include <libmaus2/util/ArgParser.hpp>
#include <libmaus2/fastx/StreamFastAReader.hpp>

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::fastx::StreamFastAReaderWrapper SFA(std::cin);
		libmaus2::fastx::StreamFastAReaderWrapper::pattern_type pattern;

		libmaus2::util::ArgParser const arg(argc,argv);
		uint64_t cnt = 0;
		std::string const prefix = arg.uniqueArgPresent("p") ? arg["p"] : "fasta_";
		bool const singleline = arg.argPresent("s");
		bool const dataonly = arg.argPresent("d");

		while ( SFA.getNextPatternUnlocked(pattern) )
		{
			std::ostringstream fnostr;
			fnostr << prefix << std::setw(6) << std::setfill('0') << cnt++ << std::setw(0) << (dataonly ? "" : ".fasta");
			std::string const fn = fnostr.str();
			libmaus2::aio::OutputStreamInstance OSI(fn);

			std::string & spat = pattern.spattern;

			for ( uint64_t i = 0; i < spat.size(); ++i )
				spat[i] = toupper(spat[i]);

			pattern.sid = pattern.getShortStringId();

			if ( dataonly )
				OSI.write(pattern.spattern.c_str(),pattern.spattern.size());
			else if ( singleline )
				OSI << pattern;
			else
				pattern.printMultiLine(OSI,80);

			std::cout << fn << "\t" << pattern.getStringId() << std::endl;
		}
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
