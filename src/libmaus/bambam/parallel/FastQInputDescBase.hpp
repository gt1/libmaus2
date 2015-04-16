/*
    libmaus
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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_FASTQINPUTDESCBASE_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_FASTQINPUTDESCBASE_HPP

#include <libmaus/bambam/parallel/GenericInputBlockSubBlockInfo.hpp>
#include <libmaus/bambam/parallel/GenericInputBlock.hpp>
#include <libmaus/bambam/parallel/GenericInputBlockAllocator.hpp>
#include <libmaus/bambam/parallel/GenericInputBlockTypeInfo.hpp>

#include <libmaus/parallel/LockedFreeList.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			struct FastQInputDescBase
			{
				// meta data type for block reading
				typedef libmaus::bambam::parallel::GenericInputBlockSubBlockInfo meta_type;
				// block type
				typedef libmaus::bambam::parallel::GenericInputBlock<meta_type> input_block_type;
				typedef libmaus::parallel::LockedFreeList<
					input_block_type,
					libmaus::bambam::parallel::GenericInputBlockAllocator<meta_type>,
					libmaus::bambam::parallel::GenericInputBlockTypeInfo<meta_type>
				> free_list_type;
			};
		}
	}
}
#endif
