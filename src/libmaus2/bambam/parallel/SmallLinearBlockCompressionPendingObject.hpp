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
#if ! defined(LIBMAUS2_BAMBAM_PARALLEL_SMALLLINEARBLOCKCOMPRESSIONPENDINGOBJECT_HPP)
#define LIBMAUS2_BAMBAM_PARALLEL_SMALLLINEARBLOCKCOMPRESSIONPENDINGOBJECT_HPP

#include <libmaus2/types/types.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			struct SmallLinearBlockCompressionPendingObject
			{
				int64_t blockid;
				uint64_t subid;
				uint8_t * pa;
				uint8_t * pe;

				SmallLinearBlockCompressionPendingObject() : blockid(0), subid(0), pa(0), pe(0) {}
				SmallLinearBlockCompressionPendingObject(
					int64_t const rblockid,
					uint64_t const rsubid,
					uint8_t * const rpa,
					uint8_t * const rpe
				) : blockid(rblockid), subid(rsubid), pa(rpa), pe(rpe) {}
			};
		}
	}
}
#endif
