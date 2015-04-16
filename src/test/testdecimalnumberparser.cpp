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
#include <iostream>
#include <libmaus/random/Random.hpp>
#include <libmaus/math/DecimalNumberParser.hpp>
#include <libmaus/timing/RealTimeClock.hpp>

int main(int argc, char * argv[])
{
	try
	{
		{		
			libmaus::math::DecimalNumberParser NP;

			srand(time(0));
			
			std::vector<std::string> SV;
			std::vector<uint64_t> NV;
			for ( uint64_t i = 0; i < 1u << 24; ++i )
			{
				uint64_t j = libmaus::random::Random::rand64();
				std::ostringstream ostr;
				ostr << j;
				SV.push_back(ostr.str());
				NV.push_back(j);
			}
			
			libmaus::timing::RealTimeClock rtc;
			rtc.start();
			for ( uint64_t i = 0; i < NV.size(); ++i )
			{
				std::string const & s = SV[i];
				char const * a = s.c_str();
				
				uint64_t const vv = NP.parseUnsignedNumber<uint64_t>(a,a+s.size());
				
				assert ( vv == NV[i] );
			}
			std::cerr << rtc.getElapsedSeconds() << std::endl;

			rtc.start();
			for ( uint64_t i = 0; i < NV.size(); ++i )
			{
				std::string const & s = SV[i];
				std::istringstream istr(s);
				
				uint64_t vv;
				istr >> vv;
				
				assert ( vv == NV[i] );
			}
			std::cerr << rtc.getElapsedSeconds() << std::endl;

			rtc.start();
			for ( uint64_t i = 0; i < NV.size(); ++i )
			{
				std::string const & s = SV[i];
				char const * a = s.c_str();
				char * ep;
				
				uint64_t const vv = strtoull(a,&ep,10);
				
				assert ( vv == NV[i] );
			}
			std::cerr << rtc.getElapsedSeconds() << std::endl;
			
			return 0;

			#if 0		
			for ( int64_t i = std::numeric_limits<int16_t>::min(); i <= std::numeric_limits<int16_t>::max(); ++i )
			{
				std::ostringstream ostr;
				ostr << i;
				std::string const s = ostr.str();
				char const * a = s.c_str();
				
				std::cerr << s << std::endl;
			
				std::cerr << static_cast<int>(NP.parseSignedNumber<int16_t>(a,a+s.size())) << std::endl;
				
				assert ( NP.parseSignedNumber<int16_t>(a,a+s.size()) == i );
			}
			#endif

			for ( uint64_t i = std::numeric_limits<uint16_t>::min(); i <= std::numeric_limits<uint16_t>::max(); ++i )
			{
				std::ostringstream ostr;
				ostr << i;
				std::string const s = ostr.str();
				char const * a = s.c_str();
				
				std::cerr << s << std::endl;
			
				std::cerr << static_cast<uint>(NP.parseUnsignedNumber<uint16_t>(a,a+s.size())) << std::endl;
				
				assert ( NP.parseUnsignedNumber<uint16_t>(a,a+s.size()) == i );
			}
			
			#if 0
			{
				int64_t i = std::numeric_limits<int16_t>::min();
				i -= 1;

				std::ostringstream ostr;
				ostr << i;
				std::string const s = ostr.str();
				char const * a = s.c_str();
			
				std::cerr << static_cast<int>(NP.parseSignedNumber<int16_t>(a,a+s.size())) << std::endl;
			}
			#endif
			#if 0
			{
				int64_t i = std::numeric_limits<int16_t>::max();
				i += 1;

				std::ostringstream ostr;
				ostr << i;
				std::string const s = ostr.str();
				char const * a = s.c_str();
			
				std::cerr << static_cast<int>(NP.parseSignedNumber<int16_t>(a,a+s.size())) << std::endl;
			}
			#endif
			#if 1
			{
				uint64_t i = std::numeric_limits<uint16_t>::max();
				i += 10;

				std::ostringstream ostr;
				ostr << i;
				std::string const s = ostr.str();
				char const * a = s.c_str();
			
				std::cerr << static_cast<int>(NP.parseUnsignedNumber<uint16_t>(a,a+s.size())) << std::endl;
			}
			#endif
		}
		
		return 0;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;	
	}
}
