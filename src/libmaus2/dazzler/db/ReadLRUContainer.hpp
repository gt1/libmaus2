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
#if ! defined(LIBMAUS2_DAZZLER_DB_READLRUCONTAINER_HPP)
#define LIBMAUS2_DAZZLER_DB_READLRUCONTAINER_HPP

#include <libmaus2/dazzler/db/DatabaseFile.hpp>
#include <libmaus2/lru/SparseLRU.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace db
		{
			struct ReadLRUContainer
			{
				libmaus2::dazzler::db::DatabaseFile & DB;
				std::vector<libmaus2::dazzler::db::Read> const & meta;
				std::map<uint64_t,std::string> V;
				libmaus2::lru::SparseLRU L;
				libmaus2::aio::InputStream::unique_ptr_type Pbasestr; 
				libmaus2::autoarray::AutoArray<char> tmpspace;
				
				ReadLRUContainer(
					libmaus2::dazzler::db::DatabaseFile & rDB, 
					std::vector<libmaus2::dazzler::db::Read> const & rmeta,
					uint64_t const size
				)
				: DB(rDB), meta(rmeta), V(), L(size), Pbasestr(DB.openBaseStream())
				{
				}
				
				void loadRead(uint64_t const id)
				{
					uint64_t const rlen = meta.at(id).rlen;
					DB.decodeRead(*Pbasestr,tmpspace,rlen);
					V [ id ] = std::string(tmpspace.begin(),tmpspace.begin()+rlen);	
				}
				
				std::string const & operator[](uint64_t const id)
				{
					int64_t const remid = L.get(id);
					
					if ( remid >= 0 )
					{
						assert ( V.find(remid) != V.end() );
						V.erase(V.find(remid));
						loadRead(id);
					}
					else if ( V.find(id) == V.end() )
					{
						loadRead(id);
					}
							
					assert ( V.find(id) != V.end() );
					return V.find(id)->second;
				}
			};
		}
	}
}
#endif
