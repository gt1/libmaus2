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
#if ! defined(LIBMAUS_LRU_SPARSELRU_HPP)
#define LIBMAUS_LRU_SPARSELRU_HPP

#include <libmaus/types/types.hpp>
#include <libmaus/util/unordered_map.hpp>

#include <map>
#include <cassert>

namespace libmaus
{
	namespace lru
	{
		struct SparseLRU
		{
			uint64_t curtime;
			uint64_t maxactive;
			
			libmaus::util::unordered_map<uint64_t,uint64_t>::type objectToTime;
			std::map<uint64_t,uint64_t> timeToObject;
			
			SparseLRU(uint64_t const rmaxactive) : curtime(0), maxactive(rmaxactive) {}
			
			void update(uint64_t const objectid)
			{
				libmaus::util::unordered_map<uint64_t,uint64_t>::type::iterator it = objectToTime.find(objectid);
				assert ( it != objectToTime.end() );
				
				uint64_t const otime = it->second;

				std::map<uint64_t,uint64_t>::iterator mit = timeToObject.find(otime);
				assert ( mit != timeToObject.end() );
				
				objectToTime.erase(it);
				timeToObject.erase(mit);
				
				uint64_t const ntime = curtime++;
				
				objectToTime[objectid] = ntime;
				timeToObject[ntime] = objectid;
			}
			
			void erase(uint64_t const objectid)
			{
				libmaus::util::unordered_map<uint64_t,uint64_t>::type::iterator it = objectToTime.find(objectid);
				assert ( it != objectToTime.end() );
				uint64_t otime = it->second;
				
				objectToTime.erase(it);
				
				std::map<uint64_t,uint64_t>::iterator oit = timeToObject.find(otime);
				assert ( oit != timeToObject.end() );
				
				timeToObject.erase(oit);
			}
			
			int64_t get(uint64_t const objectid)
			{
				libmaus::util::unordered_map<uint64_t,uint64_t>::type::iterator it = objectToTime.find(objectid);
				
				// object is present, update access time
				if ( it != objectToTime.end() )
				{
					update(objectid);
					return -1;
				}
				
				// object is not present but there is more space
				if ( objectToTime.size() < maxactive )
				{
					uint64_t const ntime = curtime++;
					objectToTime[objectid] = ntime;
					timeToObject[ntime] = objectid;
					return -1;
				}
				
				// object is not present and we need to remove an element
				std::map<uint64_t,uint64_t>::iterator otimeit = timeToObject.begin();
				uint64_t const oobject = otimeit->second;
				libmaus::util::unordered_map<uint64_t,uint64_t>::type::iterator oobjectit = objectToTime.find(oobject);
				
				timeToObject.erase(otimeit);
				objectToTime.erase(oobjectit);
				
				uint64_t const ntime = curtime++;
				objectToTime[objectid] = ntime;
				timeToObject[ntime] = objectid;
				
				return static_cast<int64_t>(oobject);
			}
		};
	}
}
#endif
