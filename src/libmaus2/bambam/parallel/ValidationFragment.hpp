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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_VALIDATIONFRAGMENT_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_VALIDATIONFRAGMENT_HPP

#include <libmaus2/bambam/parallel/AlignmentBuffer.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			struct ValidationFragment
			{
				uint64_t low;
				uint64_t high;
				AlignmentBuffer::shared_ptr_type buffer;
				
				ValidationFragment() : low(0), high(0), buffer() {}
				ValidationFragment(uint64_t const rlow, uint64_t const rhigh, AlignmentBuffer::shared_ptr_type rbuffer) : low(rlow), high(rhigh), buffer(rbuffer) {}
				
				bool dispatch()
				{
					return buffer->checkValidPacked(low,high);
				}
				
				void updateChecksums(libmaus2::bambam::ChecksumsInterface & chksums)
				{
					buffer->updateChecksumsPacked(low,high,chksums);
				}
			};
			
			std::ostream & operator<<(std::ostream & out, ValidationFragment const & V);
		}
	}
}
#endif
