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

#if ! defined(LIBMAUS2_UTIL_UNORDERED_SET_HPP)
#define LIBMAUS2_UTIL_UNORDERED_SET_HPP

#include <libmaus2/LibMausConfig.hpp>

#if defined(LIBMAUS2_USE_STD_UNORDERED_SET)
#include <unordered_set>
#elif defined(LIBMAUS2_USE_BOOST_UNORDERED_SET)
#include <boost/unordered_set.hpp>
#include <boost/functional/hash.hpp>
#endif

namespace libmaus2
{
	namespace util
	{
		template<typename T>
		struct unordered_set_hash
		{
			#if defined(LIBMAUS2_USE_STD_UNORDERED_MAP)
			typedef std::hash<T> hash_type;
			#elif defined(LIBMAUS2_USE_BOOST_UNORDERED_MAP)
			typedef ::boost::hash<T> hash_type;
			#else
			#error "Required unordered_set not found."
			#endif
		};

		template<typename T, typename H = typename unordered_set_hash<T>::hash_type >
		struct unordered_set
		{
			#if defined(LIBMAUS2_USE_STD_UNORDERED_SET)
			typedef typename ::std::unordered_set<T,H> type;
			#elif defined(LIBMAUS2_USE_BOOST_UNORDERED_SET)
			typedef typename ::boost::unordered_set<T,H> type;
			#else
			#error "Required unordered_set not found."
			#endif
		};
	}
}
#endif
