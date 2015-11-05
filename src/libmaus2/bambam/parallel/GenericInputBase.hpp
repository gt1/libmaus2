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
#if ! defined(LIBMAUS2_BAMBAM_PARALLEL_GENERICINPUTBASE_HPP)
#define LIBMAUS2_BAMBAM_PARALLEL_GENERICINPUTBASE_HPP

#include <libmaus2/bambam/parallel/GenericInputBlock.hpp>
#include <libmaus2/bambam/parallel/GenericInputBlockAllocator.hpp>
#include <libmaus2/bambam/parallel/GenericInputBlockTypeInfo.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			struct GenericInputBase
			{
				typedef GenericInputBase this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				// meta data type for block reading
				typedef libmaus2::bambam::parallel::GenericInputBlockSubBlockInfo meta_type;
				// block type
				typedef libmaus2::bambam::parallel::GenericInputBlock<meta_type> generic_input_block_type;
				// pointer to block type
				typedef generic_input_block_type::unique_ptr_type generic_input_block_ptr_type;
				// shared pointer to block type
				typedef generic_input_block_type::shared_ptr_type generic_input_shared_block_ptr_type;

				// free list type
				typedef libmaus2::parallel::LockedFreeList<
					generic_input_block_type,
					libmaus2::bambam::parallel::GenericInputBlockAllocator<meta_type>,
					libmaus2::bambam::parallel::GenericInputBlockTypeInfo<meta_type>
				> generic_input_block_free_list_type;
				// pointer to free list type
				typedef generic_input_block_free_list_type::unique_ptr_type generic_input_block_free_list_pointer_type;

				// stall array type
				typedef libmaus2::autoarray::AutoArray<uint8_t,libmaus2::autoarray::alloc_type_c> generic_input_stall_array_type;
			};
		}
	}
}
#endif
