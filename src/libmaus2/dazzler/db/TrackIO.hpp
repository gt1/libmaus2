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
#if ! defined(LIBMAUS2_DAZZLER_DB_TRACKIO_HPP)
#define LIBMAUS2_DAZZLER_DB_TRACKIO_HPP

#include <libmaus2/dazzler/db/InputBase.hpp>
#include <libmaus2/dazzler/db/DatabaseFile.hpp>
#include <libmaus2/aio/InputStreamFactoryContainer.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		struct TrackIO : public libmaus2::dazzler::db::InputBase
		{
			static void readTrack(std::string const & path, std::string const & root, int64_t const part, std::string const & trackname)
			{
				std::string const annoname = libmaus2::dazzler::db::DatabaseFile::getTrackAnnoFileName(path,root,part,trackname);
				std::string const dataname = libmaus2::dazzler::db::DatabaseFile::getTrackDataFileName(path,root,part,trackname);
				
				libmaus2::aio::InputStream::unique_ptr_type Panno(libmaus2::aio::InputStreamFactoryContainer::constructUnique(annoname));
				std::istream & anno = *Panno;
			
				uint64_t offset = 0;
				int32_t tracklen = getLittleEndianInteger4(anno,offset);
				int32_t size = getLittleEndianInteger4(anno,offset);
				
				libmaus2::aio::InputStream::unique_ptr_type Pdata;
				if ( libmaus2::aio::InputStreamFactoryContainer::tryOpen(dataname) )
				{
					libmaus2::aio::InputStream::unique_ptr_type Tdata(libmaus2::aio::InputStreamFactoryContainer::constructUnique(dataname));
					Pdata = UNIQUE_PTR_MOVE(Tdata);
				}
				
				// incomplete
			}
		};	
	}
}
#endif
