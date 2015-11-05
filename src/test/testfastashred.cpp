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
#include <libmaus2/fastx/StreamFastAReader.hpp>

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::fastx::StreamFastAReaderWrapper SFA(std::cin);
		libmaus2::fastx::StreamFastAReaderWrapper::pattern_type pattern;

		libmaus2::util::ArgInfo const arginfo(argc,argv);
		uint64_t const readlen = arginfo.getValueUnsignedNumeric<uint64_t>("readlen",32*1024);
		uint64_t const step = arginfo.getValueUnsignedNumeric<uint64_t>("step",4*1024);
		uint64_t cnt = 0;

		while ( SFA.getNextPatternUnlocked(pattern) )
		{
			std::string & spat = pattern.spattern;

			for ( uint64_t i = 0; i < spat.size(); ++i )
				spat[i] = toupper(spat[i]);

			if ( spat.size() < readlen )
			{
				std::cout <<
					"@read" << cnt++ << "\n" <<
					spat << "\n" <<
					"+\n" <<
					std::string(spat.size(),'H') << "\n";
			}
			else
			{
				uint64_t low = 0;
				while ( low < spat.size() )
				{
					uint64_t const rest = spat.size() - low;

					uint64_t const pos = (rest >= readlen) ? low : (spat.size()-readlen);

					std::cout <<
						"@read" << cnt++ << "\n" <<
						spat.substr(pos,readlen) << "\n" <<
						"+\n" <<
						std::string(readlen,'H') << "\n";

					if ( pos + readlen == spat.size() )
						low = spat.size();
					else
						low = pos+step;
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
