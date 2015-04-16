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
#if ! defined(LIBMAUS_LRU_HPP)
#define LIBMAUS_LRU_HPP

#include <vector>
#include <map>

#include <libmaus/autoarray/AutoArray.hpp>

namespace libmaus
{
	namespace lru
	{
		struct LRU
		{
			private:
			uint64_t const n;
			uint64_t curtime;
			uint64_t fill;
			std::vector < uint64_t > objectToTime;
			std::map < uint64_t, uint64_t > timeToObject;

			std::pair<uint64_t,uint64_t> getAndEraseOldest()
			{
				std::map < uint64_t, uint64_t > :: iterator ita = timeToObject.begin();
				uint64_t const otime = ita->first;
				uint64_t const object = ita->second;
				timeToObject.erase(ita);
				return std::pair<uint64_t, uint64_t>(object,otime);
			}
			
			public:
			LRU(uint64_t const rn  = 0) : n(rn), curtime(0), fill(0), objectToTime(n) {}
			
			void update(uint64_t object)
			{
				uint64_t const otime = objectToTime[object];
				timeToObject.erase ( timeToObject.find(otime) );

				++curtime;
				objectToTime[object] = curtime;
				timeToObject[curtime] = object;
			}
			
			std::pair < std::pair < uint64_t, uint64_t >, bool > add()
			{
				std::pair < std::pair < uint64_t, uint64_t >, bool > ot;

				++curtime;
			
				if ( fill < n )
				{
					ot.first = std::pair<uint64_t, uint64_t>(fill++,0);
					ot.second = true;
				}
				else
				{
					ot.first = getAndEraseOldest();
					ot.second = false;
				}

				objectToTime[ot.first.first] = curtime;
				timeToObject[curtime] = ot.first.first;
				
				return ot;
			}
		};
		
	}
}
#endif
