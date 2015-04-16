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
#if ! defined(LIBMAUS2_UTIL_OBJECT_HPP)
#define LIBMAUS2_UTIL_OBJECT_HPP

#include <libmaus2/util/shared_ptr.hpp>
#include <libmaus2/util/unique_ptr.hpp>

namespace libmaus2
{
	namespace util
	{
		struct ObjectBase
		{
			typedef ObjectBase this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
		
			virtual ~ObjectBase()
			{
			
			}		
		};
		
		template<typename _wrapped_type>
		struct Object : public ObjectBase
		{
			typedef _wrapped_type wrapped_type;
			typedef Object<wrapped_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
		
			virtual ~Object()
			{
			
			}
		};
	}
}
#endif
