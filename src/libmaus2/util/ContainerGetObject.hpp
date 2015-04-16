/*
    libmaus2
    Copyright (C) 2009-2013 German Tischler
    Copyright (C) 2011-2013 Genome Research Limited

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
#if ! defined(LIBMAUS2_UTIL_CONTAINERGETOBJECT_HPP)
#define LIBMAUS2_UTIL_CONTAINERGETOBJECT_HPP

#include <libmaus2/types/types.hpp>

namespace libmaus2
{
	namespace util
	{
		/**
		 * get wrapper class for a linear sequence container;
		 * the get function returns the elements in the container
		 * from front to back
		 **/
		template<typename container_type>
		struct ContainerGetObject
		{
			//! wrapped container
			container_type const & container;
			//! current index
			uint64_t i;
			
			/**
			 * constructor
			 *
			 * @param rcontainer container to be wrapped
			 **/
			ContainerGetObject(container_type const & rcontainer) : container(rcontainer), i(0) {}
			
			/**
			 * @return next element
			 **/
			int get()
			{
				if ( i == container.size() )
					return -1;
				else
					return container[i++];
			}
		};
	}
}
#endif
