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
#if ! defined(LIBMAUS2_BAMBAM_PARALLEL_FASTQTOBAMCONTROLSUBREADPENDING_HPP)
#define LIBMAUS2_BAMBAM_PARALLEL_FASTQTOBAMCONTROLSUBREADPENDING_HPP

#include <libmaus2/bambam/parallel/FastQInputDescBase.hpp>
#include <libmaus2/bambam/parallel/DecompressedBlock.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			struct FastqToBamControlSubReadPending
			{
				typedef FastqToBamControlSubReadPending this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				FastQInputDescBase::input_block_type::shared_ptr_type block;
				uint64_t volatile subid;
				uint64_t volatile absid;
				DecompressedBlock::shared_ptr_type deblock;
					
				FastqToBamControlSubReadPending() : block(), subid(0), absid(0), deblock() {}
				FastqToBamControlSubReadPending(
					FastQInputDescBase::input_block_type::shared_ptr_type rblock,
					uint64_t rsubid
				) : block(rblock), subid(rsubid), absid(0), deblock() {}
			};
		}
	}
}
#endif
