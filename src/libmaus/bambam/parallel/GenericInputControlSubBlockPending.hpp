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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_GENERICINPUTCONTROLSUBBLOCKPENDING_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_GENERICINPUTCONTROLSUBBLOCKPENDING_HPP

#include <libmaus/bambam/parallel/GenericInputBase.hpp>
#include <libmaus/bambam/parallel/MemInputBlock.hpp>
#include <libmaus/bambam/parallel/DecompressedBlock.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			struct GenericInputControlSubBlockPending
			{
				typedef GenericInputControlSubBlockPending this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
				
				GenericInputBase::block_type::shared_ptr_type block;
				uint64_t subblockid;
			
				libmaus::bambam::parallel::MemInputBlock::shared_ptr_type mib;
				libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type db;
				
				GenericInputControlSubBlockPending(
					GenericInputBase::block_type::shared_ptr_type rblock = GenericInputBase::block_type::shared_ptr_type(), 
					uint64_t rsubblockid = 0)
				: block(rblock), subblockid(rsubblockid), mib(), db() {}
			};
		}
	}
}
#endif
