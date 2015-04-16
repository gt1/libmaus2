/*
    libmaus2
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
#if ! defined(LIBMAUS_BAMBAM_BAMINDEXMETAINFO_HPP)
#define LIBMAUS_BAMBAM_BAMINDEXMETAINFO_HPP

#include <libmaus2/types/types.hpp>

namespace libmaus2
{
	namespace bambam
	{
		struct BamIndexMetaInfo
		{
			int64_t refid;
			uint64_t start;
			uint64_t end;
			uint64_t mapped;
			uint64_t unmapped;
			
			BamIndexMetaInfo() : refid(-1), start(0), end(0), mapped(0), unmapped(0) {}
			BamIndexMetaInfo(
				int64_t const rrefid,
				uint64_t const rstart,
				uint64_t const rend,
				uint64_t const rmapped,
				uint64_t const runmapped
			) : refid(rrefid), start(rstart), end(rend), mapped(rmapped), unmapped(runmapped) {}
		};
	}
}
#endif
