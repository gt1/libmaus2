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
#if ! defined(LIBMAUS2_DAZZLER_DB_PARTTRACKCONTAINER_HPP)
#define LIBMAUS2_DAZZLER_DB_PARTTRACKCONTAINER_HPP

#include <libmaus2/dazzler/db/DatabaseFile.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace db
		{
			struct PartTrackContainer
			{
				::libmaus2::dazzler::db::DatabaseFile const & DB;
				libmaus2::autoarray::AutoArray<libmaus2::dazzler::db::Track::unique_ptr_type> Atracks;
				
				PartTrackContainer(libmaus2::dazzler::db::DatabaseFile const & rDB, std::string const trackname)
				: DB(rDB), Atracks(DB.numblocks)
				{
					for ( uint64_t i = 0; i < Atracks.size(); ++i )
					{
						try
						{
							libmaus2::dazzler::db::Track::unique_ptr_type Ttrack(DB.readTrack(trackname,i+1));
							Atracks[i] = UNIQUE_PTR_MOVE(Ttrack);							
						}
						catch(...)
						{
						
						}
					}
				}
			
				bool haveBlock(uint64_t const blockid) const
				{
					return blockid >= 1 && blockid <= Atracks.size() && Atracks[blockid-1];
				}
				
				libmaus2::dazzler::db::Track const & getTrack(uint64_t const blockid) const
				{
					if ( haveBlock(blockid) )
						return *(Atracks[blockid-1]);
					else
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "PartTrackContainer::getTrack(): track for blockid " << blockid << " is not loaded" << std::endl;
						lme.finish();
						throw lme;
					}
				}
			};
		}
	}
}
#endif
