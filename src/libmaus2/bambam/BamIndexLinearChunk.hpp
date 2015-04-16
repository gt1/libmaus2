/*
    libmaus2
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
#if ! defined(LIBMAUS2_BAMBAM_BAMINDEXLINEARCHUNK_HPP)
#define LIBMAUS2_BAMBAM_BAMINDEXLINEARCHUNK_HPP

#include <libmaus2/types/types.hpp>
#include <ostream>

namespace libmaus2
{
	namespace bambam
	{
		struct BamIndexLinearChunk
		{
			uint64_t refid;
			uint64_t pos;
			uint64_t alcmpstart;
			uint64_t alstart;
			int64_t chunkid;
			
			BamIndexLinearChunk() : refid(0), pos(0), alcmpstart(0), alstart(0), chunkid(-1)
			{
			
			}

			BamIndexLinearChunk(
				uint64_t const rrefid,
				uint64_t const rpos,
				uint64_t const ralcmpstart,
				uint64_t const ralstart,
				int64_t  const rchunkid
			)
			: refid(rrefid), pos(rpos), alcmpstart(ralcmpstart), alstart(ralstart), chunkid(rchunkid) {}
			
			bool operator<(BamIndexLinearChunk const & o) const
			{
				if ( refid != o.refid )
					return refid < o.refid;
				else
					return chunkid < o.chunkid;
			}
			
			uint64_t getOffset() const
			{
				return (alcmpstart<<16)|(alstart);
			}
		};
		
		::std::ostream & operator<<(::std::ostream & out, BamIndexLinearChunk const & o);
	}
}

#endif
