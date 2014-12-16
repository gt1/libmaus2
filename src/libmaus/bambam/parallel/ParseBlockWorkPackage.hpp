/*
    libmaus
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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_PARSEBLOCKWORKPACKAGE_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_PARSEBLOCKWORKPACKAGE_HPP

#include <libmaus/bambam/parallel/DecompressedBlock.hpp>
#include <libmaus/bambam/parallel/AlignmentBuffer.hpp>
#include <libmaus/bambam/parallel/ParseInfo.hpp>
#include <libmaus/parallel/SimpleThreadWorkPackage.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			struct ParseBlockWorkPackage : public libmaus::parallel::SimpleThreadWorkPackage
			{
				typedef ParseBlockWorkPackage this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
				DecompressedBlock::shared_ptr_type decompressedblock;
				AlignmentBuffer::shared_ptr_type parseBlock;
				ParseInfo * parseInfo;
	
				ParseBlockWorkPackage()
				: 
					libmaus::parallel::SimpleThreadWorkPackage(), 
					decompressedblock(0),
					parseBlock(0),
					parseInfo(0)
				{
				}
				
				ParseBlockWorkPackage(
					uint64_t const rpriority, 
					DecompressedBlock::shared_ptr_type rdecompressedblock,
					AlignmentBuffer::shared_ptr_type rparseBlock,
					ParseInfo * rparseInfo,
					uint64_t const rparseDispatcherId
				)
				: libmaus::parallel::SimpleThreadWorkPackage(rpriority,rparseDispatcherId), 
				  decompressedblock(rdecompressedblock), parseBlock(rparseBlock), parseInfo(rparseInfo)
				{
				}
			
				char const * getPackageName() const
				{
					return "ParseBlockWorkPackage";
				}
			};
		}
	}
}
#endif
