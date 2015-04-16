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

#include <libmaus/fastx/FastABPGenerator.hpp>
#include <libmaus/fastx/FastaBPDecoder.hpp>
#include <libmaus/util/ArgInfo.hpp>

int main(int argc, char * argv[])
{
	try
	{
		libmaus::util::ArgInfo const arginfo(argc,argv);

		std::ostringstream ostr;
		libmaus::fastx::FastABPGenerator::fastAToFastaBP(std::cin,ostr,&std::cerr);

		std::istringstream istrpre(ostr.str());
		libmaus::fastx::FastaBPDecoder fabd(istrpre);
		fabd.checkBlockPointers(istrpre);
		fabd.printSequences(istrpre,std::cout);		
		uint64_t const totalseqlength = fabd.getTotalSequenceLength(istrpre);
		
		uint64_t const runs = 10;
		std::vector<double> dectimes;
		for ( uint64_t i = 0; i < runs; ++i )
		{
			libmaus::timing::RealTimeClock rtc;
			rtc.start();
			fabd.decodeSequencesNull(istrpre);
			dectimes.push_back(totalseqlength / rtc.getElapsedSeconds());
		}
		double const avg = std::accumulate(dectimes.begin(),dectimes.end(),0.0) / dectimes.size();
		double var = 0;
		for ( uint64_t i = 0; i < dectimes.size(); ++i )
			var += (dectimes[i]-avg)*(dectimes[i]-avg);
		var /= dectimes.size();
		double const stddev = ::std::sqrt(var);
		
		std::cerr << "[V] decoding speed " << avg << " += " << stddev << " bases/s" << std::endl;
		
		for ( uint64_t i = 0; i < fabd.getNumSeq(); ++i )
			fabd.decodeSequence(istrpre,i);
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
