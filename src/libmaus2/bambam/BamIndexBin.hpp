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
#if ! defined(LIBMAUS2_BAMBAM_BAMINDEXBIN_HPP)
#define LIBMAUS2_BAMBAM_BAMINDEXBIN_HPP

#include <libmaus2/autoarray/AutoArray.hpp>

namespace libmaus2
{
	namespace bambam
	{
		struct BamIndexBin
		{
			typedef BamIndexBin this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			
			typedef std::pair<uint64_t,uint64_t> Chunk;
			
			uint32_t bin;
			libmaus2::autoarray::AutoArray<Chunk> chunks;
			
			BamIndexBin() {}
			
			bool operator<(BamIndexBin const & o) const
			{
				return bin < o.bin;
			}
		};
	}
}
#endif
