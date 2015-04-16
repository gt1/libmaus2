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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_GENERICINPUTBLOCKALLOCATOR_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_GENERICINPUTBLOCKALLOCATOR_HPP

#include <libmaus/bambam/parallel/GenericInputBlock.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			template<typename _meta_info_type>
			struct GenericInputBlockAllocator
			{
				typedef _meta_info_type meta_info_type;
				typedef GenericInputBlockAllocator<meta_info_type> this_type;
				typedef typename libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef typename libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
				uint64_t blocksize;
				
				GenericInputBlockAllocator(uint64_t rblocksize = 0) : blocksize(rblocksize) {}
				
				typename GenericInputBlock<meta_info_type>::shared_ptr_type operator()()
				{
					typename GenericInputBlock<meta_info_type>::shared_ptr_type ptr(
						new GenericInputBlock<meta_info_type>(blocksize)
					);
					return ptr;
				}
			};
		}
	}
}
#endif
