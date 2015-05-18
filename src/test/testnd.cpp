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
#include <libmaus2/lcs/ND.hpp>
#include <libmaus2/util/ArgInfo.hpp>
#include <libmaus2/fastx/StreamFastAReader.hpp>

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgInfo const arginfo(argc,argv);
			
		// std::string a = "ACGTACGT";
		// std::string b = "ACGTTTACGZ";
		std::string const a = "GCAGGTGGAAAGCACCGCAAATCACATTTACGAAAAAGCTCTGTTAACCCCGATTTAGGTGGCGACATTCCCCTTGACATAATAAAGTCTGTACCAAGAG";
		std::string const b = "TGCAGCTGGAAGCACCGCAAAAATCAAAATTTACGAAAAAGTCGTCTGTTAACCCGATGTTAGGTGCCGGAAACTTTCCCCTTGACTAATAAAGTCTGTACAGAG";
		

		// maximum number of errors
		unsigned int d = 30;
		
		libmaus2::lcs::ND nd;
		bool const ok = nd.process(a.begin(),a.size(),b.begin(),b.size(),d);
		
		if ( ok )
		{
			// nd.printTrace(std::cout,a.begin(),b.begin());
			nd.printAlignment(std::cout,a.begin(),b.begin());
		}
		else
		{
			std::cout << "no alignment found" << std::endl;
		}
	}
	// catch(std::exception const & ex)
	catch(int)
	{
		// std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
