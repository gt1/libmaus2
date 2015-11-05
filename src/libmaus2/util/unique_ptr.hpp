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

#if ! defined(LIBMAUS2_UTIL_UNIQUE_PTR_HPP)
#define LIBMAUS2_UTIL_UNIQUE_PTR_HPP

#include <libmaus2/LibMausConfig.hpp>

#if defined(LIBMAUS2_USE_STD_UNIQUE_PTR)
#include <memory>
#elif defined(LIBMAUS2_USE_BOOST_UNIQUE_PTR)
#include <libmaus2/deleter/Deleter.hpp>
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#endif

#if defined(LIBMAUS2_USE_STD_MOVE)
#define UNIQUE_PTR_MOVE(obj) ::std::move(obj)
#elif defined(LIBMAUS2_USE_BOOST_MOVE)
#include <boost/move/move.hpp>
#define UNIQUE_PTR_MOVE(obj) ::boost::move(obj)
#else
#error "Required move function for unique_ptr objects not found."
#endif

namespace libmaus2
{
	namespace util
	{
		template<typename T>
		struct unique_ptr
		{
			#if defined(LIBMAUS2_USE_STD_UNIQUE_PTR)
			typedef typename ::std::unique_ptr<T> type;
			#elif defined(LIBMAUS2_USE_BOOST_UNIQUE_PTR)
			typedef typename ::boost::interprocess::unique_ptr<T,::libmaus2::deleter::Deleter<T> > type;
			#else
			#error "Required unique_ptr not found."
			#endif
		};
	}
}
#endif
