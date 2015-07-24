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
#if ! defined(LIBMAUS2_DAZZLER_DB_TRACK_HPP)
#define LIBMAUS2_DAZZLER_DB_TRACK_HPP

#include <libmaus2/dazzler/db/TrackAnno.hpp>

namespace libmaus2
{
	namespace dazzler
	{	
		struct Track
		{
			typedef Track this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			
		
			std::string name;
			TrackAnnoInterface::unique_ptr_type PDanno;
			libmaus2::autoarray::AutoArray<unsigned char>::unique_ptr_type Adata;
		
			Track(std::string const & rname, TrackAnnoInterface::unique_ptr_type & rPDanno, libmaus2::autoarray::AutoArray<unsigned char>::unique_ptr_type & rAdata)
			: name(rname), PDanno(UNIQUE_PTR_MOVE(rPDanno)), Adata(UNIQUE_PTR_MOVE(rAdata))
			{
			
			}
		};
	}
}
#endif
