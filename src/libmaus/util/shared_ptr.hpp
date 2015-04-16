/*
    libmaus
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

#if ! defined(LIBMAUS_UTIL_SHARED_PTR_HPP)
#define LIBMAUS_UTIL_SHARED_PTR_HPP

#include <libmaus/LibMausConfig.hpp>

#if defined(LIBMAUS_USE_STD_SHARED_PTR)
#include <memory>
#elif defined(LIBMAUS_USE_BOOST_SHARED_PTR)
#include <boost/shared_ptr.hpp>
#endif

namespace libmaus
{
	namespace util
	{
		template<typename T>
		struct shared_ptr
		{
			#if defined(LIBMAUS_USE_STD_SHARED_PTR)
			typedef typename ::std::shared_ptr<T> type;			
			#elif defined(LIBMAUS_USE_BOOST_SHARED_PTR)
			typedef typename ::boost::shared_ptr<T> type;
			#else
			#error "Required shared_ptr not found."
			#endif
		};
	}
}
#endif
