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

#include <libmaus2/bambam/CigarStringParser.hpp>
#include <cassert>
#include <sstream>

std::vector< libmaus2::bambam::cigar_operation > libmaus2::bambam::CigarStringParser::parseCigarString(std::string cigar)
{
	std::vector<cigar_operation> ops;
	
	while ( cigar.size() )
	{
		assert ( isdigit(cigar[0]) );
		
		uint64_t numlen = 0;
		while ( numlen < cigar.size() && isdigit(cigar[numlen]) )
			numlen++;
			
		std::string const nums = cigar.substr(0,numlen);
		std::istringstream istr(nums);
		uint64_t num = 0;
		istr >> num;
		
		cigar = cigar.substr(numlen);
		
		uint32_t op = 0;
	
		assert ( cigar.size() );
				
		switch ( cigar[0] )
		{
			case 'M':
				op = 0;
				break;
			case 'I':
				op = 1;
				break;
			case 'D':
				op = 2;
				break;
			case 'N':
				op = 3;
				break;
			case 'S':
				op = 4;
				break;
			case 'H':
				op = 5;
				break;
			case 'P':
				op = 6;
				break;
			case '=':
				op = 7;
				break;
			case 'X':
				op = 8;
				break;
			default:
				op = 9;
				break;
		}
		
		cigar = cigar.substr(1);
		
		ops.push_back(cigar_operation(op,num));
	}
	
	return ops;
}
