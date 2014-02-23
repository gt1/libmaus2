/*
    libmaus
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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

#include <libmaus/lcs/EditDistance.hpp>
#include <libmaus/lcs/BandedEditDistance.hpp>

uint64_t dif(uint64_t const a, uint64_t const b)
{
	return static_cast<uint64_t>(std::abs(static_cast<int64_t>(a)-static_cast<int64_t>(b)));
}

int main()
{
	try
	{
		#if 0
		#if 1
		std::string const b = "lichtensteijn";
		std::string const a = "lichesxein";
		#else
		std::string const a = "schokolade lecker";
		std::string const b = "iss schokolade";
		#endif
		#endif
		
		std::string const a = "aaaaaab";
		std::string const b = "aaaaba";
		
		std::cerr << a << std::endl;
		std::cerr << b << std::endl;
		
		libmaus::lcs::EditDistance E(a.size(),b.size());
		libmaus::lcs::EditDistanceResult EDR = E.process(a.begin(),b.begin());
		std::cout << EDR << std::endl;
		
		E.printAlignmentLines(std::cout,a,b,80);

		libmaus::lcs::BandedEditDistance BE(a.size(),b.size(),dif(a.size(),b.size()));
		
		libmaus::lcs::EditDistanceResult EDRB = BE.process(a.begin(),b.begin());
		std::cout << EDRB << std::endl;

		BE.printAlignmentLines(std::cout,a,b,80);
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
	}
}
