/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#if ! defined(LIBMAUS2_UTIL_REVERSEADAPTER_HPP)
#define LIBMAUS2_UTIL_REVERSEADAPTER_HPP

#include <libmaus2/LibMausConfig.hpp>
#if defined(LIBMAUS2_HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include <iterator>

namespace libmaus2
{
	namespace util
	{
		template<typename iterator>
		struct ReverseAdapter
		{
			iterator ita;
			iterator ite;
			
			ReverseAdapter()
			{
			
			}
			
			ReverseAdapter(iterator rita, iterator rite)
			: ita(rita), ite(rite)
			{
			
			}
			
			typename ::std::iterator_traits<iterator>::value_type get(ssize_t i) const
			{
				return *(ite-i-1);
			}
		};
	}
}
#endif
