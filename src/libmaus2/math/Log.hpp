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
#if ! defined(LIBMAUS2_MATH_LOG_HPP)
#define LIBMAUS2_MATH_LOG_HPP

#include <libmaus2/math/IPower.hpp>

namespace libmaus2
{
	namespace math
	{

		template<uint64_t num, uint64_t b>
		struct LogFloor
		{
			static int const log = 1 + LogFloor<num/b,b>::log;
		};

		template<uint64_t b>
		struct LogFloor<0,b>
		{
			static int const log = -1;
		};

		template<uint64_t num, uint64_t b>
		struct LogCeil
		{
			static int const log =
				(::libmaus2::math::IPower< b,LogFloor<num,b>::log >::n == num) ? (LogFloor<num,b>::log) : (LogFloor<num,b>::log+1);
		};
	}
}
#endif
