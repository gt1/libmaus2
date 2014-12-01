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
#if ! defined(LIBMAUS_BAMBAM_BAMPARALLELDECODINGCONTROL_HPP)
#define LIBMAUS_BAMBAM_BAMPARALLELDECODINGCONTROL_HPP

#include <libmaus/bambam/BamParallelDecodingDecompressedBlock.hpp>
#include <libmaus/bambam/BamParallelDecodingAlignmentBuffer.hpp>
#include <libmaus/bambam/BamParallelDecodingParseInfo.hpp>
#include <libmaus/parallel/SimpleThreadWorkPackage.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct BamParallelDecodingParseBlockWorkPackage : public libmaus::parallel::SimpleThreadWorkPackage
		{
			typedef BamParallelDecodingParseBlockWorkPackage this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			BamParallelDecodingDecompressedBlock * decompressedblock;
			BamParallelDecodingAlignmentBuffer * parseBlock;
			BamParallelDecodingParseInfo * parseInfo;

			BamParallelDecodingParseBlockWorkPackage()
			: 
				libmaus::parallel::SimpleThreadWorkPackage(), 
				decompressedblock(0),
				parseBlock(0),
				parseInfo(0)
			{
			}
			
			BamParallelDecodingParseBlockWorkPackage(
				uint64_t const rpriority, 
				BamParallelDecodingDecompressedBlock * rdecompressedblock,
				BamParallelDecodingAlignmentBuffer * rparseBlock,
				BamParallelDecodingParseInfo * rparseInfo,
				uint64_t const rparseDispatcherId
			)
			: libmaus::parallel::SimpleThreadWorkPackage(rpriority,rparseDispatcherId), 
			  decompressedblock(rdecompressedblock), parseBlock(rparseBlock), parseInfo(rparseInfo)
			{
			}
		
			char const * getPackageName() const
			{
				return "BamParallelDecodingParseBlockWorkPackage";
			}
		};
	}
}
#endif
