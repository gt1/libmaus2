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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_GENERICINPUTCONTROLMERGEHEAPENTRY_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_GENERICINPUTCONTROLMERGEHEAPENTRY_HPP

#include <libmaus2/bambam/parallel/DecompressedBlock.hpp>
#include <libmaus2/bambam/BamAlignmentDecoderBase.hpp>
#include <libmaus2/bambam/StrCmpNumRecode.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			struct GenericInputControlMergeHeapEntryCoordinate
			{
				libmaus2::bambam::parallel::DecompressedBlock * block;
				uint64_t coord;
				
				inline void setup()
				{
					uint8_t const * algn4 = reinterpret_cast<uint8_t const *>(block->getNextParsePointer()) + sizeof(uint32_t);
					coord = 
						(static_cast<uint64_t>(static_cast<uint32_t>(libmaus2::bambam::BamAlignmentDecoderBase::getRefID(algn4))) << 32)
						|
						static_cast<uint64_t>(static_cast<int64_t>(libmaus2::bambam::BamAlignmentDecoderBase::getPos(algn4)) - std::numeric_limits<int32_t>::min());
						;	
				}
				
				GenericInputControlMergeHeapEntryCoordinate()
				{
				
				}

				#if 0
				GenericInputControlMergeHeapEntryCoordinate(libmaus2::bambam::parallel::DecompressedBlock * rblock)
				: block(rblock)
				{
					setup();
				}
				#endif
				
				void set(libmaus2::bambam::parallel::DecompressedBlock * rblock)
				{
					block = rblock;
					setup();
				}
				
				bool isLast() const
				{
					return block->cPP == block->nPP;
				}
				
				bool operator<(GenericInputControlMergeHeapEntryCoordinate const & A) const
				{
					if ( coord != A.coord )
						return coord < A.coord;
					else
						return block->streamid < A.block->streamid;
				}
			};

			struct GenericInputControlMergeHeapEntryQueryName
			{
				libmaus2::bambam::parallel::DecompressedBlock * block;
				char const * name;
				int read1;

				libmaus2::autoarray::AutoArray<uint64_t> recoded;
				size_t recodedlength;

				inline void setup()
				{
					uint8_t const * algn4 = reinterpret_cast<uint8_t const *>(block->getNextParsePointer()) + sizeof(uint32_t);

					name = ::libmaus2::bambam::BamAlignmentDecoderBase::getReadName(algn4);
					read1 = ::libmaus2::bambam::BamAlignmentDecoderBase::getFlags(algn4) & ::libmaus2::bambam::BamFlagBase::LIBMAUS_BAMBAM_FREAD1;
					recodedlength = libmaus2::bambam::StrCmpNumRecode::recode(name,recoded);
				}

				GenericInputControlMergeHeapEntryQueryName()
				{
				
				}
				
				#if 0
				GenericInputControlMergeHeapEntryQueryName(libmaus2::bambam::parallel::DecompressedBlock * rblock)
				: block(rblock)
				{
					setup();
				}
				#endif

				void set(libmaus2::bambam::parallel::DecompressedBlock * rblock)
				{
					block = rblock;
					setup();
				}
				
				bool isLast() const
				{
					return block->cPP == block->nPP;
				}
				
				bool oprec(GenericInputControlMergeHeapEntryQueryName const & A) const
				{
					uint64_t const * pa = recoded.begin();
					uint64_t const * pb = A.recoded.begin();
					
					while ( *pa == *pb )
					{
						// same name?
						if ( ! *pa )
						{
							return read1 > A.read1;
						}
						else
						{
							++pa, ++pb;
						}
					}
					
					return *pa < *pb;
				}
				
				bool opcomp(GenericInputControlMergeHeapEntryQueryName const & A) const
				{
					int const r = libmaus2::bambam::StrCmpNum::strcmpnum(name,A.name);
				
					if ( r != 0 )
						return r < 0;
					else
						return read1 > A.read1;				
				}
				
				bool operator<(GenericInputControlMergeHeapEntryQueryName const & A) const
				{
					return oprec(A);
				
					#if 0
					bool const resref = opcomp(A);
					bool const resnew = oprec(A);
					
					if ( resref != resnew )
					{
						std::cerr << "failed comparison for " << name << " " << A.name << std::endl;
					}
					
					assert ( resref == resnew );
					
					return resref;
					#endif
				}
			};
		}
	}
}
#endif
