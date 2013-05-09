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


#if ! defined(LIBMAUS_TIMING_REALTIMECLOCK_HPP)
#define LIBMAUS_TIMING_REALTIMECLOCK_HPP

#include <libmaus/LibMausConfig.hpp>
#include <libmaus/types/types.hpp>
#include <sstream>
#include <iomanip>

#if defined(LIBMAUS_HAVE_WINDOWS_H)
#include <windows.h>
#endif

#if defined(LIBMAUS_HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif

namespace libmaus
{
	namespace timing
	{
		struct RealTimeClockBase
		{
			static std::string formatTime(double dsecs)
			{
				uint64_t days = 0;
				uint64_t hours = 0;
				uint64_t minutes = 0;
				uint64_t secs = 0;
				bool printdays = false;
				bool printhours = false;
				bool printminutes = false;
				bool printsecs = false;
				
				if ( static_cast<uint64_t>(dsecs) >= 60*60*24 )
				{
					days = static_cast<uint64_t>(dsecs)/(60*60*24);
					dsecs -= days*60ull*60ull*24ull;
					printdays = true;
					printhours = true;
					printminutes = true;
					printsecs = true;
				}
				if ( static_cast<uint64_t>(dsecs) >= 60*60 )
				{
					hours = static_cast<uint64_t>(dsecs)/(60*60);
					dsecs -= hours*60ull*60ull;
					printhours = true;
					printminutes = true;
					printsecs = true;
				}
				if ( static_cast<uint64_t>(dsecs) >= 60 )
				{
					minutes = static_cast<uint64_t>(dsecs)/(60);
					dsecs -= minutes*60ull;
					printminutes = true;
					printsecs = true;
				}
				if ( static_cast<uint64_t>(dsecs) >= 1 )
				{
					secs = static_cast<uint64_t>(dsecs);
					dsecs -= secs;
					printsecs = true;
				}
				std::ostringstream timestr;
				timestr << std::setfill('0');
				if ( printdays )
					timestr << days << ":";
				if ( printhours )
					timestr << std::setw(2) << hours << ":";
				if ( printminutes )
					timestr << std::setw(2) << minutes << ":";
				if ( printsecs )
					timestr << std::setw(2) << secs << ":";
				
				unsigned int ddigs = 0;
				unsigned int const maxddigs = 8;
				timestr << std::setw(0);
				while ( dsecs && (ddigs++ < maxddigs) )
				{
					dsecs *= 10;
					unsigned int dig = static_cast<unsigned int>(dsecs);
					timestr << dig;
					dsecs -= dig;
				}

				return timestr.str();
			}
		};

#if defined(__linux) || defined(__FreeBSD__) || defined(__APPLE__)
		typedef uint64_t rtc_u_int64_t;

		struct RealTimeClock : public RealTimeClockBase
		{
			private:
			struct timeval started;
			mutable struct timezone tz;
      
			public:
			RealTimeClock() 
			{
				tz.tz_minuteswest = 0;
				tz.tz_dsttime = 0;
			}
			~RealTimeClock() throw() {}
      
			bool start() throw() 
			{
				return gettimeofday(&started,&tz) == 0;
			}
			
			static rtc_u_int64_t getTime()
			{
				struct timeval started;
				struct timezone tz;
				gettimeofday(&started,&tz);

				return static_cast<rtc_u_int64_t>(started.tv_usec) + (
						static_cast<rtc_u_int64_t>(started.tv_sec) * 
						static_cast<rtc_u_int64_t>(1000000ul)
				);
			}
      
			//! elapsed time in u-secs
			rtc_u_int64_t getElapsed() const 
			{
				struct timeval stopped;
				gettimeofday(&stopped,&tz);
				struct timeval dif;
				timersub(&stopped,&started,&dif);
				return 
					static_cast<rtc_u_int64_t>(dif.tv_usec) + (
						static_cast<rtc_u_int64_t>(dif.tv_sec) * 
						static_cast<rtc_u_int64_t>(1000000ul)
					);
			}
			double getElapsedSeconds() const 
			{
				rtc_u_int64_t const t = getElapsed();
				rtc_u_int64_t const s = t / 1000000;
				rtc_u_int64_t const u = t % 1000000;
				double const ds = static_cast<double>(s);
				double const du = static_cast<double>(u)/1000000.0;
				return (ds+du);
			}
		};
#else
		typedef unsigned __int64 rtc_u_int64_t;

		struct RealTimeClock : public RealTimeClockBase
		{
			private:
			LARGE_INTEGER freq;
			LARGE_INTEGER started;  

			static __int64 ltd(LARGE_INTEGER const & l) 
			{
				__int64 result = l.HighPart;
				result <<= 32;
				result |= l.LowPart;
				return result;
			} 
      
			public:
			RealTimeClock() 
			{
				QueryPerformanceFrequency(&freq);
			}
			~RealTimeClock() throw() {}
      
			bool start() throw()
			{
				if ( ltd(freq) == 0 )
					return false;
				else
				{
					return QueryPerformanceCounter(&started) > 0;
				}
			}
      
			rtc_u_int64_t getElapsed() const
			{
				if ( ! ltd(freq) )
					return 0;
				else
				{
					LARGE_INTEGER now;
					QueryPerformanceCounter(&now);
					return 
						( static_cast<rtc_u_int64_t>(ltd(now)-ltd(started)) *
							static_cast<rtc_u_int64_t>(1000000ul) )
							/ static_cast<rtc_u_int64_t>(ltd(freq));
				}
			}

			double getElapsedSeconds() const 
			{
				rtc_u_int64_t const t = getElapsed();
				rtc_u_int64_t const s = t / 1000000;
				rtc_u_int64_t const u = t % 1000000;
				double ds = static_cast<double>(s);
				double du = static_cast<double>(u)/1000000.0;
				return (ds+du);
			}
		};
#endif
	}
}
#endif
