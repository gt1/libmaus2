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
#if ! defined(LIBMAUS_UTIL_PUSHBUFFER_HPP)
#define LIBMAUS_UTIL_PUSHBUFFER_HPP

#include <libmaus/autoarray/AutoArray.hpp>

namespace libmaus
{
	namespace util
	{
		template<typename _value_type>
		struct PushBuffer
		{
			typedef _value_type value_type;
			typedef PushBuffer<value_type> this_type;

			libmaus::autoarray::AutoArray<value_type> A;
			uint64_t f;
			
			PushBuffer() : A(), f(0) {}
			
			void push(value_type const & o)
			{
				if ( f == A.size() )
				{
					uint64_t const newsize = A.size() ? 2*A.size() : 1;
					libmaus::autoarray::AutoArray<value_type> B(newsize,false);
					std::copy(A.begin(),A.end(),B.begin());
					A = B;
				}
				
				A[f++] = o;
			}
			
			value_type & get()
			{
				if ( f == A.size() )
				{
					uint64_t const newsize = A.size() ? 2*A.size() : 1;
					libmaus::autoarray::AutoArray<value_type> B(newsize,false);
					std::copy(A.begin(),A.end(),B.begin());
					A = B;
				}
			
				return A[f++];
			}
			
			void reset()
			{
				f = 0;
			}
			
			value_type const * begin() const
			{
				return A.begin();
			}

			value_type * begin()
			{
				return A.begin();
			}

			value_type const * end() const
			{
				return A.begin()+f;
			}

			value_type * end()
			{
				return A.begin()+f;
			}
		};
	}
}
#endif
