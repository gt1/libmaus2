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
#if ! defined(LIBMAUS_BAMBAM_BAMINDEXBINCHUNK_HPP)
#define LIBMAUS_BAMBAM_BAMINDEXBINCHUNK_HPP

#include <libmaus/types/types.hpp>
#include <ostream>

namespace libmaus
{
	namespace bambam
	{
		struct BamIndexBinChunk
		{
			uint64_t refid;
			uint64_t bin;
			uint64_t alcmpstart;
			uint64_t alstart;
			uint64_t alcmpend;
			uint64_t alend;
			
			BamIndexBinChunk()
			{
			
			}
			
			BamIndexBinChunk(
				uint64_t const rrefid,
				uint64_t const rbin,
				uint64_t const ralcmpstart,
				uint64_t const ralstart,
				uint64_t const ralcmpend,
				uint64_t const ralend
			)
			: refid(rrefid), bin(rbin), alcmpstart(ralcmpstart), alstart(ralstart),
			  alcmpend(ralcmpend), alend(ralend)
			{

			}
			
			bool operator<(BamIndexBinChunk const & o) const
			{
				if ( refid != o.refid )
					return refid < o.refid;
				if ( bin != o.bin )
					return bin < o.bin;
				if ( alcmpstart != o.alcmpstart )
					return alcmpstart < o.alcmpstart;
				if ( alstart != o.alstart )
					return alstart < o.alstart;
				if ( alcmpend != o.alcmpend )
					return alcmpend < o.alcmpend;
				if ( alend != o.alend )
					return alend < o.alend;
					
				return false;
			}
		};
	}
}

std::ostream & operator<<(std::ostream & out, libmaus::bambam::BamIndexBinChunk const & BC);
#endif
