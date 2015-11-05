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

#include <libmaus2/regex/PosixRegex.hpp>
#include <iostream>

void testRegex()
{
	::libmaus2::regex::PosixRegex PR("ab");
	std::vector<uint64_t> POS = PR.findAllMatches("abababab");
	for ( uint64_t i = 0; i < POS.size(); ++i )
		std::cerr << "[" << i << "]=" << POS[i] << " len " << strlen("eeeeeeee") << std::endl;

	std::string const R = PR.replaceFirstMatch("zzzabyyyy","huhc");
	std::cerr << R << std::endl;

	std::cerr << PR.replaceAllMatches("ababababcabababd","xy") << std::endl;
}

int main(/* int argc, char * argv[] */)
{
	try
	{
		testRegex();
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
	}

	return 0;
}
