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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_GENERICINPUTBAMPARSEOBJECT_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_GENERICINPUTBAMPARSEOBJECT_HPP

#include <libmaus/util/unique_ptr.hpp>
#include <libmaus/util/shared_ptr.hpp>
#include <libmaus/bambam/parallel/GenericInputSingleDataBamParseInfo.hpp>
#include <libmaus/bambam/parallel/DecompressedBlock.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			struct GenericInputBamParseObject
			{
				typedef GenericInputBamParseObject this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
				GenericInputSingleDataBamParseInfo * parseInfo;
				libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type block;
				
				GenericInputBamParseObject() : parseInfo(0), block() {}
				GenericInputBamParseObject(
					GenericInputSingleDataBamParseInfo * rparseInfo,
					libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type rblock
				) : parseInfo(rparseInfo), block(rblock)
				{
				}
				
				void dispatch()
				{
					parseInfo->parseBlock(block);
				}
			};
		}
	}
}
#endif
