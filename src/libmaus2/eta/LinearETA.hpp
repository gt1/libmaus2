/*
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
*/


#if ! defined(ETA_HPP)
#define ETA_HPP

#include <sys/types.h>
#include <cmath>
#include <string>
#include <sstream>
#include <ctime>
#include <libmaus/timing/RealTimeClock.hpp>

namespace libmaus
{
	namespace eta
	{
		struct LinearETA
		{
			::libmaus::timing::RealTimeClock rtc;
			uint64_t const n;
			
			LinearETA(uint64_t const rn)
			: rtc(), n(rn)
			{
				rtc.start();
			}
			
			void reset()
			{
				rtc.start();
			}
			
			static std::string secondsToString(double resttime)
			{
				resttime = std::floor(resttime + 0.5);

				uint64_t resthours = static_cast<uint64_t>(resttime / (60*60));
				resttime -= resthours * (60*60);
				uint64_t restmins = static_cast<uint64_t>(resttime / (60));
				resttime -= restmins * 60;
				uint64_t restsecs = resttime;

				std::ostringstream hourstr; hourstr << resthours;
				std::ostringstream minstr; if ( restmins < 10 ) minstr << "0"; minstr << restmins;	
				std::ostringstream secstr; if ( restsecs < 10 ) secstr << "0"; secstr << restsecs;	
				
				return hourstr.str() + ":" + minstr.str() + ":" + secstr.str();
			}
			
			std::string eta(uint64_t i) const
			{
				double const timeelapsed = rtc.getElapsedSeconds();
				double const timeperelement = timeelapsed / i;
				double const restsize = n-i;
				double const resttime = restsize*timeperelement;	
				return secondsToString(resttime);
			}
			
			std::string elapsed() const
			{
				return secondsToString ( rtc.getElapsedSeconds() );
			}
		};
	}
}
#endif
