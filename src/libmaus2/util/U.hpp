/*
    libmaus2
    Copyright (C) 2017 German Tischler

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
#if !defined(LIBMAUS2_UTIL_U_HPP)
#define LIBMAUS2_UTIL_U_HPP

#include <libmaus2/util/NumberSerialisation.hpp>

namespace libmaus2
{
	namespace util
	{
		template<typename _utype>
		struct U
		{
			typedef _utype utype;

			utype u;

			U(utype const ru = 0) : u(ru) {}

			std::istream & deserialise(std::istream & in)
			{
				u = libmaus2::util::NumberSerialisation::deserialiseNumber(in,sizeof(u));
				return in;
			}

			std::ostream & serialise(std::ostream & out) const
			{
				libmaus2::util::NumberSerialisation::serialiseNumber(out,u,sizeof(u));
				return out;
			}

			bool operator<(U<utype> const & U) const
			{
				return u < U.u;
			}
		};
	}
}
#endif
