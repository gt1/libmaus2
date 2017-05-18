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
#if ! defined(LIBMAUS2_UTIL_ENUMERATOR_HPP)
#define LIBMAUS2_UTIL_ENUMERATOR_HPP

#include <libmaus2/types/types.hpp>
#include <ostream>
#include <sstream>

namespace libmaus2
{
	namespace util
	{
		template<unsigned int c>
		struct Enumerator
		{
			uint64_t const n;
			uint64_t I[c];

			Enumerator(uint64_t const rn) : n(rn)
			{
				if ( c <= n )
				{
					for ( uint64_t i = 0; i < c; ++i )
						I[i] = i;
				}
				else
				{
					for ( uint64_t i = 0; i < c; ++i )
						I[i] = n;
				}
			}

			bool valid() const
			{
				return I[0] < n;
			}

			bool next()
			{
				for ( unsigned int ii = 0; ii < c; ++ii )
				{
					unsigned int i = c-ii-1;

					if ( I[i]+ii+1 < n )
					{
						uint64_t const b = I[i]+1;

						for ( unsigned int j = i; j < c; ++j )
							I[j] = b+(j-i);

						return true;
					}
				}

				for ( uint64_t i = 0; i < c; ++i )
					I[i] = n;

				return false;
			}

			std::ostream & toString(std::ostream & out) const
			{
				out << "[";
				for ( uint64_t i = 0; i < c; ++i )
					out << I[i] << ";";
				out << "]";

				return out;
			}

			std::string toString() const
			{
				std::ostringstream ostr;
				toString(ostr);
				return ostr.str();
			}
		};
	}
}
#endif
