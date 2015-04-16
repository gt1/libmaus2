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

#if ! defined(LIBMAUS_UTIL_UNORDERED_MAP_HPP)
#define LIBMAUS_UTIL_UNORDERED_MAP_HPP

#include <libmaus/LibMausConfig.hpp>

#if defined(LIBMAUS_USE_STD_UNORDERED_MAP)
#include <unordered_map>
#elif defined(LIBMAUS_USE_BOOST_UNORDERED_MAP)
#include <boost/unordered_map.hpp>
#include <boost/functional/hash.hpp>
#endif

namespace libmaus
{
	namespace util
	{
		template<typename T>
		struct unordered_map_hash
		{
			#if defined(LIBMAUS_USE_STD_UNORDERED_MAP)
			typedef std::hash<T> hash_type;
			#elif defined(LIBMAUS_USE_BOOST_UNORDERED_MAP)
			typedef ::boost::hash<T> hash_type;
			#else
			#error "Required unordered_map not found."
			#endif			
		};
	
		template<typename T1, typename T2, typename H = typename unordered_map_hash<T1>::hash_type >
		struct unordered_map
		{
			#if defined(LIBMAUS_USE_STD_UNORDERED_MAP)
			typedef typename ::std::unordered_map<T1,T2,H> type;			
			#elif defined(LIBMAUS_USE_BOOST_UNORDERED_MAP)
			typedef typename ::boost::unordered_map<T1,T2,H> type;
			#else
			#error "Required unordered_map not found."
			#endif
		};
	}
}
#endif
