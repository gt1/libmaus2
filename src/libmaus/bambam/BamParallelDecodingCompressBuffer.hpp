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
#if ! defined(LIBMAUS_BAMBAM_BAMPARALLELDECODINGCOMPRESSBUFFER_HPP)
#define LIBMAUS_BAMBAM_BAMPARALLELDECODINGCOMPRESSBUFFER_HPP

#include <libmaus/autoarray/AutoArray.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct BamParallelDecodingCompressBuffer
		{
			libmaus::autoarray::AutoArray<uint8_t> inputBuffer;
			libmaus::autoarray::AutoArray<char> outputBuffer;
			size_t compsize;
			
			uint64_t blockid;
			uint64_t subid;
			uint64_t totalsubids;
			
			BamParallelDecodingCompressBuffer(uint64_t const inputBufferSize)
			: inputBuffer(inputBufferSize,false), compsize(0), blockid(0), subid(0), totalsubids(0)
			{
			
			}
			
			bool operator<(BamParallelDecodingCompressBuffer const & o) const
			{
				return subid > o.subid;
			}
		};
	}
}
#endif
