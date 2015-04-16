/*
    libmaus2
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#if ! defined(LIBMAUS2_BAMBAM_PARALLEL_SAMPARSEPENDING_HPP)
#define LIBMAUS2_BAMBAM_PARALLEL_SAMPARSEPENDING_HPP

#include <libmaus2/bambam/parallel/GenericInputBase.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			struct SamParsePending
			{
				libmaus2::bambam::parallel::GenericInputBase::generic_input_shared_block_ptr_type block;
				uint64_t subid;
				uint64_t absid;
				
				SamParsePending() : block(), subid(0), absid(0) {}
				SamParsePending(
					libmaus2::bambam::parallel::GenericInputBase::generic_input_shared_block_ptr_type rblock,
					uint64_t rsubid,
					uint64_t rabsid
				) : block(rblock), subid(rsubid), absid(rabsid)
				{
				
				}
			};
		}
	}
}
#endif
