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
#if ! defined(LIBMAUS2_BAMBAM_BAMAUXSORTINGBUFFER_HPP)
#define LIBMAUS2_BAMBAM_BAMAUXSORTINGBUFFER_HPP

#include <libmaus2/bambam/BamAuxSortingBufferEntry.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <algorithm>

namespace libmaus2
{
	namespace bambam
	{
		//! aux sorting buffer class
		struct BamAuxSortingBuffer
		{
			//! entry buffer vector
			libmaus2::autoarray::AutoArray<BamAuxSortingBufferEntry> B;
			//! number of elements in entry buffer vector
			uint64_t fill;
			//! buffer for rewriting aux areas
			libmaus2::autoarray::AutoArray<uint8_t> U;
			
			//! constructor for empty sorting buffer
			BamAuxSortingBuffer() : B(), U()
			{
			
			}
			
			/**
			 * add an element to the buffer
			 *
			 * @param E entry to be added
			 **/
			void push(BamAuxSortingBufferEntry const & E)
			{
				if ( fill == B.size() )
				{
					uint64_t const newsize = fill ? 2*fill : 1;
					libmaus2::autoarray::AutoArray<BamAuxSortingBufferEntry> N(newsize,false);
					std::copy(B.begin(),B.end(),N.begin());
					B = N;
				}
				
				B[fill++] = E;
			}
			
			/**
			 * empty buffer
			 **/
			void reset()
			{
				fill = 0;
			}
			
			/**
			 * sort entry buffer
			 **/
			void sort()
			{
				::std::sort(B.begin(),B.begin()+fill);
			}
		};
	}
}
#endif
