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
#if ! defined(LIBMAUS_BAMBAM_BAMPARALLELDECODINGALIGNMENTBUFFERALLOCATOR_HPP)
#define LIBMAUS_BAMBAM_BAMPARALLELDECODINGALIGNMENTBUFFERALLOCATOR_HPP

#include <libmaus/bambam/BamParallelDecodingAlignmentBuffer.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct BamParallelDecodingAlignmentBufferAllocator
		{
			uint64_t bufferSize;
			uint64_t mult;
			
			BamParallelDecodingAlignmentBufferAllocator()
			: bufferSize(0), mult(0)
			{
			
			}
			
			BamParallelDecodingAlignmentBufferAllocator(uint64_t const rbufferSize, uint64_t const rmult)
			: bufferSize(rbufferSize), mult(rmult)
			{
			
			}
			
			BamParallelDecodingAlignmentBuffer * operator()() const
			{
				return new BamParallelDecodingAlignmentBuffer(bufferSize,mult);
			}
		};
	}
}
#endif
