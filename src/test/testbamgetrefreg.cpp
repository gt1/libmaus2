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
#include <libmaus2/bambam/BamDecoder.hpp>
#include <libmaus2/fastx/FastAReader.hpp>

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgInfo const arginfo(argc,argv);

		libmaus2::bambam::BamDecoder dec(std::cin);

		std::string const reffn = arginfo.getUnparsedValue("reference",std::string());
		assert ( reffn.size() );

		libmaus2::fastx::FastAReader FA(reffn);
		libmaus2::fastx::FastAReader::pattern_type pattern;
		std::vector<std::string> Vref;
		while ( FA.getNextPatternUnlocked(pattern) )
			Vref.push_back(pattern.spattern);

		libmaus2::bambam::BamAlignment & algn = dec.getAlignment();
		bool gok = true;
		while ( dec.readAlignment() )
		{
			try
			{
				// check the alignment
				bool const algnok = algn.checkCigar(Vref.at(algn.getRefID()).begin());
				assert ( algnok );
				std::string const cref = Vref[algn.getRefID()].substr(algn.getPos()-algn.getFrontDel(),algn.getReferenceLength());
				std::string const sref = algn.getReferenceRegionViaMd();

				if ( sref == cref )
				{
					std::cerr << "[V] " << algn.getName() << " OK" << std::endl;
				}
				else
				{
					std::cerr << "[V] " << algn.getName() << " failed" << std::endl;
					gok = false;
				}
			}
			catch(std::exception const & ex)
			{
				std::cerr << "[V] " << algn.getName() << " failed processing: " << ex.what() << std::endl;
				gok = false;
			}
		}

		if ( gok )
		{
			std::cerr << "[V] all OK" << std::endl;
			return EXIT_SUCCESS;
		}
		else
		{
			std::cerr << "[V] at least one failure" << std::endl;
			return EXIT_FAILURE;
		}
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
