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
#if ! defined(LIBMAUS_AIO_MEMORYFILE_HPP)
#define LIBMAUS_AIO_MEMORYFILE_HPP

#include <libmaus2/autoarray/AutoArray.hpp>

namespace libmaus2
{
	namespace aio
	{
		struct MemoryFile
		{
			typedef MemoryFile this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			typedef libmaus2::autoarray::AutoArray<char,libmaus2::autoarray::alloc_type_c> array_type;
			typedef array_type::shared_ptr_type array_ptr_type;
			
			array_ptr_type A;
			uint64_t f;
	
			MemoryFile() : A(new array_type(0)), f(0)
			{
			
			}						
		};
	}
}
#endif
