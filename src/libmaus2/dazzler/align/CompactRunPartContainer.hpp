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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_COMPACTRUNPARTCONTAINER_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_COMPACTRUNPARTCONTAINER_HPP

#include <libmaus2/dazzler/db/PartTrackContainer.hpp>
#include <libmaus2/dazzler/align/ApproximateRun.hpp>
#include <sstream>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct CompactRunPartContainer : public libmaus2::dazzler::db::PartTrackContainer
			{
				CompactRunPartContainer(libmaus2::dazzler::db::DatabaseFile const & DB, int64_t const blockid = -1)
				: PartTrackContainer(DB,"compactruns",blockid) 
				{
				}

				std::vector< std::pair<uint64_t,uint64_t> > getApproximateRuns(uint64_t const readid) const
				{
					uint64_t const blockid = DB.getBlockForIdTrimmed(readid);
					assert ( blockid >= 1 );
					assert ( blockid <= DB.numblocks );
					libmaus2::dazzler::db::Track const & track = libmaus2::dazzler::db::PartTrackContainer::getTrack(blockid);
					std::pair<uint64_t,uint64_t> const B = DB.getTrimmedBlockInterval(blockid);
					assert ( readid >= B.first );
					assert ( readid < B.second );
					libmaus2::dazzler::db::TrackAnnoInterface const & anno = track.getAnno();
					uint64_t const offset = anno[readid-B.first];
					uint64_t const len = anno[readid+1-B.first] - offset;
					assert ( track.Adata );
					assert ( offset <= track.Adata->size() );
					assert ( offset + len <= track.Adata->size() );
					std::string const appdata(track.Adata->begin() + offset,track.Adata->begin() + offset + len);
					std::istringstream appistr(appdata);
					
					uint64_t roffset = 0;
					uint64_t const numel = len ? libmaus2::dazzler::db::InputBase::getLittleEndianInteger8(appistr,roffset) : 0;
					std::pair<uint64_t,uint64_t> V(numel);
					for ( uint64_t i = 0; i < numel; ++i )
					{
						uint64_t const p0 = libmaus2::dazzler::db::InputBase::getLittleEndianInteger8(appistr,offset);
						uint64_t const p1 = libmaus2::dazzler::db::InputBase::getLittleEndianInteger8(appistr,offset);
						V[i] = std::pair<uint64_t,uint64_t>(p0,p1);
					}
					
					return V;
				}


			};
		}
	}
}
#endif
