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
#include <libmaus2/fastx/StreamFastAReader.hpp>
#include <libmaus2/aio/OutputStreamInstance.hpp>
#include <libmaus2/util/ArgInfo.hpp>

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgInfo const arginfo(argc,argv);
		bool const explode = arginfo.getValue<int>("explode",false);
		std::string const expprefix = arginfo.getUnparsedValue("explodeprefix",std::string());
		uint64_t const minlen = arginfo.getValueUnsignedNumeric<uint64_t>("minlen",0);

		libmaus2::fastx::StreamFastAReaderWrapper SFAR(std::cin);
		libmaus2::fastx::StreamFastAReaderWrapper::pattern_type pattern;

		uint64_t expid = 0;

		while ( SFAR.getNextPatternUnlocked(pattern) )
		{
			if ( pattern.spattern.size() >= minlen )
			{
				if ( explode )
				{
					std::ostringstream fnostr;
					fnostr << expprefix << "_" << std::setw(6) << std::setfill('0')	<< expid++ << std::setw(0) << ".fasta";
					libmaus2::aio::OutputStreamInstance OSI(fnostr.str());
					OSI << pattern;
					OSI.flush();
				}
				else
				{
					std::cout << pattern;
				}
			}
		}
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
