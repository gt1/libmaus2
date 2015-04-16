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

#if ! defined(LIBMAUS_UTIL_NSTAT_HPP)
#define LIBMAUS_UTIL_NSTAT_HPP

#include <vector>
#include <libmaus2/types/types.hpp>
#include <numeric>
#include <libmaus2/exception/LibMausException.hpp>

namespace libmaus2
{
	namespace util
	{
		struct NStat
		{
			/**
			 * compute n staticic use ta = 50, b = 100 for n50
			 * 
			 * @param ta numerator
			 * @param b denominator
			 * @param nval reference for storing n value
			 * @param avg reference for storing average value
			 **/
			static void nstat(
				std::vector<uint64_t> const & clens,
				uint64_t const ta,
				uint64_t const b,
				double & nval,
				double & avg
				);
		};
	}
}
#endif
