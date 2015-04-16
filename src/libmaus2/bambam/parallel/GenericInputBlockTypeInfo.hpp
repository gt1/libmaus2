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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_GENERICINPUTBLOCKTYPEINFO_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_GENERICINPUTBLOCKTYPEINFO_HPP

#include <libmaus2/bambam/parallel/GenericInputBlock.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			template<typename _meta_data_type>
			struct GenericInputBlockTypeInfo
			{
				typedef _meta_data_type meta_data_type;
				typedef GenericInputBlock<meta_data_type> element_type;
				typedef typename element_type::shared_ptr_type pointer_type;			
				
				static pointer_type deallocate(pointer_type /*p*/)
				{
					pointer_type ptr = getNullPointer();
					return ptr;
				}
				
				static pointer_type getNullPointer()
				{
					pointer_type ptr;
					return ptr;
				}
			};
		}
	}
}
#endif
