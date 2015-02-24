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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_GENERICINPUTCONTROLMERGEHEAPENTRY_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_GENERICINPUTCONTROLMERGEHEAPENTRY_HPP

#include <libmaus/bambam/parallel/DecompressedBlock.hpp>
#include <libmaus/bambam/BamAlignmentDecoderBase.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			struct GenericInputControlMergeHeapEntry
			{
				libmaus::bambam::parallel::DecompressedBlock * block;
				uint64_t coord;
				
				inline void setup()
				{
					uint8_t const * algn4 = reinterpret_cast<uint8_t const *>(block->getNextParsePointer()) + sizeof(uint32_t);
					coord = 
						(static_cast<uint64_t>(static_cast<uint32_t>(libmaus::bambam::BamAlignmentDecoderBase::getRefID(algn4))) << 32)
						|
						static_cast<uint64_t>(static_cast<int64_t>(libmaus::bambam::BamAlignmentDecoderBase::getPos(algn4)) - std::numeric_limits<int32_t>::min());
						;	
				}
				
				GenericInputControlMergeHeapEntry()
				{
				
				}
				GenericInputControlMergeHeapEntry(libmaus::bambam::parallel::DecompressedBlock * rblock)
				: block(rblock)
				{
					setup();
				}
				
				bool isLast() const
				{
					return block->cPP == block->nPP;
				}
				
				bool operator<(GenericInputControlMergeHeapEntry const & A) const
				{
					if ( coord != A.coord )
						return coord < A.coord;
					else
						return block->streamid < A.block->streamid;
				}
			};
		}
	}
}
#endif
