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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_VALIDATEBLOCKWORKPACKAGE_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_VALIDATEBLOCKWORKPACKAGE_HPP

#include <libmaus2/bambam/parallel/AlignmentBuffer.hpp>
#include <libmaus2/parallel/SimpleThreadWorkPackage.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			struct ValidateBlockWorkPackage : public libmaus2::parallel::SimpleThreadWorkPackage
			{
				typedef ValidateBlockWorkPackage this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			
				AlignmentBuffer::shared_ptr_type parseBlock;
	
				ValidateBlockWorkPackage() : libmaus2::parallel::SimpleThreadWorkPackage(), parseBlock() {}
				
				ValidateBlockWorkPackage(
					uint64_t const rpriority, 
					AlignmentBuffer::shared_ptr_type rparseBlock,
					uint64_t const rparseDispatcherId
				)
				: libmaus2::parallel::SimpleThreadWorkPackage(rpriority,rparseDispatcherId), parseBlock(rparseBlock)
				{
				}
			
				char const * getPackageName() const
				{
					return "ValidateBlockWorkPackage";
				}
			};
		}
	}
}
#endif
