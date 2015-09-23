/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#if ! defined(LIBMAUS2_DAZZLER_DB_READCONTAINER_HPP)
#define LIBMAUS2_DAZZLER_DB_READCONTAINER_HPP

#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/dazzler/db/DatabaseFile.hpp>
#include <libmaus2/parallel/PosixMutex.hpp>
#include <set>

namespace libmaus2
{
	namespace dazzler
	{
		namespace db
		{
			struct ReadContainer
			{
				typedef std::string string_type;
				typedef libmaus2::util::shared_ptr<string_type>::type string_ptr_type;

				libmaus2::parallel::PosixMutex mutex;
				libmaus2::dazzler::db::DatabaseFile const & DB;
				std::set<uint64_t> missing;
				std::set<uint64_t> seen;
				
				libmaus2::autoarray::AutoArray<char> B;
				libmaus2::autoarray::AutoArray<uint64_t> O;
				std::map<uint64_t,uint64_t> MM;
				
				ReadContainer(libmaus2::dazzler::db::DatabaseFile const & rDB)
				: mutex(), DB(rDB)
				{
				}
				
				void fullreset()
				{
					missing.clear();
					seen.clear();
					B.resize(0);
					O.resize(0);
					MM.clear();
				}

				template<typename iterator>	
				int64_t fill(iterator low, iterator high, uint64_t const maxmem, std::ostream * verbstr = 0)
				{
					MM.clear();
					B.resize(0);
					O.resize(0);

					iterator itc = DB.getReadDataVectorMemInterval(low,high,maxmem,B,O);

					uint64_t id = 0;
					while ( low != itc )
						MM[*(low++)] = id++;

					if ( verbstr )
						(*verbstr) << "[V] refilled " << id << std::endl;

					if ( MM.size() )
						return MM.rbegin()->first;
					else
						return -1;
				}

				uint64_t fillMem(uint64_t const low, uint64_t const maxmem, std::ostream * verbstr = 0)
				{
					MM.clear();
					B.resize(0);
					O.resize(0);
					
					uint64_t const high = DB.getReadDataVectorMemInterval(low,maxmem,B,O);
					for ( uint64_t i = low; i < high; ++i )
						MM [ i ] = i-low;
						
					if ( verbstr )
						(*verbstr) << "[V] fill mem [" << low << "," << high << ")" << std::endl;
					
					return high;
				}

				std::pair<uint64_t,uint64_t> fillMem(uint64_t const low, uint64_t high, uint64_t const maxmem, std::ostream * verbstr = 0)
				{
					MM.clear();
					B.resize(0);
					O.resize(0);
					
					std::pair<uint64_t,uint64_t> const P = DB.getReadDataVectorMemInterval(low,high,maxmem,B,O);
					high = P.first;
					for ( uint64_t i = low; i < high; ++i )
						MM [ i ] = i-low;
						
					if ( verbstr )
						(*verbstr) << "[V] fill mem [" << low << "," << high << ")" << std::endl;
					
					return P;
				}

				void fill(uint64_t const low, uint64_t const high)
				{
					MM.clear();
					B.resize(0);
					O.resize(0);
					
					std::vector<uint64_t> I;
					for ( uint64_t i = low; i < high; ++i )
					{
						I.push_back(i);
						MM [ i ] = i - low;
					}
					DB.getReadDataVector(I,B,O);
				}
				
				std::pair<uint8_t const *, uint64_t> get(uint64_t const i)
				{
					if ( MM.find(i) == MM.end() )
					{
						{
							libmaus2::parallel::ScopePosixMutex slock(mutex);
							missing.insert(i);
							// std::cerr << "missing " << i << std::endl;
						}
						
						return std::pair<uint8_t const *, uint64_t>(0,0);
					}
					else
					{
						uint64_t const id = MM.find(i)->second;
						uint64_t const off0 = O[id];
						uint64_t const off1 = O[id+1];
						
						std::pair<uint8_t const *, uint64_t> const P(reinterpret_cast<uint8_t const *>(B.begin()+off0),off1-off0);
					
						//std::cerr << "Checking " << i << std::endl;
						
						#if 0
						std::string const R = DB[i];
						assert ( R.size() == P.second );
						assert ( memcmp(R.c_str(),P.first,P.second) == 0 );
						#endif
						
						return P;
					}
				}
				
				void reset()
				{
					for ( std::map<uint64_t,uint64_t>::const_iterator ita = MM.begin(); ita != MM.end(); ++ita )		
						seen.insert(ita->first);

					missing.clear();
					
					B.resize(0);
					O.resize(0);
					MM.clear();
				}
				
				bool haveMissing(std::ostream * verbstr = 0) const
				{
					if ( verbstr )
						(*verbstr) << "[V] missing " << missing.size() << std::endl;
					return missing.size();
				}
				
				bool haveSeen(uint64_t const i) const
				{
					return seen.find(i) != seen.end();
				}
			};
		}
	}
}
#endif
