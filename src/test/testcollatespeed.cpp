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
#include <libmaus2/bambam/CircularHashCollatingBamDecoder.hpp>

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgInfo const arginfo(argc,argv);
		libmaus2::timing::RealTimeClock rtc;
		uint64_t const runs = 10;
		std::pair <libmaus2::bambam::BamAlignment const *, libmaus2::bambam::BamAlignment const *> P;
		
		for ( uint64_t i = 0; i < arginfo.restargs.size(); ++i )
		{
			std::string const fn = arginfo.restargs[i];
			double srate = 0, drate = 0;
	
			for ( uint64_t j = 0; j < runs; ++j )		
			{
				rtc.start();
				libmaus2::bambam::BamDecoder bamdec(fn);
				uint64_t cnt = 0;
				while ( bamdec.readAlignment() )
					++cnt;
				double const lela = rtc.getElapsedSeconds();
				
				std::cerr << "[S] " << "cnt=" << cnt << " ela=" << lela 
					<< " rate=" << cnt/lela << std::endl;
					
				srate += cnt/lela;
			}

			for ( uint64_t j = 0; j < runs; ++j )		
			{
				rtc.start();
				libmaus2::aio::InputStreamInstance CIS(fn);
				libmaus2::bambam::BamCircularHashCollatingBamDecoder bamdec(CIS,"tmpfile");
				uint64_t cnt = 0;
				while ( bamdec.tryPair(P) )
				{
					if ( P.first )
						++cnt;
					if ( P.second )
						++cnt;
				}
				remove("tmpfile");
				double const lela = rtc.getElapsedSeconds();
				
				std::cerr << "[D] " << "cnt=" << cnt << " ela=" << lela 
					<< " rate=" << cnt/lela << std::endl;
					
				drate += cnt/lela;
			}
			
			srate /= runs;
			drate /= runs;
			std::cerr << "[Q] " << srate/drate << std::endl;
		}
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
