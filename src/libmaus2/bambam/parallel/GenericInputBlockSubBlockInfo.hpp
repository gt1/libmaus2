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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_GENERICINPUTBLOCKSUBBLOCKINFO_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_GENERICINPUTBLOCKSUBBLOCKINFO_HPP

#include <libmaus2/parallel/LockedCounter.hpp>
#include <vector>
#include <utility>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			struct GenericInputBlockSubBlockInfo
			{
				typedef GenericInputBlockSubBlockInfo this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
				
				std::vector< std::pair<uint8_t *,uint8_t *> > blocks;
				libmaus2::parallel::LockedCounter returnedBlocks;
				uint64_t streamid;
				uint64_t blockid;
				bool eof;
				
				size_t byteSize() const
				{
					return
						blocks.size() * sizeof(std::pair<uint8_t *,uint8_t *>) +
						sizeof(returnedBlocks) +
						sizeof(streamid) +
						sizeof(blockid) +
						sizeof(eof);
				}
				
				GenericInputBlockSubBlockInfo() : returnedBlocks(0), streamid(0), blockid(0), eof(false)
				{
				
				}

				void addBlock(std::pair<uint8_t *,uint8_t *> const & P)
				{
					blocks.push_back(P);
				}
				
				bool returnBlock()
				{
					return returnedBlocks.increment() == blocks.size();
				}

				void reset()
				{
					blocks.resize(0);
					returnedBlocks -= static_cast<uint64_t>(returnedBlocks);
				}
			};
		}
	}
}
#endif
