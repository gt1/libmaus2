/**
    libmaus
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
**/

#include <libmaus/lcs/LCS.hpp>
#include <libmaus/util/ArgInfo.hpp>
#include <libmaus/util/GetFileSize.hpp>
#include <libmaus/fastx/FastAReader.hpp>
#include <libmaus/lcs/GenericAlignmentPrint.hpp>
#include <libmaus/lcs/GenericEditDistance.hpp>

int main(int argc, char * argv[])
{
	try
	{
		::libmaus::util::ArgInfo arginfo(argc,argv);
		
		std::string const fna = arginfo.getRestArg<std::string>(0);
		std::string const fnb = arginfo.getRestArg<std::string>(1);
		
		typedef ::libmaus::fastx::FastAReader reader_type;
		typedef reader_type::pattern_type pattern_type;
		
		reader_type ra(fna);
		reader_type rb(fnb);
		
		pattern_type pa;
		pattern_type pb;
		
		while (
			ra.getNextPatternUnlocked(pa)
			&&
			rb.getNextPatternUnlocked(pb)
		)
		{
			pa.computeMapped();
			pb.computeMapped();
			
			uint64_t na = pa.getPatternLength();
			uint64_t nb = pb.getPatternLength();			
			uint64_t const dif = (na>=nb) ? (na-nb) : (nb-na);

			::libmaus::lcs::BandedEditDistance BED(na,nb,2*dif);
			::libmaus::lcs::EditDistanceResult EDR = BED.process(pa.pattern,pb.pattern,na,nb);

			BED.printAlignmentLines(std::cout,pa.spattern,pb.spattern,80);
		}
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;	
	}
}
